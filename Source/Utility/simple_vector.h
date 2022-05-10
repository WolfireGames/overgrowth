//-----------------------------------------------------------------------------
//           Name: simple_vector.h
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
#include <Internal/integer.h>
#include <Utility/assert.h>
#include <Memory/allocation.h>

#include <algorithm>
#include <vector>

/*
 * Very simple vector construct that doesn't call constructors or destructors on the memory allocated.
 */

static bool SortDescending(int a, int b) { return a > b; }

template <typename T>
class SimpleVector {
   private:
    T* v;
    uint32_t v_count;
    uint32_t v_size;
    uint32_t step;

   public:
    SimpleVector(uint32_t initial_size, uint32_t resize_step) : v(NULL), v_count(0), v_size(0), step(resize_step) {
        v = static_cast<T*>(realloc(v, initial_size * sizeof(T)));
    }

    ~SimpleVector() {
        OG_FREE(v);
        v = NULL;
        v_count = 0;
        v_size = 0;
    }

    uint32_t Allocate() {
        if (v_count >= v_size) {
            v_size += step;
            v = static_cast<T*>(realloc(v, v_size * sizeof(T)));
        }

        return v_count++;
    }

    void Deallocate(uint32_t index) {
        for (unsigned int i = index + 1; i < v_count; i++) {
            v[i - 1] = v[i];
        }
    }

    void Deallocate(std::vector<uint32_t> indexes) {
        std::sort(indexes.begin(), indexes.end(), SortDescending);

        for (unsigned int i = 0; i < indexes.size(); i++) {
            Deallocate(indexes[i]);
            if (i > 0) {
                LOG_ASSERT(indexes[i] < indexes[i - 1]);
            }
        }
    }

    size_t Count() {
        return v_count;
    }

    T& operator[](uint32_t index) {
        return v[index];
    }
};
