//-----------------------------------------------------------------------------
//           Name: allocation.h
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

#include <Memory/stack_allocator.h>
#include <Memory/block_allocator.h>

#include <Internal/integer.h>

#include <cstdlib>

#define OG_MALLOC_GEN 0
#define OG_MALLOC_NEW 1
#define OG_MALLOC_NEW_ARR 2
#define OG_MALLOC_RC 3
#define OG_MALLOC_DT 4

const char* OgMallocTypeString(uint8_t type);

#define OG_MALLOC_TYPE_COUNT 5

#define OG_MALLOC(size) og_malloc(size, OG_MALLOC_GEN)
#define OG_FREE(pointer) og_free(pointer)

void* og_malloc(size_t size, uint8_t source_id);
void og_free(void* ptr);

#if MONITOR_MEMORY
uint32_t GetAndResetMallocAllocationCount();
uint64_t GetCurrentTotalMallocAllocation(int index);
size_t GetCurrentNumberOfMallocAllocations();
#endif

class Allocation {
   public:
    Allocation();

    void Init();
    void Dispose();

    StackAllocator stack;
    // BlockAllocator block;
    // ThreadSafeBlockAllocator ts_block;
   private:
    void* mem_block_stack_allocator;
    // void* mem_block_block_allocator;
};

class StackMem {
   private:
    void* v;
    StackMem(StackMem& other);

   public:
    StackMem(void* ptr);
    ~StackMem();
    void* ptr();
};

extern Allocation alloc;
