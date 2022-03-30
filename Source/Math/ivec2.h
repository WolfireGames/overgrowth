//-----------------------------------------------------------------------------
//           Name: ivec2.h
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

class ivec2{
public:
    int entries[2];

    inline ivec2(int val = 0) {
        entries[0] = val;
        entries[1] = val;
    }

    inline ivec2(int newx, int newy) {
        entries[0] = newx;
        entries[1] = newy;
    }

    inline int& operator[](const int which) {
        return entries[which];
    }

    inline const int& operator[](const int which) const {
        return entries[which];
    }

    inline ivec2& operator=(int param) {
        entries[0]=param;
        entries[1]=param;
        return *this;
    }

    inline ivec2& operator=(const ivec2 &param) {
        entries[0]=param.entries[0];
        entries[1]=param.entries[1];
        return *this;
    }
};

inline std::ostream& operator<<(std::ostream& os, const ivec2& v )
{
    os << "ivec2(" << v[0] << "," << v[1] << ")";
    return os;
}
