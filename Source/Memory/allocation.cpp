//-----------------------------------------------------------------------------
//           Name: allocation.cpp
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
#include "allocation.h"

#include <Internal/error.h>

#include <cstdlib>

#if MONITOR_MEMORY
// POSIX variant, win version necessary.
#include <pthread.h>
static pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

static uint64_t total_memory_allocations[OG_MALLOC_TYPE_COUNT] = {0};
static size_t allocations_count = 0;
static uint32_t allocations_this_frame = 0;

struct Allocations {
    void* ptr;
    size_t size;
    uint8_t source_id;
};

const int MAX_ALLOCATIONS = 1024 * 16;

Allocations allocations[MAX_ALLOCATIONS] = {NULL, 0};

const char* OgMallocTypeString(uint8_t type) {
    switch (type) {
        case OG_MALLOC_GEN:
            return "Generic";
        case OG_MALLOC_NEW:
            return "new";
        case OG_MALLOC_NEW_ARR:
            return "new []";
        case OG_MALLOC_RC:
            return "Recast";
        case OG_MALLOC_DT:
            return "Detour";
        default:
            return "Unknown";
    }
}

void* og_malloc(size_t size, uint8_t source_id) {
    if (source_id >= OG_MALLOC_TYPE_COUNT) {
        source_id = 0;
    }

    if (size > 100 * 1024 * 1024) {
        printf("Allocating more than 100 MiB of memory\n");
    }
    void* ptr = malloc(size);

    pthread_mutex_lock(&fastmutex);

    if (allocations_count < MAX_ALLOCATIONS) {
        int64_t alloc_index;
        alloc_index = allocations_count;

        while (allocations[alloc_index].ptr != NULL && alloc_index >= 0) {
            alloc_index--;
        }

        if (alloc_index >= 0 && alloc_index < MAX_ALLOCATIONS) {
            allocations[alloc_index].ptr = ptr;
            allocations[alloc_index].size = size;
            allocations[alloc_index].source_id = source_id;
            allocations_count++;

            total_memory_allocations[source_id] += size;
        }
    }

    allocations_this_frame += 1;
    pthread_mutex_unlock(&fastmutex);

    return ptr;
}

void og_free(void* ptr) {
    pthread_mutex_lock(&fastmutex);
    for (uint32_t i = 0; i < MAX_ALLOCATIONS; i++) {
        if (allocations[i].ptr == ptr) {
            total_memory_allocations[allocations[i].source_id] -= allocations[i].size;
            allocations[i].size = 0;
            allocations[i].ptr = NULL;
            allocations_count--;
            break;
        }
    }
    pthread_mutex_unlock(&fastmutex);

    free(ptr);
}

void* operator new(size_t size) { return og_malloc(size, OG_MALLOC_NEW); }
void* operator new[](size_t size) { return og_malloc(size, OG_MALLOC_NEW_ARR); }
void operator delete(void* ptr) { og_free(ptr); }
void operator delete[](void* ptr) { og_free(ptr); }

uint32_t GetAndResetMallocAllocationCount() {
    uint32_t v;
    pthread_mutex_lock(&fastmutex);
    v = allocations_this_frame;
    allocations_this_frame = 0;
    pthread_mutex_unlock(&fastmutex);
    return v;
}

uint64_t GetCurrentTotalMallocAllocation(int index) {
    return total_memory_allocations[index];
}

size_t GetCurrentNumberOfMallocAllocations() {
    return allocations_count;
}
#else
void* og_malloc(size_t size, uint8_t source_id) {
    return malloc(size);
}

void og_free(void* ptr) {
    free(ptr);
}
#endif

Allocation::Allocation() {
}

void Allocation::Init() {
    int mem_size = 150 * 1024 * 1024;

    mem_block_stack_allocator = malloc(mem_size);

#ifndef NO_ERR
    if (!mem_block_stack_allocator) {
        FatalError("Error", "Could not allocate initial memory block for stack allocator");
    }
#endif

    stack.Init(mem_block_stack_allocator, mem_size);

    /*
    int block_alloc_blocksize = 1024;
    int block_alloc_blockcount = 128 * 1024;
    int mem_size_block = block_alloc_blockcount * block_alloc_blocksize;

    mem_block_block_allocator = malloc(mem_size_block);

#ifndef NO_ERR
    if(!mem_block_block_allocator) {
        FatalError("Error", "Could not allocate initial memory block for block allocator");
    }
#endif

    block.Init(mem_block_block_allocator, block_alloc_blockcount, block_alloc_blocksize);
*/
}

void Allocation::Dispose() {
    stack.Dispose();

    free(mem_block_stack_allocator);
    mem_block_stack_allocator = NULL;
    //    free(mem_block_block_allocator);
}

StackMem::StackMem(void* ptr) : v(ptr) {
}

StackMem::~StackMem() {
    alloc.stack.Free(v);
}

void* StackMem::ptr() {
    return v;
}
