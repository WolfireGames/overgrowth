//-----------------------------------------------------------------------------
//           Name: vec4math.h
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

#include <Math/vec4.h>
#include <Math/vec3.h>
#include <Math/vec3math.h>

inline vec4 operator+(const vec4 &vec, const vec4 &param) {
    return vec4(vec.entries[0] + param.entries[0],
                vec.entries[1] + param.entries[1],
                vec.entries[2] + param.entries[2],
                vec.entries[3]);
}

inline vec4 operator-(const vec4 &vec, const vec4 &param) {
    return vec4(vec.entries[0] - param.entries[0],
                vec.entries[1] - param.entries[1],
                vec.entries[2] - param.entries[2],
                vec.entries[3]);
}

inline vec4 operator*(const vec4 &vec, float param) {
    return vec4(vec.entries[0] * param,
                vec.entries[1] * param,
                vec.entries[2] * param,
                vec.entries[3]);
}
inline vec4 operator/(const vec4 &vec, float param) {
    return vec4(vec.entries[0] / param,
                vec.entries[1] / param,
                vec.entries[2] / param,
                vec.entries[3]);
}

inline void operator+=(vec4 &vec, const vec4 &param) {
    vec.entries[0] += param.entries[0];
    vec.entries[1] += param.entries[1];
    vec.entries[2] += param.entries[2];
}

inline void operator-=(vec4 &vec, const vec4 &param) {
    vec.entries[0] -= param.entries[0];
    vec.entries[1] -= param.entries[1];
    vec.entries[2] -= param.entries[2];
}

inline void operator*=(vec4 &vec, float param) {
    vec.entries[0] *= param;
    vec.entries[1] *= param;
    vec.entries[2] *= param;
}

inline void operator*=(vec4 &vec, const vec4 &param) {
    vec.entries[0] *= param.entries[0];
    vec.entries[1] *= param.entries[1];
    vec.entries[2] *= param.entries[2];
}

inline void operator/=(vec4 &vec, float param) {
    vec.entries[0] /= param;
    vec.entries[1] /= param;
    vec.entries[2] /= param;
}

inline bool operator==(const vec4 &vec, const vec4 &param) {
    return (vec.entries[0] == param.entries[0] &&
            vec.entries[1] == param.entries[1] &&
            vec.entries[2] == param.entries[2] &&
            vec.entries[3] == param.entries[3]);
}

inline bool operator!=(const vec4 &vec, const vec4 &param) {
    return (vec.entries[0] != param.entries[0] ||
            vec.entries[1] != param.entries[1] ||
            vec.entries[2] != param.entries[2] ||
            vec.entries[3] != param.entries[3]);
}

float length(const vec4 &vec);

inline float length_squared(const vec4 &vec) {
    return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
}

inline float dot(const vec4 &vec_a, const vec4 &vec_b) {
    return vec_a[0] * vec_b[0] + vec_a[1] * vec_b[1] + vec_a[2] * vec_b[2];
}

inline float distance(const vec4 &vec_a, const vec4 &vec_b) {
    return length(vec_a - vec_b);
}

inline float distance_squared(const vec4 &vec_a, const vec4 &vec_b) {
    return length_squared(vec_a - vec_b);
}

inline vec4 normalize(const vec4 &vec) {
    return vec4(normalize(vec.xyz()), vec[3]);
}

inline vec4 cross(const vec4 &vec_a, const vec4 &vec_b) {
    vec4 result;
    result[0] = vec_a[1] * vec_b[2] - vec_a[2] * vec_b[1];
    result[1] = vec_a[2] * vec_b[0] - vec_a[0] * vec_b[2];
    result[2] = vec_a[0] * vec_b[1] - vec_a[1] * vec_b[0];
    return result;
}

inline vec4 reflect(const vec4 &vec, const vec4 &normal) {
    return vec - normal * (2.0f * dot(vec, normal));
}

inline vec4 operator-(const vec4 &vec) {
    return vec * -1.0f;
}

inline vec4 operator*(float param, const vec4 &vec_b) {
    return vec_b * param;
}

inline vec4 lerp(vec4 v0, vec4 v1, float t) {
    return (1 - t) * v0 + t * v1;
}
