//-----------------------------------------------------------------------------
//           Name: vec2.h
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

#include <Math/ivec2.h>

#include <iostream>

class vec2 {
   public:
    float entries[2];

    inline explicit vec2(float val = 0.0f) {
        entries[0] = val;
        entries[1] = val;
    }

    inline vec2(float newx, float newy) {
        entries[0] = newx;
        entries[1] = newy;
    }

    inline vec2(int newx, int newy) {
        entries[0] = (float)newx;
        entries[1] = (float)newy;
    }

    inline vec2(const ivec2& d) {
        entries[0] = (float)d[0];
        entries[1] = (float)d[1];
    }

    inline float& operator[](const int which) {
        return entries[which];
    }

    inline const float& operator[](const int which) const {
        return entries[which];
    }

    inline vec2& operator=(float param) {
        entries[0] = param;
        entries[1] = param;
        return *this;
    }

    inline vec2& operator=(const vec2& param) {
        entries[0] = param.entries[0];
        entries[1] = param.entries[1];
        return *this;
    }

    inline vec2 operator/(const vec2& param) {
        vec2 v;
        v[0] = entries[0] / param.entries[0];
        v[1] = entries[1] / param.entries[1];
        return v;
    }

    inline vec2 operator+(const vec2& param) {
        vec2 v;
        v[0] = entries[0] + param.entries[0];
        v[1] = entries[1] + param.entries[1];
        return v;
    }

    inline vec2 operator-(const vec2& param) {
        vec2 v;
        v[0] = entries[0] - param.entries[0];
        v[1] = entries[1] - param.entries[1];
        return v;
    }

    inline const float& x() const { return entries[0]; }
    inline const float& y() const { return entries[1]; }
    inline float& x() { return entries[0]; }
    inline float& y() { return entries[1]; }
};

inline std::ostream& operator<<(std::ostream& os, const vec2& v) {
    os << "vec2(" << v[0] << "," << v[1] << ")";
    return os;
}
