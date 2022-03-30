//-----------------------------------------------------------------------------
//           Name: common.h
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

#include <Utility/assert.h>

#include <cstdio>
#include <cassert>

float MoveTowards(float val, float target, float amount);

inline void VFormatString(char* buf, int buf_size, const char* fmt, va_list args) {
    int val = vsnprintf(buf, buf_size, fmt, args);
    if(val == -1 || val >= buf_size){
        buf[buf_size-1] = '\0';
        LOG_ASSERT(false); // Failed to format string
    }
}

void FormatString(char* buf, int buf_size, const char* fmt, ...);

int djb2_hash(unsigned char* str);
int djb2_hash_len(unsigned char* str, int len);
