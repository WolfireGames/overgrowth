//-----------------------------------------------------------------------------
//           Name: vec3math.cpp
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
#include "vec3math.h"

#include <Math/vec3.h>

#include <cmath>

float length( const vec3 &vec ) {
    return sqrtf(length_squared(vec));
}

float xz_length( const vec3 &vec ) {
    return sqrtf(xz_length_squared(vec));
}

vec3 normalize(const vec3 &vec) {
    float length_squared_val = length_squared(vec);
    if(length_squared_val == 0.0f){
        return vec3(0.0f);
    }
    return vec/sqrtf(length_squared_val);
}
