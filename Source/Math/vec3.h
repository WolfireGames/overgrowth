//-----------------------------------------------------------------------------
//           Name: vec3.h
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

#include <Math/vec2.h>

#include <cassert>
#include <iostream>

class vec3 {
   public:
    float entries[3];

    inline explicit vec3(float val = 0.0f) {
        entries[0] = val;
        entries[1] = val;
        entries[2] = val;
    }

    inline vec3(float newx, float newy, float newz) {
        entries[0] = newx;
        entries[1] = newy;
        entries[2] = newz;
    }

    inline vec3(const vec2& vec, float newz) {
        entries[0] = vec[0];
        entries[1] = vec[1];
        entries[2] = newz;
    }

    inline vec3& operator=(float param) {
        entries[0] = param;
        entries[1] = param;
        entries[2] = param;
        return *this;
    }

    inline vec3& operator=(const vec3& param) {
        entries[0] = param.entries[0];
        entries[1] = param.entries[1];
        entries[2] = param.entries[2];
        return *this;
    }

    inline float& operator[](const int which) {
        assert(which < 3);
        return entries[which];
    }

    inline const float& operator[](const int which) const {
        return entries[which];
    }

    inline const float& x() const { return entries[0]; }
    inline const float& y() const { return entries[1]; }
    inline const float& z() const { return entries[2]; }
    inline float& x() { return entries[0]; }
    inline float& y() { return entries[1]; }
    inline float& z() { return entries[2]; }
    inline const float& r() const { return entries[0]; }
    inline const float& g() const { return entries[1]; }
    inline const float& b() const { return entries[2]; }
    inline float& r() { return entries[0]; }
    inline float& g() { return entries[1]; }
    inline float& b() { return entries[2]; }

    inline vec2 xy() const {
        return vec2(entries[0], entries[1]);
    }

    inline float* operator*() { return entries; }
};

inline std::ostream& operator<<(std::ostream& os, const vec3& vec) {
    os << "vec3(" << vec[0] << ", " << vec[1] << ", " << vec[2] << ")";
    return os;
}
