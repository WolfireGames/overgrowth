//-----------------------------------------------------------------------------
//           Name: vec3math.h
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

inline vec3& operator+=(vec3 &vec, const vec3 & param) {
    vec.entries[0] += param.entries[0];
    vec.entries[1] += param.entries[1];
    vec.entries[2] += param.entries[2];
    return vec;
}

inline vec3& operator-=(vec3 &vec, const vec3 & param) {
    vec.entries[0] -= param.entries[0];
    vec.entries[1] -= param.entries[1];
    vec.entries[2] -= param.entries[2];
    return vec;
}

inline vec3& operator*=(vec3 &vec, float param) {
    vec.entries[0] *= param;
    vec.entries[1] *= param;
    vec.entries[2] *= param;
    return vec;
}

inline vec3& operator*=(vec3 &vec, const vec3 & param) {
    vec.entries[0] *= param.entries[0];
    vec.entries[1] *= param.entries[1];
    vec.entries[2] *= param.entries[2];
    return vec;
}

inline vec3& operator/=(vec3 &vec, float param) {
    vec.entries[0] /= param;
    vec.entries[1] /= param;
    vec.entries[2] /= param;
    return vec;
}

inline bool operator!=(const vec3 &vec, const vec3 &param) {
    return ( vec.entries[0] != param.entries[0] ||
             vec.entries[1] != param.entries[1] ||
             vec.entries[2] != param.entries[2] );
}

inline bool operator!=(const vec3& vec, float param) {
    return ( vec.entries[0] != param || 
             vec.entries[1] != param || 
             vec.entries[2] != param );
}

inline vec3 operator+( const vec3 &vec_a, const vec3 &vec_b ) {
    vec3 vec(vec_a);
    vec += vec_b;
    return vec;
}

inline vec3 operator-( const vec3 &vec_a, const vec3 &vec_b ) {
    vec3 vec(vec_a);
    vec -= vec_b;
    return vec;
}

inline vec3 operator*( const vec3 &vec_a, const float param ) {
    vec3 vec(vec_a);
    vec *= param;
    return vec;
}

inline vec3 operator/( const vec3 &vec_a, const float param ) {
    vec3 vec(vec_a);
    vec /= param;
    return vec;
}

inline vec3 operator/( const vec3 &vec_a, const vec3 &vec_b ) {
    return vec3(vec_a.entries[0]/vec_b.entries[0],
                vec_a.entries[1]/vec_b.entries[1],
                vec_a.entries[2]/vec_b.entries[2]);
}

inline float length_squared(const vec3 &vec) {
    return vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
}

float length(const vec3 &vec);

inline float xz_length_squared(const vec3 &vec) {
    return vec[0]*vec[0] + vec[2]*vec[2];
}

float xz_length(const vec3 &vec);

inline float dot(const vec3 &vec_a, const vec3 &vec_b) {
    return vec_a[0]*vec_b[0] + vec_a[1]*vec_b[1] + vec_a[2]*vec_b[2];
}

inline float distance(const vec3 &vec_a, const vec3 &vec_b) {
    return length(vec_a-vec_b);
}

inline float distance_squared(const vec3 &vec_a, const vec3 &vec_b) {
    return length_squared(vec_a-vec_b);
}

inline float xz_distance(const vec3 &vec_a, const vec3 &vec_b) {
    return xz_length(vec_a-vec_b);
}

inline float xz_distance_squared(const vec3 &vec_a, const vec3 &vec_b) {
    return xz_length_squared(vec_a-vec_b);
}

vec3 normalize(const vec3 &vec);


// TODO: these should be templated
inline vec3 components_min(const vec3 &a, const vec3 &b) {
    vec3 result;

    result.x() = (a.x() < b.x()) ? a.x() : b.x();
    result.y() = (a.y() < b.y()) ? a.y() : b.y();
    result.z() = (a.z() < b.z()) ? a.z() : b.z();

    return result;
}


inline vec3 components_max(const vec3 &a, const vec3 &b) {
    vec3 result;

    result.x() = (a.x() > b.x()) ? a.x() : b.x();
    result.y() = (a.y() > b.y()) ? a.y() : b.y();
    result.z() = (a.z() > b.z()) ? a.z() : b.z();

    return result;
}


inline vec3 cross(const vec3 &vec_a, const vec3 &vec_b) {
    vec3 result;
    result[0] = vec_a[1] * vec_b[2] - vec_a[2] * vec_b[1];
    result[1] = vec_a[2] * vec_b[0] - vec_a[0] * vec_b[2];
    result[2] = vec_a[0] * vec_b[1] - vec_a[1] * vec_b[0];
    return result;
}

inline vec3 operator*(const vec3 &vec_a, const vec3 &vec_b) {
    return vec3(vec_a[0]*vec_b[0],
                vec_a[1]*vec_b[1],
                vec_a[2]*vec_b[2]);
}

inline vec3 reflect(const vec3 &vec, const vec3 &normal) {
    return vec - normal*(2.0f*dot(vec, normal));
}

inline vec3 operator-(const vec3 &vec) {
    return vec * -1.0f;
}

inline vec3 operator*(float param, const vec3 &vec_b) {
    return vec_b*param;
}

inline bool operator==( const vec3 &vec_a, const vec3 &vec_b ) {
    return (vec_a.entries[0]==vec_b.entries[0] &&
            vec_a.entries[1]==vec_b.entries[1] &&
            vec_a.entries[2]==vec_b.entries[2]);
}
inline bool operator<( const vec3 &a, const vec3 &b ) {
    return a.entries[0] <  b.entries[0] ||
          (a.entries[0] == b.entries[0] &&
          (a.entries[1] <  b.entries[1] ||
          (a.entries[1] == b.entries[1] && 
           a.entries[2] <  b.entries[2])));
}

inline vec3 lerp(vec3 v0, vec3 v1, float t) {
    return (1-t)*v0 + t*v1;
}

