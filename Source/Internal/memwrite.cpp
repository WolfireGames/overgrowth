//-----------------------------------------------------------------------------
//           Name: memwrite.cpp
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
#include <Internal/memwrite.h>

#include <cstring>

void memwrite(const void *source, size_t size, size_t count, std::vector<char> &target) {
    int start = target.size();
    int total_size = size * count;
    target.resize(start + total_size);

    if (total_size > 0) {
        memcpy((void *)&target[start], source, total_size);
    }
}

void memread(void *source, size_t size, size_t count, const std::vector<char> &target, int &index) {
    int total_size = size * count;

    if (total_size > 0) {
        memcpy(source, (void *)&target[index], total_size);
    }
    index += total_size;
}

void memread(void *source, size_t size, size_t count, const std::vector<char> &target) {
    int total_size = size * count;
    if (total_size > 0) {
        memcpy(source, (void *)&target[0], total_size);
    }
}
