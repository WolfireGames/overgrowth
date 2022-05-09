//-----------------------------------------------------------------------------
//           Name: block_allocator.h
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

#include <Memory/blockallocation.h>
#include <Memory/bitarray.h>

#include <Utility/disallow_copy_and_assign.h>
#include <Internal/integer.h>

#include <cstring>
#include <vector>

class BlockAllocator {
   public:
    BlockAllocator();
    ~BlockAllocator();

    void Init(void* mem, size_t _blocks, size_t _blocksize);
    void* Alloc(size_t size);
    bool CanAlloc(size_t size);
    void Free(void* ptr);

    bool no_log;

   private:
    DISALLOW_COPY_AND_ASSIGN(BlockAllocator);
    void* base_mem;

    size_t block_count;
    size_t blocksize;

    std::vector<void*> backup;  // Normal malloc allocations that are either too big or don't fit to ensure continued execution.

    BlockAllocation* allocations;
    Bitarray allocations_placements;
};
