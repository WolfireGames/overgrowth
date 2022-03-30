//-----------------------------------------------------------------------------
//           Name: enginemath.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This contains most 3d math functions and classes
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

#include <Math/mat4.h>
#include <Math/mat3.h>
#include <Math/vec3.h>
#include <Math/quaternions.h>

#include <Internal/integer.h>

#include <algorithm>

using std::min;
using std::max;

const double PI = 3.141592653589793238462643383279502884197;
const float PI_f = (float)PI;
const double deg2rad = PI/180.0;
const double rad2deg = 180.0/PI;
const float deg2radf = (float)deg2rad;
const float rad2degf = (float)rad2deg;

const float EPSILON = 0.000001f;

#define PI_DEFINED 1

#define ISNAN(x) (x!=x)

float RangedRandomFloat(float min, float max);
int RangedRandomInt(int min, int max);

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
float YAxisRotationFromVector(const vec3 &theVector);
vec3 AngleAxisRotation(const vec3 &thePoint, const vec3 &theAxis, const float howmuch);
vec3 AngleAxisRotationRadian(const vec3 &thePoint, const vec3 &theAxis, const float howmuch);
vec3 doRotation(const vec3 &thePoint, const float xang, const float yang, const float zang);
vec3 doRotationRadian(const vec3 &thePoint, const float xang, const float yang, const float zang);

inline float clamp(float val, float floor, float ceil);
float Range(float val, float min_val, float max_val);

//-----------------------------------------------------------------------------
// Inline Function Definitions
//-----------------------------------------------------------------------------

inline float square( float f ) { return (f*f) ;}
inline int square( int f ) { return (f*f) ;}

template <typename T> inline T mix(T x, T y, float alpha) {
    return x * (1.0f - alpha) + y * alpha;    
}

template <typename T> inline T mix(T x, T y, double alpha) {
    return x * (1.0 - alpha) + y * alpha;    
}

template <typename T> T BlendFour(T* values, float* weights) {
    T value;
    float total_weight = weights[0] + weights[1];
    if(total_weight > 0.0f){
        value = mix(values[0],values[1],weights[1]/total_weight);
    }
    total_weight += weights[2];
    if(total_weight > 0.0f){
        value = mix(value,values[2],weights[2]/total_weight);
    }
    total_weight += weights[3];
    if(total_weight > 0.0f){
        value = mix(value,values[3],weights[3]/total_weight);
    }
    return value;
}

inline float clamp(float val, float floor, float ceil) {
    if(val < floor){
        return floor;
    } else if(val > ceil){
        return ceil;
    } else {
        return val;
    }
}

inline int clamp(int val, int floor, int ceil) {
    if(val < floor){
        return floor;
    } else if(val > ceil){
        return ceil;
    } else {
        return val;
    }
}


void PlaneSpace(const vec3 &n, vec3 &p, vec3 &q);

void GetRotationBetweenVectors(const vec3 &a, const vec3 &b, quaternion &rotate);

int popcount(uint64_t x);
