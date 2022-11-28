//-----------------------------------------------------------------------------
//           Name: bitarray.cpp
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
#include "bitarray.h"

#include <Memory/allocation.h>
#include <Internal/filesystem.h>
#include <Logging/logdata.h>

#include <cstdlib>

using std::endl;

Bitarray::Bitarray(size_t _size) : size(_size), arr(NULL) {
    ResizeAndReset(size);
}

Bitarray::~Bitarray() {
    if (arr != NULL) {
        OG_FREE(arr);
        arr = NULL;
    }
}

void Bitarray::ResizeAndReset(size_t _size) {
    if (arr) {
        OG_FREE(arr);
    }

    size = _size;

    if (size > 0) {
        size_t arr_size = size / 64 + (size % 64 ? 1 : 0);
        arr = (uint64_t*)OG_MALLOC(sizeof(uint64_t) * arr_size);
        FreeBits(0, size);
        if (arr == NULL) {
            LOGF << "Unable to allocate memory for Bitarray" << endl;
        }
    } else {
        arr = NULL;
    }
}

bool Bitarray::GetBit(size_t index) {
    size_t p = index / 64;
    size_t i = index % 64;

    return (arr[p] & (1ULL << i));
}

void Bitarray::SetBit(size_t index) {
    size_t p = index / 64;
    size_t i = index % 64;

    arr[p] |= (1ULL << i);
}

void Bitarray::FreeBit(size_t index) {
    size_t p = index / 64;
    size_t i = index % 64;

    arr[p] &= ~(1ULL << i);
}

void Bitarray::SetBits(size_t index, size_t count) {
    for (size_t off = 0; off < count; off++) {
        SetBit(index + off);
    }
}

void Bitarray::FreeBits(size_t index, size_t count) {
    for (size_t off = 0; off < count; off++) {
        FreeBit(index + off);
    }
}

int Bitarray::GetFirstFreeSlot(size_t req_size) {
    size_t countdown = req_size;
    for (size_t i = 0; i < size; i++) {
        if (GetBit(i) == false) {
            countdown--;
        } else {
            countdown = req_size;
        }

        if (countdown == 0) {
            return i - (req_size - 1);
        }
    }

    return -1;
}
