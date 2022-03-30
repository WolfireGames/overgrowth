//-----------------------------------------------------------------------------
//           Name: serialize.cpp
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
#include "serialize.h"

#include <Internal/integer.h>

#include <cstring>

char to_hex( uint8_t v ) {
    if( v <= 9 ) {
        return '0' + v;
    } else if( v < 16 ) {
        return 'A' + v - 10;
    } else {
        return 'x';
    }
}

uint8_t from_hex(char v) {
    if( v >= '0' && v <= '9' ) {
        return v - '0';
    } else if( v >= 'A' && v <= 'F' ) {
        return v - 'A' + 10;
    } else {
        return 0;
    }
}

int flags_to_string(char* dest, uint32_t val) {
    dest[0] = to_hex( (val & 0xF0000000) >> 28);
    dest[1] = to_hex( (val & 0x0F000000) >> 24);
    dest[2] = to_hex( (val & 0x00F00000) >> 20);
    dest[3] = to_hex( (val & 0x000F0000) >> 16);
    dest[4] = to_hex( (val & 0x0000F000) >> 12);
    dest[5] = to_hex( (val & 0x00000F00) >> 8);
    dest[6] = to_hex( (val & 0x000000F0) >> 4);
    dest[7] = to_hex( (val & 0x0000000F) >> 0);
    dest[8] = '\0';
    return 0;
}


int string_flags_to_uint32(uint32_t* dest, const char* source ) {
    if( strlen( source ) >= 8 ) {
        *dest = 0x0;
        *dest |= ((uint32_t)from_hex(source[0])) << 28;
        *dest |= ((uint32_t)from_hex(source[1])) << 24;
        *dest |= ((uint32_t)from_hex(source[2])) << 20;
        *dest |= ((uint32_t)from_hex(source[3])) << 16;
        *dest |= ((uint32_t)from_hex(source[4])) << 12;
        *dest |= ((uint32_t)from_hex(source[5])) << 8;
        *dest |= ((uint32_t)from_hex(source[6])) << 4;
        *dest |= ((uint32_t)from_hex(source[7])) << 0;
        return 0;
    } else {
        return -1;
    }
}
