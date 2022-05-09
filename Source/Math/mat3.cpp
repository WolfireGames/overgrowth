//-----------------------------------------------------------------------------
//           Name: mat3.cpp
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
#include "mat3.h"

#include <memory>
#include <cstring>

mat3::mat3(float val) {
    entries[0] = val;
    entries[4] = val;
    entries[8] = val;
    entries[3] = entries[2] = entries[1] = 0.0f;
    entries[7] = entries[6] = entries[5] = 0.0f;
}
mat3::mat3(float e0, float e1, float e2,
           float e3, float e4, float e5,
           float e6, float e7, float e8) {
    entries[0] = e0;
    entries[1] = e1;
    entries[2] = e2;
    entries[3] = e3;
    entries[4] = e4;
    entries[5] = e5;
    entries[6] = e6;
    entries[7] = e7;
    entries[8] = e8;
}
mat3::mat3(const float* rhs) {
    memcpy(entries, rhs, sizeof(entries));
}

mat3::mat3(const mat4& m) {
    entries[0] = m[0];
    entries[1] = m[1];
    entries[2] = m[2];
    entries[3] = m[4];
    entries[4] = m[5];
    entries[5] = m[6];
    entries[6] = m[8];
    entries[7] = m[9];
    entries[8] = m[10];
}

vec3 mat3::operator*(const vec3& rhs) const {
    return vec3(entries[0] * rhs[0] + entries[3] * rhs[1] + entries[6] * rhs[2],
                entries[1] * rhs[0] + entries[4] * rhs[1] + entries[7] * rhs[2],
                entries[2] * rhs[0] + entries[5] * rhs[1] + entries[8] * rhs[2]);
}
