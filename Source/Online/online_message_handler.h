//-----------------------------------------------------------------------------
//           Name: online_message_handler.h
//      Developer: Wolfire Games LLC
//         Author: Max Danielsson
//    Description: A manager for online messages, handling all the types,
//                 their serialization and allocation
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#pragma once

#include <Memory/block_allocator.h>
#include <Memory/allocation.h>
#include <Online/online_datastructures.h>

#include <binn.h>

#include <tuple>
#include <array>
#include <map>
#include <mutex>

using std::tuple;
using std::size_t;
using std::array;
using std::map;
using std::mutex;
using std::lock_guard;
using std::get;
using std::endl;

/*
    This is a template hack that allows us to calculate a static
    Components id in compile-time
*/
template <class T, class Tuple>
struct Index;

template <class T, class... Types>
struct Index<T, tuple<T, Types...>> {
    static const size_t value = 0;
};

template <class T, class U, class... Types>
struct Index<T, tuple<U, Types...>> {
    static const size_t value = 1 + Index<T, tuple<Types...>>::value;
};

template<typename... Messages>
class OnlineMessageHandlerTemplate {

public:
    static const int COUNT = sizeof...(Messages);

private:
    uint32_t message_id_counter = 1;

    void* block_allocator_memory;
    mutex block_allocator_lock;
    BlockAllocator block_allocator;

    mutex message_lock;
    map<OnlineMessageID, tuple<uint32_t, OnlineMessageType, void*>> messages;

    array<size_t, sizeof...(Messages)> sizes = {{(sizeof(Messages))...}};
    array<binn* (*)(void*),sizeof...(Messages)> serialize_functions = {{(&Messages::Serialize)...}};
    array<void (*)(void*, binn*), sizeof...(Messages)> deserialize_functions = {{(&Messages::Deserialize)...}};
    array<void (*)(const OnlineMessageRef&, void*, PeerID), sizeof...(Messages)> execute_functions = {{(&Messages::Execute)...}};
    array<void* (*)(void*), sizeof...(Messages)> construct_functions = {{(&Messages::Construct)...}};
    array<void (*)(void*), sizeof...(Messages)> destruct_functions = {{(&Messages::Destroy)...}};

public:
    OnlineMessageHandlerTemplate() {
        int block_size = 64;
        int block_count = 1024 * 16;
        block_allocator_memory = OG_MALLOC(block_size * block_count);
        block_allocator_lock.lock();
        block_allocator.Init(block_allocator_memory, block_count, block_size);
        block_allocator_lock.unlock();
    }

    ~OnlineMessageHandlerTemplate() {
        ForceRemoveAllMessages();
        OG_FREE(block_allocator_memory);
    }

    void ForceRemoveAllMessages() {
        message_lock.lock();
        for(auto& v : messages) {
            get<0>(v.second) = 0;
        } 
        message_lock.unlock();

        FreeUnreferencedMessages();
    }

    void FreeUnreferencedMessages() {
        while(StepFreeUnreferencedMessage()) { }
    }

    bool StepFreeUnreferencedMessage() {
        OnlineMessageID remove_message = 0;  
        OnlineMessageType remove_message_type = 0;
        void* remove_message_ptr = nullptr;
        bool ret_value = false;

        message_lock.lock();
        for(auto& v : messages) {
            if(get<0>(v.second) <= 0) {
                remove_message = v.first;
                remove_message_type = get<1>(v.second);
                remove_message_ptr = get<2>(v.second);
                break;
            }
        }

        if(remove_message != 0) {
            destruct_functions[remove_message_type](remove_message_ptr);
            block_allocator_lock.lock();
            block_allocator.Free(get<2>(messages[remove_message]));
            block_allocator_lock.unlock();

            messages.erase(remove_message);
            ret_value = true;
        }

        message_lock.unlock();
        return ret_value;
    }

