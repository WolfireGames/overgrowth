//-----------------------------------------------------------------------------
//           Name: block_allocator_tester.cpp
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

#include <Logging/logdata.h>
#include <Memory/block_allocator.h>
#include <Wrappers/tut.h>
#include <Math/enginemath.h>

#include <cmath>
#include <cstdlib>

#include <set>
#include <vector>

namespace tut {
struct BlockAllocatorTestData  //
{
};

typedef test_group<BlockAllocatorTestData> tg;
tg test_group_i("Block Allocation tests");

typedef tg::object block_allocation_test;

template <>
template <>
void block_allocation_test::test<1>() {
    const size_t block_count = 2048;

    size_t count = 128;
    size_t blocksize = sizeof(uint64_t) * count;

    void* mem = malloc(block_count * blocksize);

    ensure("Non null mem", mem != NULL);

    BlockAllocator ba;
    ba.Init(mem, block_count, blocksize);

    uint64_t* ptrs[block_count];

    for (auto& ptr : ptrs) {
        ptr = (uint64_t*)ba.Alloc(blocksize);

        ensure("NULL Return", ptr != NULL);
    }
    free(mem);
}

template <>
template <>
void block_allocation_test::test<2>() {
    const size_t block_count = 2048;

    size_t count = 128;
    size_t blocksize = sizeof(uint64_t) * count;

    void* mem = malloc(block_count * blocksize);

    BlockAllocator ba;
    ba.Init(mem, block_count, blocksize);

    uint64_t* ptrs[block_count];

    for (uint32_t i = 0; i < block_count; i++) {
        ptrs[i] = (uint64_t*)ba.Alloc(blocksize);

        if (i > 0) {
            ensure_equals("Expected Pointer Distance", (uintptr_t)ptrs[i] - blocksize, (uintptr_t)ptrs[i - 1]);
        }
    }
    free(mem);
}

template <>
template <>
void block_allocation_test::test<3>() {
    const size_t block_count = 2048;

    size_t count = 128;
    size_t blocksize = sizeof(uint64_t) * count;

    void* mem = malloc(block_count * blocksize);

    BlockAllocator ba;
    ba.Init(mem, block_count, blocksize);

    uint64_t* ptrs[block_count];

    for (uint32_t i = 0; i < block_count; i++) {
        ptrs[i] = (uint64_t*)ba.Alloc(blocksize);

        for (uint32_t k = 0; k < i; k++) {
            ensure("Non-repeating addresse", ptrs[k] != ptrs[i]);
        }
    }
    free(mem);
}

template <>
template <>
void block_allocation_test::test<4>() {
    const size_t block_count = 2048;

    size_t count = 128;
    size_t blocksize = sizeof(uint64_t) * count;

    void* mem = malloc(block_count * blocksize);

    BlockAllocator ba;
    ba.Init(mem, block_count, blocksize);

    uint64_t* ptrs[block_count];

    for (uint32_t i = 0; i < block_count; i++) {
        ptrs[i] = (uint64_t*)ba.Alloc(1024);

        for (uint32_t k = 0; k < count; k++) {
            ptrs[i][k] = i;
        }
    }

    for (uint32_t i = 0; i < block_count; i++) {
        for (uint32_t k = 0; k < count; k++) {
            ensure_equals("Linear non-overlap", ptrs[i][k], i);
        }
    }
    free(mem);
}

struct TestData {
    std::vector<uint8_t> ref_data;
    uint8_t* ptr;
    size_t count;

    bool VerifyData() {
        for (uint32_t i = 0; i < count; i++) {
            if (ptr[i] != ref_data[i]) {
                return false;
            }
        }
        return true;
    }

    void GenerateData() {
        ref_data.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            uint8_t v = RangedRandomInt(0, 255);
            ptr[i] = v;
            ref_data[i] = v;
        }
    }
};

template <>
template <>
void block_allocation_test::test<5>() {
    const size_t block_count = 2048;

    size_t count = 128;
    size_t blocksize = sizeof(uint64_t) * count;

    void* mem = malloc(block_count * blocksize);

    BlockAllocator ba;
    ba.Init(mem, block_count, blocksize);

    std::vector<TestData> ptrs;

    for (uint32_t attempt = 0; attempt < 32; attempt++) {
        while (true) {
            size_t size = sizeof(uint8_t) * RangedRandomInt(1, 2048 * 8);

            if (ba.CanAlloc(size)) {
                uint8_t* ptr = (uint8_t*)ba.Alloc(size);

                ensure("Not NULL", ptr != NULL);
                TestData t;

                t.ptr = ptr;
                t.count = size;
                t.GenerateData();

                ptrs.push_back(t);
            } else {
                break;
            }
        }

        uint32_t number_of_removes = RangedRandomInt(0, ptrs.size() - 1);

        for (uint32_t i = 0; i < number_of_removes; i++) {
            int index = RangedRandomInt(0, ptrs.size() - 1);

            std::vector<TestData>::iterator it = ptrs.begin() + index;

            ensure("Data Overlap Persistance", it->VerifyData());

            ba.Free(it->ptr);
            it->ptr = NULL;
            ptrs.erase(it);
        }
    }
    free(mem);
}
}  // namespace tut
