//-----------------------------------------------------------------------------
//           Name: hash.cpp
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
#include "hash.h"

#include <Internal/filesystem.h>

#include <murmurhash3/MurmurHash3.h>

#include <sstream>
#include <string>
#include <iomanip>
#include <cstring>

std::string MurmurHash::ToString() {
    std::stringstream ss;

    ss << std::hex << hash[0] << hash[1];

    return ss.str();
}

MurmurHash GetFileHash(const char* file) {
    std::vector<unsigned char> data = readFile(file);

    MurmurHash hash;
    memset(&hash, 0, sizeof(MurmurHash));

    if (data.size() == 0) {
        return hash;
    }

    MurmurHash3_x86_128(&data[0], data.size(), 1337, hash.hash);

    return hash;
}
