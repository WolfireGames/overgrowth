//-----------------------------------------------------------------------------
//           Name: assetmanagerthreadhandler.cpp
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

#include "assetmanagerthreadhandler.h"

#include <Memory/allocation.h>
#include <Asset/assetloaderrors.h>

#include <cstdlib>

AssetLoaderThreadedInstance::AssetLoaderThreadedInstance() : loader(NULL), load_id(0), step(0), return_state(kLoadOk) {
}

AssetLoaderThreadedInstance::AssetLoaderThreadedInstance(AssetLoaderBase* loader, uint32_t load_id, int step) : loader(loader), load_id(load_id), step(step), return_state(kLoadOk) {
}

AssetManagerThreadedInstanceQueue::AssetManagerThreadedInstanceQueue() : queue(NULL), queue_count(0), queue_size(0) {
    accessmutex = new std::mutex();
}

AssetManagerThreadedInstanceQueue::~AssetManagerThreadedInstanceQueue() {
    OG_FREE(queue);
    queue = NULL;
    queue_count = 0;
    queue_size = 0;

    delete accessmutex;
    accessmutex = NULL;
}

AssetLoaderThreadedInstance AssetManagerThreadedInstanceQueue::Pop() {
    AssetLoaderThreadedInstance instance;
    accessmutex->lock();
    if (queue_count > 0) {
        instance = queue[0];
        for (unsigned i = 1; i < queue_count; i++) {
            queue[i - 1] = queue[i];
        }
        queue_count -= 1;
    }
    accessmutex->unlock();

    return instance;
}

void AssetManagerThreadedInstanceQueue::Push(const AssetLoaderThreadedInstance& input) {
    accessmutex->lock();

    if (queue_size >= queue_count) {
        queue_size += 32;
        queue = static_cast<AssetLoaderThreadedInstance*>(realloc(queue, sizeof(AssetLoaderThreadedInstance) * queue_size));
    }

    queue[queue_count] = input;
    queue_count += 1;

    accessmutex->unlock();
}

size_t AssetManagerThreadedInstanceQueue::Count() {
    size_t ret;
    accessmutex->lock();
    ret = queue_count;
    accessmutex->unlock();
    return ret;
}

AssetManagerThreadInstance::AssetManagerThreadInstance(AssetManagerThreadedInstanceQueue* queue_in, AssetManagerThreadedInstanceQueue* queue_out, bool* stop) : queue_in(queue_in), queue_out(queue_out), stop(stop) {
}

void AssetManagerThreadHandler_Operate(void* userdata) {
    AssetManagerThreadInstance* data = static_cast<AssetManagerThreadInstance*>(userdata);
    while (*(data->stop) == false) {
        if (data->queue_in->Count() > 0) {
            AssetLoaderThreadedInstance instance = data->queue_in->Pop();

            instance.return_state = instance.loader->DoLoadStep(instance.step);

            data->queue_out->Push(instance);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    delete data;
}

AssetManagerThreadHandler::AssetManagerThreadHandler(int thread_count) : stop(false) {
    for (int i = 0; i < thread_count; i++) {
        AssetManagerThreadInstance* threadInstance = new AssetManagerThreadInstance(&queue_in, &queue_out, &stop);
        std::thread* thread = new std::thread(AssetManagerThreadHandler_Operate, threadInstance);
        threads.push_back(thread);
    }
}

AssetManagerThreadHandler::~AssetManagerThreadHandler() {
    stop = true;

    for (auto& thread : threads) {
        thread->join();
        thread = NULL;
    }
}