    template<typename Message, typename... VArgs>
    OnlineMessageRef Create(VArgs... args) {
        block_allocator_lock.lock();
        void* memory = block_allocator.Alloc(sizeof(Message));
        block_allocator_lock.unlock();

        message_lock.lock();
        OnlineMessageID message_id = message_id_counter;
        message_id_counter++;

        Message* message = new(memory) Message(args...);

        //Set this to 1, so we do an initial Aquire on the object, to prevent it from being 
        //deleted while we create the reference.
        if(message != nullptr) {
            messages[message_id] = std::tuple<unsigned int, short unsigned int, void*>(1, GetMessageType<Message>(), message);
        } else {
            LOGF << "Got a nullptr when trying to create a message, this is a program killer" << endl;
        }
        message_lock.unlock();

        //This constructor will call Acquire on the message id, requiring a lock.
        OnlineMessageRef message_ref(message_id);

        //Release the initial acquire, making the message_ref the only reference holder.
        Release(message_id);

        return message_ref;
    }

    //Return a const ref so we do a move up, and not an unecessary copy.
    OnlineMessageRef Create(OnlineMessageType message_type) {
        block_allocator_lock.lock();
        void* memory = block_allocator.Alloc(sizes[message_type]);
        block_allocator_lock.unlock();

        message_lock.lock();
        OnlineMessageID message_id = message_id_counter;
        message_id_counter++;

        void* message = construct_functions[message_type](memory);

        //Set this to 1, so we do an initial Aquire on the object, to prevent it from being 
        //deleted while we create the reference.
        if(message != nullptr) {
            messages[message_id] = std::tuple<unsigned int, short unsigned int, void*>(1, message_type, message);
        } else {
            LOGF << "Got a nullptr when trying to create a message, this is a program killer" << endl;
        }

        message_lock.unlock();

        //This constructor will call Acquire on the message id, requiring a lock.
        OnlineMessageRef message_ref(message_id);

        //Release the initial acquire, making the message_ref the only reference holder.
        Release(message_id);

        return message_ref;
    }

    template<typename Message>
    OnlineMessageType GetMessageType() {
        return Index<Message,tuple<Messages...>>::value;
    }

    /*
    template<typename Message>
    Message* Get(const OnlineMessageRef& message_ref) {
        return static_cast<Message*>(get<2>(messages[message_ref.GetID()]));
    }
    */

    void Release(OnlineMessageID message_id) {
        //We can optimize the Acquire/Release here by keeping better track of id allocation
        //and pre-creating an array that holds reference counts, rather than using a map.
        lock_guard<mutex> lg(message_lock);
        auto message = messages.find(message_id);
        if(message != messages.end()) {
            get<0>(message->second)--;
        }
    }

    void Acquire(OnlineMessageID message_id) {
        lock_guard<mutex> lg(message_lock);
        auto message = messages.find(message_id);
        if(message != messages.end()) {
            get<0>(message->second)++;
        }
    }

    binn* Serialize(const OnlineMessageRef& message_ref) {
        message_lock.lock();
        auto message = messages.find(message_ref.GetID());
        message_lock.unlock();

        if(message != messages.end()) {
            return serialize_functions[get<1>(message->second)](get<2>(message->second));
        }
        return binn_object();
    }

    OnlineMessageRef Deserialize(OnlineMessageType message_type, binn* l) {
        OnlineMessageRef message_ref = Create(message_type);

        message_lock.lock();
        auto message = messages.find(message_ref.GetID());
        message_lock.unlock();

        if(message != messages.end()) {
            deserialize_functions[message_type](get<2>(message->second), l);
        }

        return message_ref;
    }

    void Execute(const OnlineMessageRef& message_ref, PeerID from) {
        //We can optimize away this messages de-reference by storing the pointer and type in OnlineMessageRef,
        //which could potentially be a speed-boost. The tradeoff is that the size of OnlineMessageRef would increase.
        //same optimization is applicable to Deserialize, Serialize and Destroy.
        message_lock.lock();
        auto message = messages.find(message_ref.GetID());
        message_lock.unlock();

        if(message != messages.end()) {
            execute_functions[get<1>(message->second)](message_ref, get<2>(message->second), from);
        }
    }

    OnlineMessageType GetMessageType(const OnlineMessageRef& message_ref) {
        lock_guard<mutex> lg(message_lock);
        auto message = messages.find(message_ref.GetID()); 
        if(message != messages.end()) {
            return get<1>(message->second);
        }

        return static_cast<OnlineMessageType>(-1);
    }

    void* GetMessageData(const OnlineMessageRef& message_ref) {
        message_lock.lock();
        auto message = messages.find(message_ref.GetID()); 
        message_lock.unlock();

        if(message != messages.end()) {
            return get<2>(message->second);
        } else {
            return nullptr;
        }
    }
};
