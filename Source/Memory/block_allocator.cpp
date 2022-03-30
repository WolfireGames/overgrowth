//-----------------------------------------------------------------------------
//           Name: block_allocator.cpp
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
#include "block_allocator.h"

#include <Memory/allocation.h>
#include <Internal/integer.h>
#include <Logging/logdata.h>
#include <Threading/thread_sanity.h>

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <climits>

using std::endl;
using std::vector;

BlockAllocator::BlockAllocator() : base_mem(NULL), block_count(0), blocksize(0), allocations(NULL), allocations_placements(0), no_log(false)
{

}

void BlockAllocator::Init( void* mem, size_t _blocks, size_t _blocksize ) 
{
    base_mem = mem; 
    block_count = _blocks; 
    blocksize = _blocksize; 

    allocations_placements.ResizeAndReset( block_count );
    allocations = new BlockAllocation[_blocks];
}

BlockAllocator::~BlockAllocator()
{
    base_mem = NULL;
    delete[] allocations;
    allocations = NULL;
}

void* BlockAllocator::Alloc(size_t size)
{
    //AssertMainThread();
    size_t slots = size/blocksize+(size%blocksize?1:0);
    int start_pos = allocations_placements.GetFirstFreeSlot(slots);
    
    if( start_pos >= 0 )
    {
        allocations_placements.SetBits(start_pos,slots);
        void* ptr = ((char*)base_mem) + (size_t)(blocksize*start_pos);
        for( size_t i = 0; i < block_count; i++ )
        {
            if( allocations[i].IsValid() == false )
            {
                allocations[i] = BlockAllocation(start_pos,slots,ptr);
                return ptr;
            }
        }

        if(!no_log){
            LOGW << "Out of block allocator memory, returning NULL." << endl;
        }
        return NULL;
    }
    else
    {
        if(!no_log){
            LOGW << "Out of block allocator memory slots (" << start_pos << "), falling back to normal heap." << endl;
        }

        void * ptr = OG_MALLOC( size ); 
        if( ptr != NULL )
        {
            backup.push_back(ptr);
            return ptr;
        }
        else
        {
            if(!no_log){
                LOGE << "Unable to allocate with OG_MALLOC()" << endl;
            }
        }
        
        return NULL;
    }
}

bool BlockAllocator::CanAlloc(size_t size)
{
    //AssertMainThread();
    size_t slots = size/blocksize+(size%blocksize?1:0);
    int start_pos = allocations_placements.GetFirstFreeSlot(slots);
    
    if( start_pos >= 0 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void BlockAllocator::Free(void* ptr)
{
    //AssertMainThread();
    for( size_t i = 0; i < block_count; i++ )
    {
        if( allocations[i].ptr == ptr )
        {
            allocations_placements.FreeBits(allocations[i].block_index, allocations[i].block_count);
            allocations[i] = BlockAllocation();
            return;
        } 
    }

    vector<void*>::iterator backupit;
    for( backupit = backup.begin(); backupit != backup.end(); backupit++ )
    {
        if( *backupit == ptr )
        {
            OG_FREE( ptr );
            backup.erase(backupit);
            return;
        } 
    }

    if(!no_log){
        LOGF << "Could not find an allocation for " << ptr << " unable to free." << endl;
    }
}
