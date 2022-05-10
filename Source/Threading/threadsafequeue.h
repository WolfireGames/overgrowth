//-----------------------------------------------------------------------------
//           Name: threadsafequeue.h
//      Developer: Wolfire Games LLC
//    Description:
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

#include <queue>
#include <mutex>

template <class T>
class ThreadSafeQueue {
    std::queue<T> messages;
    std::mutex mutex;

   public:
    void Queue(const T &v) {
        mutex.lock();
        messages.push(v);
        mutex.unlock();
    }

    T Pop() {
        mutex.lock();
        T val = messages.front();
        messages.pop();
        mutex.unlock();

        return val;
    }

    size_t Count() {
        size_t c = 0;
        mutex.lock();
        c = messages.size();
        mutex.unlock();
        return c;
    }
};
