//-----------------------------------------------------------------------------
//           Name: assetmanagerthreadhandler.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <Internal/integer.h>
#include <Asset/assetloaderbase.h>

#include <thread>
#include <mutex>
#include <chrono>

struct AssetLoaderThreadedInstance {
    AssetLoaderThreadedInstance();
    AssetLoaderThreadedInstance(AssetLoaderBase* loader, uint32_t load_id, int step);

    AssetLoaderBase* loader;
    uint32_t load_id;
    int step;
    int return_state;
};

class AssetManagerThreadedInstanceQueue {
   public:
    AssetManagerThreadedInstanceQueue();
    ~AssetManagerThreadedInstanceQueue();

    AssetLoaderThreadedInstance Pop();
    void Push(const AssetLoaderThreadedInstance& input);
    size_t Count();

   private:
    std::mutex* accessmutex;

    AssetLoaderThreadedInstance* queue;
    size_t queue_count;
    size_t queue_size;
};

class AssetManagerThreadInstance {
   public:
    bool* stop;
    AssetManagerThreadedInstanceQueue* queue_in;
    AssetManagerThreadedInstanceQueue* queue_out;

    AssetManagerThreadInstance(AssetManagerThreadedInstanceQueue* queue_in, AssetManagerThreadedInstanceQueue* queue_out, bool* stop);
};

class AssetManagerThreadHandler {
   public:
    AssetManagerThreadedInstanceQueue queue_in;
    AssetManagerThreadedInstanceQueue queue_out;

    AssetManagerThreadHandler(int thread_count);
    ~AssetManagerThreadHandler();

   private:
    std::vector<std::thread*> threads;

    bool stop;
};

void AssetManagerThreadHandler_Operate(void* data);
