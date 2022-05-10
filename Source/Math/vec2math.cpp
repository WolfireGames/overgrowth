//-----------------------------------------------------------------------------
//           Name: vec2math.cpp
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
#include "vec2math.h"

#include <cmath>

float length(const vec2 &vec) {
    return sqrtf(length_squared(vec));
}

vec2 components_min2(const vec2 &a, const vec2 &b) {
    vec2 result;

    result.x() = (a.x() < b.x()) ? a.x() : b.x();
    result.y() = (a.y() < b.y()) ? a.y() : b.y();

    return result;
}

vec2 components_max2(const vec2 &a, const vec2 &b) {
    vec2 result;

    result.x() = (a.x() > b.x()) ? a.x() : b.x();
    result.y() = (a.y() > b.y()) ? a.y() : b.y();

    return result;
}
