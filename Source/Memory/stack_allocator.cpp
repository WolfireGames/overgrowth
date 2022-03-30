//-----------------------------------------------------------------------------
//           Name: stack_allocator.cpp
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
#include "stack_allocator.h"

#include <Utility/assert.h>
#include <Utility/stacktrace.h>

#include <Logging/logdata.h>
#include <Threading/thread_sanity.h>
#include <Memory/allocation.h>

#ifndef NO_ERR
#include <Internal/error.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <stdint.h>

using std::endl;

void* StackAllocator::Alloc(size_t requested_size) {
    current_allocation_count++;
    frame_allocation_count++;
    //AssertMainThread();
#ifdef DEBUG_DISABLE_STACK_ALLOCATOR
	return OG_MALLOC(requested_size);
#else // DEBUG_DISABLE_STACK_ALLOCATOR
    if(stack_block_pts[stack_blocks] + requested_size < size && stack_blocks < kMaxBlocks-2){
        ++stack_blocks;
        stack_block_pts[stack_blocks] = stack_block_pts[stack_blocks-1] + requested_size;
        void* ptr = (void*)((uintptr_t)mem + stack_block_pts[stack_blocks-1]);
        return ptr;
    } else {
        if( stack_block_pts[stack_blocks] + requested_size >= size )
        {
            LOGF << "Unable to stack allocate memory size:" << requested_size << ". Out of memory." << endl;
        }
        else
        {
            LOGF << "Unable to stack allocate memory size:" << requested_size << ". Out of blocks." << endl;
        }
        
        LOGF << GenerateStacktrace() << endl;
        return NULL;
    }
#endif // DEBUG_DISABLE_STACK_ALLOCATOR
}

void StackAllocator::Free(void* ptr) {
    current_allocation_count--;
    //AssertMainThread();
#ifdef DEBUG_DISABLE_STACK_ALLOCATOR
	OG_FREE(ptr);
#else // DEBUG_DISABLE_STACK_ALLOCATOR
    if(stack_blocks){
        --stack_blocks;
        LOG_ASSERT(ptr == (void*)((uintptr_t)mem + stack_block_pts[stack_blocks]));
        if( ptr != (void*)((uintptr_t)mem + stack_block_pts[stack_blocks]) )
        {
            LOGE << "Called fre on something that isn't at the top of the stack" << endl;
            LOGE << "Stacktrace:" << endl << GenerateStacktrace() << endl; 
        }
        
    } else {
#ifndef NO_ERR
        FatalError("Memory stack underflow", "Calling Free() on StackMemoryBlock with no stack elements");
#endif
    }
#endif // DEBUG_DISABLE_STACK_ALLOCATOR
}


uintptr_t StackAllocator::TopBlockSize() {
    switch(stack_blocks){
    case 0:
        return 0;
    default:
        return stack_block_pts[stack_blocks] - stack_block_pts[stack_blocks-1];
    }
}

void StackAllocator::Init(void* p_mem, size_t p_size) {
    for (int i = 0; i < kMaxBlocks; i++) {
        stack_block_pts[i] = 0;
    }
    mem = p_mem;
    size = p_size;
    stack_blocks = 0;
}

void StackAllocator::Dispose() {
    for (int i = 0; i < kMaxBlocks; i++) {
        stack_block_pts[i] = 0;
    }
    mem = 0;
    size = 0;
    stack_blocks = 0;
}

uint64_t StackAllocator::AllocatedAmountMemory() {
    return stack_block_pts[stack_blocks];
}

uint32_t StackAllocator::GetCurrentAllocations() {
    return current_allocation_count;
}

uint32_t StackAllocator::GetAndResetAllocationCount() {
    uint32_t v = frame_allocation_count;
    frame_allocation_count = 0;
    return v;
}
