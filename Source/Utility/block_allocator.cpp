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

BlockAlloctorThreadSafe::BlockAlloctorThreadSafe(uint32_t size, uint16_t blockSize) {
    // register inside engine.cpp so that we can easily track all active allocators
    this->size = size;
    this->blockSize = blockSize;
    pool = new uint8_t[size];

    this->nrOfBlocks = size / blockSize;  // + 1 * ((size % blockSize) > 0); // this is really stupid
    allocations = new DebugAllocationData[nrOfBlocks];

    bitmap = new bool[nrOfBlocks];

    nr_of_allocations = 0;
}

BlockAlloctorThreadSafe::~BlockAlloctorThreadSafe() {
    delete bitmap;
    delete allocations;
    delete pool;
}

uint32_t BlockAlloctorThreadSafe::GetSize() const {
    return size;
}

uint32_t BlockAlloctorThreadSafe::GetNrOfBlocks() const {
    return nrOfBlocks;
}
/*
uint8_t * BlockAllactor::Allocate(uint32_t size) {
        std::lock_guard<std::mutex> guard(bitmapLock);
        uint32_t sizeOfAllocation = size;
        uint16_t nrOfBlocksNeeded = sizeOfAllocation / blockSize + (1 * ((sizeOfAllocation % blockSize) > 0));

        int32_t pos = FindFirstFreeBlock(nrOfBlocksNeeded);
        if (pos != -1) {
                DebugAllocationData debugData;
                debugData.allocationSize = nrOfBlocksNeeded * blockSize;

                allocations[pos] = debugData;// nrOfBlocksNeeded * blockSize;// +(blockSize % sizeOfAllocation);
                std::cout << (uint32_t)&pool[pos * blockSize] << std::endl;
                return &pool[pos * blockSize];
        }
        else {
                // This will lead to undefined behavior, but thats OK. We want to crash when this happens.
                std::cout << "Failed to get any memory" << std::endl;
                return nullptr;
        }
}
*/
uint8_t* BlockAlloctorThreadSafe::Allocate(uint32_t size, uint32_t line, const char* file, bool watched) {
    std::lock_guard<std::mutex> guard(bitmapLock);
    uint32_t sizeOfAllocation = size;
    uint16_t nrOfBlocksNeeded = sizeOfAllocation / blockSize + (1 * ((sizeOfAllocation % blockSize) > 0));

    int32_t pos = FindFirstFreeBlock(nrOfBlocksNeeded);
    if (pos != -1) {
        allocations[pos].taken = true;
        allocations[pos].filename = file;
        allocations[pos].line = line;
        allocations[pos].allocationSize = nrOfBlocksNeeded * blockSize;
        allocations[pos].watched = watched;

        // debugData.addr = (uint32_t)&pool[pos * blockSize];

        if (watched) {
            std::cout << "Allocted: "
                      << " size: " << size << " bytes - " << file << " on line:" << line << std::endl;
        }

        return &pool[pos * blockSize];
    } else {
        // This will lead to undefined behavior, but thats OK. We want to crash when this happens.
        std::cout << "Failed to get any memory - crash imminenet - dumping state" << std::endl;
        for (uint32_t i = 0; i < nrOfBlocks; i++) {
            if (allocations[i].taken) {
                std::cout << allocations[i] << std::endl;
            }
        }

        return nullptr;
    }
}

int32_t BlockAlloctorThreadSafe::FindFirstFreeBlock(uint32_t blocksNeeded) {
    uint32_t available = 0;  // socket thread will call allocate, main thread will call deallocate

    uint32_t startPos = 0;

    for (uint32_t i = 0; i < nrOfBlocks && available != blocksNeeded; i++) {
        if (bitmap[i] != true) {
            available++;
        } else {
            available = 0;
            startPos = i + 1;
        }
    }

    if (available == blocksNeeded) {
        for (uint32_t i = startPos; i < blocksNeeded + startPos && i < nrOfBlocks; i++) {
            bitmap[i] = true;  // this can happen in several threads at once. V bad
                               // std::cout << "allocated: " <<  i << std::endl;
        }
    } else {
        return -1;
    }

    return startPos;
}
