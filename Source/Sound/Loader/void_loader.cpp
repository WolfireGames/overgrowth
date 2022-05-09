//-----------------------------------------------------------------------------
//           Name: void_loader.cpp
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
#include "void_loader.h"

#include <cstring>

voidLoader::voidLoader() {
}

voidLoader::~voidLoader() {
}

int voidLoader::stream_buffer_int16(char* buffer, int size) {
    ::memset(buffer, 0, size);
    return size;
}

unsigned long voidLoader::get_sample_count() {
    return 22000;
}

unsigned long voidLoader::get_channels() {
    return 2;
}

int voidLoader::get_sample_rate() {
    return 44100;
}

int voidLoader::rewind() {
    return 0;
}

bool voidLoader::is_at_end() {
    return true;
}

int64_t voidLoader::get_pcm_pos() {
    return 0;
}

void voidLoader::set_pcm_pos(int64_t pos) {
}
