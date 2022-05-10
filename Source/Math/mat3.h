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
#pragma once

#include <Math/vec3.h>
#include <Math/mat4.h>

class mat3 {
   public:
    mat3(float val = 1.0f);
    mat3(float e0, float e1, float e2,
         float e3, float e4, float e5,
         float e6, float e7, float e8);
    mat3(const float* rhs);
    mat3(const mat4& m);
    ~mat3() {}  // empty

    // cast to pointer to a (float *) for glGetFloatv etc
    operator float*() const { return (float*)this; }
    operator const float*() const { return (const float*)this; }

    inline float& operator()(int i, int j) { return entries[i + j * 3]; }
    inline const float& operator()(int i, int j) const { return entries[i + j * 3]; }

    inline float& operator[](int i) { return entries[i]; }
    inline const float& operator[](int i) const { return entries[i]; }
    vec3 operator*(const vec3& rhs) const;

    // member variables
    float entries[9];
};
