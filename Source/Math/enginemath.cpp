//-----------------------------------------------------------------------------
//           Name: enginemath.cpp
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
#include "enginemath.h"

#include <Math/vec3math.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <cstdio> 
#include <algorithm>

void PlaneSpace(const vec3 &n, vec3 &p, vec3 &q) {
    if (std::fabs(n[2]) > 0.7071067f) {
        // choose p in y-z plane
        float a = n[1]*n[1] + n[2]*n[2]; 
        float k = 1.0f/sqrtf(a);
        p[0] = 0;
        p[1] = -n[2]*k;
        p[2] = n[1]*k;
        // set q = n x p
        q[0] = a*k;
        q[1] = -n[0]*p[2];
        q[2] = n[0]*p[1];
    }
    else {
        // choose p in x-y plane
        float a = n[0]*n[0] + n[1]*n[1];
        float k = 1.0f/sqrtf(a);
        p[0] = -n[1]*k;
        p[1] = n[0]*k;
        p[2] = 0;
        // set q = n x p
        q[0] = -n[2]*p[1];
        q[1] = n[2]*p[0];
        q[2] = a*k;
    }
}
float RangedRandomFloat(float min, float max) {
    if(min == max){
        return min;
    }
    return (((float)abs(rand()))/RAND_MAX*((float)(max)-(float)(min))+(float)(min));
}

int RangedRandomInt(int min, int max) { // Inclusive
    return abs(rand())%(max-min+1)+min;
}

float Range(float val, float min_val, float max_val) {
    float temp_val = val;
    temp_val -= min_val;
    temp_val /= (max_val-min_val);
    return min(1.0f,max(0.0f,temp_val));
}

float YAxisRotationFromVector(const vec3 &theVector)
{
    vec3 vector(theVector.x(),0,theVector.z());
    vector = normalize(vector);
    float new_rotation = std::acos(vector.z())/3.1415f*180.0f;
    if(vector.x()<0)new_rotation*=-1;
    new_rotation+=180;
    return new_rotation;
}

vec3 AngleAxisRotation(const vec3 &thePoint, const vec3 &theAxis, const float howmuch){
    return AngleAxisRotationRadian(thePoint, theAxis, howmuch * deg2radf);
}

vec3 AngleAxisRotationRadian(const vec3 &thePoint, const vec3 &theAxis, const float howmuch){
    float costheta=cosf(howmuch);
    return thePoint*costheta+theAxis*(dot(thePoint,theAxis))*(1-costheta)+cross(thePoint,theAxis)*sinf(howmuch);
}

vec3 doRotation(const vec3 &thePoint, const float xang, const float yang, const float zang){
    return doRotationRadian(thePoint, xang*deg2radf, yang*deg2radf, zang*deg2radf);
}

vec3 doRotationRadian(const vec3 &thePoint, const float xang, const float yang, const float zang){
    vec3 newpoint;
    vec3 oldpoint;
    
    oldpoint=thePoint;
    
    if(yang!=0){
    newpoint.z()=oldpoint.z()*cosf(yang)-oldpoint.x()*sinf(yang);
    newpoint.x()=oldpoint.z()*sinf(yang)+oldpoint.x()*cosf(yang);
    oldpoint.z()=newpoint.z();
    oldpoint.x()=newpoint.x();
    }
    
    if(zang!=0){
    newpoint.x()=oldpoint.x()*cosf(zang)-oldpoint.y()*sinf(zang);
    newpoint.y()=oldpoint.y()*cosf(zang)+oldpoint.x()*sinf(zang);
    oldpoint.x()=newpoint.x();
    oldpoint.y()=newpoint.y();
    }
    
    if(xang!=0){
    newpoint.y()=oldpoint.y()*cosf(xang)-oldpoint.z()*sinf(xang);
    newpoint.z()=oldpoint.y()*sinf(xang)+oldpoint.z()*cosf(xang);
    oldpoint.z()=newpoint.z();
    oldpoint.y()=newpoint.y();    
    }
    
    return oldpoint;
}

int log2(unsigned int x)
{
    int log = -1;    // special case for log2(0)
    while (x != 0) { x >>= 1; log++; }
    return log;
}

void GetRotationBetweenVectors( const vec3 &a, const vec3 &b, quaternion &rotate ) {
    vec3 rotate_axis = normalize(cross(a, b));
    vec3 up = normalize(a);
    vec3 right_vec = cross(up, rotate_axis);
    vec3 ik_dir = normalize(b);
    float rotate_angle = atan2f(-dot(ik_dir, right_vec), dot(ik_dir, up));
    rotate = quaternion(vec4(rotate_axis, rotate_angle));
}

const uint64_t m1  = 0x5555555555555555; //binary: 0101...
const uint64_t m2  = 0x3333333333333333; //binary: 00110011..
const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
const uint64_t m8  = 0x00ff00ff00ff00ff; //binary:  8 zeros,  8 ones ...
const uint64_t m16 = 0x0000ffff0000ffff; //binary: 16 zeros, 16 ones ...
const uint64_t m32 = 0x00000000ffffffff; //binary: 32 zeros, 32 ones
const uint64_t hff = 0xffffffffffffffff; //binary: all ones
const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...

int popcount(uint64_t x) {
    x -= (x >> 1) & m1;             //put count of each 2 bits into those 2 bits
    x = (x & m2) + ((x >> 2) & m2); //put count of each 4 bits into those 4 bits 
    x = (x + (x >> 4)) & m4;        //put count of each 8 bits into those 8 bits 
    return (x * h01)>>56;  //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
}
