//-----------------------------------------------------------------------------
//           Name: stack_allocator.h
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
#include <Internal/integer.h>

#include <cstdlib>

class StackAllocator {
public:
    void Init(void* mem, size_t size);
    void Dispose();
    void* Alloc(size_t size);
    void Free(void* ptr);
    size_t TopBlockSize();
    uint64_t AllocatedAmountMemory();
    void* mem;

    uint32_t GetCurrentAllocations();
    uint32_t GetAndResetAllocationCount();

private:
    uint32_t frame_allocation_count;
    uint32_t current_allocation_count; 
    static const int kMaxBlocks = 100;
    uintptr_t stack_block_pts[kMaxBlocks];
    int stack_blocks;
    size_t size;
};
