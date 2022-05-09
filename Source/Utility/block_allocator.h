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

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdint>
#include <cstring>

class BlockAlloctorThreadSafe {
   private:
    struct DebugAllocationData {
        uint32_t line;
        const char* filename;
        uint32_t allocationSize;
        bool watched;
        bool taken;

        friend std::ostream& operator<<(std::ostream& os, const DebugAllocationData& data) {
            os << data.filename << "\t: " << data.line << " size: " << data.allocationSize;

            return os;
        }
    };
    uint32_t size;
    uint16_t blockSize;
    uint32_t nrOfBlocks;
    uint32_t nr_of_allocations;
    DebugAllocationData* allocations;  // tracks allocations from block number in size. Rounded to blocksize
    bool* bitmap;                      // tracks free blocks
    std::mutex bitmapLock;
    uint8_t* pool;

   public:
    BlockAlloctorThreadSafe(uint32_t size = 4096, uint16_t blockSize = 256);  // size is initalize size of the avaiable pool of bytes
    ~BlockAlloctorThreadSafe();

    uint32_t GetSize() const;
    uint32_t GetNrOfBlocks() const;

    // uint8_t * Allocate(uint32_t size);

    uint8_t* Allocate(uint32_t size, uint32_t line, const char* file, bool watched = false);

    template <class T>
    void DeAllocate(T* addr);

    template <class T>
    void DeAllocate(T* addr, uint32_t line, const char* file);

    template <class T>
    T* ReAllocate(T* addr, uint32_t size);

   private:
    int32_t FindFirstFreeBlock(uint32_t size);
};

template <class T>
void BlockAlloctorThreadSafe::DeAllocate(T* addr) {
    std::lock_guard<std::mutex> guard(bitmapLock);  // we can probably do without this in overgrowth.

    nr_of_allocations--;

    int32_t bytesFromStart = (int32_t)((uint8_t*)addr - pool);  // we need to do some magic to make sure it's on block start.

    if (bytesFromStart > size || bytesFromStart < 0) {
        std::cout << "YOU DE-ALLOCATED AN ADDRES THAT WAS NEVER ALLOCATED" << std::endl;
        return;
    }

    uint32_t startOfBlock = bytesFromStart / blockSize;  // maybe warning here instead of solving a potential user error

    if (allocations[startOfBlock].watched) {
        std::cout << " deallocated from: " << allocations[startOfBlock].filename << " line: " << allocations[startOfBlock].line << std::endl;
    }

    if (allocations[startOfBlock].taken) {
        for (uint32_t i = 0; i < allocations[startOfBlock].allocationSize / blockSize; i++)
            bitmap[((i * blockSize) + bytesFromStart) / blockSize] = false;

        allocations[startOfBlock].taken = false;
    }
}

template <class T>
void BlockAlloctorThreadSafe::DeAllocate(T* addr, uint32_t line, const char* file) {
    std::lock_guard<std::mutex> guard(bitmapLock);              // we can probably do without this in overgrowth.
    int32_t bytesFromStart = (int32_t)((uint8_t*)addr - pool);  // we need to do some magic to make sure it's on block start.

    if (bytesFromStart > size || bytesFromStart < 0) {
        std::cout << "Deallocated line: " << line << " in file: " << file << " which where never allocated." << std::endl;
        return;
    }

    nr_of_allocations--;

    uint32_t startOfBlock = bytesFromStart / blockSize;  // maybe warning here instead of solving a potential user error

    if (allocations[startOfBlock].taken) {
        DebugAllocationData temp = allocations[startOfBlock];

        if (temp.watched) {
            std::cout << " allocated from: " << temp.filename << " line: " << temp.line << " deallocated from: " << file << " on line " << line << std::endl;
        }

        for (uint32_t i = 0; i < allocations[startOfBlock].allocationSize / blockSize; i++)
            bitmap[((i * blockSize) + bytesFromStart) / blockSize] = false;

        allocations[startOfBlock].taken = false;
    } else {
        std::cout << "No allocation where deallocated" << std::endl;
    }
}

template <class T>
T* BlockAlloctorThreadSafe::ReAllocate(T* addr, uint32_t size) {
    DeAllocate(addr);

    return Allocate(addr, size);
}
