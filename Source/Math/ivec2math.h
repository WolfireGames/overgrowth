//-----------------------------------------------------------------------------
//           Name: ivec2math.h
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

inline ivec2& operator+=(ivec2 &vec, const ivec2 & param) {
    vec.entries[0] += param.entries[0];
    vec.entries[1] += param.entries[1];
    return vec;
}

inline ivec2& operator-=(ivec2 &vec, const ivec2 & param) {
    vec.entries[0] -= param.entries[0];
    vec.entries[1] -= param.entries[1];
    return vec;
}

inline ivec2& operator*=(ivec2 &vec, int param) {
    vec.entries[0] *= param;
    vec.entries[1] *= param;
    return vec;
}

inline ivec2& operator*=(ivec2 &vec, const ivec2 & param) {
    vec.entries[0] *= param.entries[0];
    vec.entries[1] *= param.entries[1];
    return vec;
}

inline ivec2& operator/=(ivec2 &vec, int param) {
    vec.entries[0] /= param;
    vec.entries[1] /= param;
    return vec;
}

inline bool operator!=(const ivec2 &vec, const ivec2 &param) {
    return ( vec.entries[0] != param.entries[0] ||
             vec.entries[1] != param.entries[1]);
}

inline bool operator!=(const ivec2& vec, int param) {
    return ( vec.entries[0] != param || 
             vec.entries[1] != param);
}

inline ivec2 operator+( const ivec2 &vec_a, const ivec2 &vec_b ) {
    ivec2 vec(vec_a);
    vec += vec_b;
    return vec;
}

inline ivec2 operator-( const ivec2 &vec_a, const ivec2 &vec_b ) {
    ivec2 vec(vec_a);
    vec -= vec_b;
    return vec;
}

inline ivec2 operator*( const ivec2 &vec_a, const int param ) {
    ivec2 vec(vec_a);
    vec *= param;
    return vec;
}

inline ivec2 operator/( const ivec2 &vec_a, const int param ) {
    ivec2 vec(vec_a);
    vec /= param;
    return vec;
}

inline ivec2 operator/( const ivec2 &vec_a, const ivec2 &vec_b ) {
    return ivec2(vec_a.entries[0]/vec_b.entries[0],
                vec_a.entries[1]/vec_b.entries[1]);
}


inline ivec2 operator*(const ivec2 &vec_a, const ivec2 &vec_b) {
    return ivec2(vec_a[0]*vec_b[0],
                vec_a[1]*vec_b[1]);
}

inline ivec2 operator-(const ivec2 &vec) {
    return vec * 1;
}

inline ivec2 operator*(int param, const ivec2 &vec_b) {
    return vec_b*param;
}

inline bool operator==( const ivec2 &vec_a, const ivec2 &vec_b ) {
    return (vec_a.entries[0]==vec_b.entries[0] &&
            vec_a.entries[1]==vec_b.entries[1]);
}
inline bool operator<( const ivec2 &a, const ivec2 &b ) {
    return a.entries[0] <  b.entries[0] ||
          (a.entries[0] == b.entries[0] &&
          (a.entries[1] <  b.entries[1] ||
          (a.entries[1] == b.entries[1])));
}
