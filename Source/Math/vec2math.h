//-----------------------------------------------------------------------------
//           Name: vec2math.h
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

inline vec2 operator+(const vec2 &vec, const vec2 &param) {
    return vec2(vec.entries[0] + param.entries[0],
                vec.entries[1] + param.entries[1]);
}

inline vec2 operator-(const vec2 &vec, const vec2 &param) {
    return vec2(vec.entries[0] - param.entries[0],
                vec.entries[1] - param.entries[1]);
}

inline vec2 operator*(const vec2 &vec, float param) {
    return vec2(vec.entries[0] * param,
                vec.entries[1] * param);
}

inline vec2 operator/(const vec2 &vec, float param) {
    return vec2(vec.entries[0] / param,
                vec.entries[1] / param);
}

inline vec2 &operator+=(vec2 &vec, const vec2 &param) {
    vec.entries[0] += param.entries[0];
    vec.entries[1] += param.entries[1];
    return vec;
}

inline vec2 &operator-=(vec2 &vec, const vec2 &param) {
    vec.entries[0] -= param.entries[0];
    vec.entries[1] -= param.entries[1];
    return vec;
}

inline vec2 &operator*=(vec2 &vec, float param) {
    vec.entries[0] *= param;
    vec.entries[1] *= param;
    return vec;
}

inline vec2 &operator*=(vec2 &vec, const vec2 &param) {
    vec.entries[0] *= param.entries[0];
    vec.entries[1] *= param.entries[1];
    return vec;
}

inline vec2 &operator/=(vec2 &vec, float param) {
    vec.entries[0] /= param;
    vec.entries[1] /= param;
    return vec;
}

inline bool operator==(const vec2 &vec, const vec2 &param) {
    return (vec.entries[0] == param.entries[0] &&
            vec.entries[1] == param.entries[1]);
}

inline bool operator!=(const vec2 &vec, const vec2 &param) {
    return (vec.entries[0] != param.entries[0] ||
            vec.entries[1] != param.entries[1]);
}

float length(const vec2 &vec);

// TODO: these should be templated
vec2 components_min2(const vec2 &a, const vec2 &b);
vec2 components_max2(const vec2 &a, const vec2 &b);

inline float length_squared(const vec2 &vec) {
    return vec[0] * vec[0] + vec[1] * vec[1];
}

inline float dot(const vec2 &vec_a, const vec2 &vec_b) {
    return vec_a[0] * vec_b[0] + vec_a[1] * vec_b[1];
}

inline float distance(const vec2 &vec_a, const vec2 &vec_b) {
    return length(vec_a - vec_b);
}

inline float distance_squared(const vec2 &vec_a, const vec2 &vec_b) {
    return length_squared(vec_a - vec_b);
}

inline vec2 normalize(const vec2 &vec) {
    if (vec[0] == 0.0f && vec[1] == 0.0f && vec[2] == 0.0f) {
        return vec2(0.0);
    }
    return vec / length(vec);
}

inline vec2 operator*(const vec2 &vec_a, const vec2 &vec_b) {
    return vec2(vec_a[0] * vec_b[0],
                vec_a[1] * vec_b[1]);
}

inline vec2 reflect(const vec2 &vec, const vec2 &normal) {
    return vec - normal * (2.0f * dot(vec, normal));
}

inline vec2 operator-(const vec2 &vec) {
    return vec * -1.0f;
}

inline vec2 operator*(float param, const vec2 &vec_b) {
    return vec_b * param;
}
