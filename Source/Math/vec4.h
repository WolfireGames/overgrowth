//-----------------------------------------------------------------------------
//           Name: vec4.h
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

#include <iostream>

class vec4{
    public:
        float entries[4];
        
        inline float& operator[](const int which) {
            return entries[which];
        }
        inline const float& operator[](const int which) const {
            return entries[which];
        }

        inline explicit vec4() {
            entries[0] = 0.0f;
            entries[1] = 0.0f;
            entries[2] = 0.0f;
            entries[3] = 1.0f;
        }

        inline vec4(float val) {
            entries[0] = val;
            entries[1] = val;
            entries[2] = val;
            entries[3] = val;
        }

        inline vec4(float newx, float newy, float newz, float neww = 1.0f) {
            entries[0] = newx;
            entries[1] = newy;
            entries[2] = newz;
            entries[3] = neww;
        }

        inline vec4(const vec3 &vec, float val = 1.0f) {
            entries[0] = vec[0];
            entries[1] = vec[1];
            entries[2] = vec[2];
            entries[3] = val;
        }

        inline vec4& operator=(float param){
            entries[0]=param;
            entries[1]=param;
            entries[2]=param;
            entries[3]=param;
          return *this;
        }

        inline vec4& operator=(const vec4 &param){
            entries[0]=param.entries[0];
            entries[1]=param.entries[1];
            entries[2]=param.entries[2];
            entries[3]=param.entries[3];
            return *this;
        }

        inline vec4& operator=(const vec3 &param){
            entries[0]=param.entries[0];
            entries[1]=param.entries[1];
            entries[2]=param.entries[2];
            return *this;
        }

        inline const float& x() const { return entries[0]; }
        inline const float& y() const { return entries[1]; }
        inline const float& z() const { return entries[2]; }
        inline const float& w() const { return entries[3]; }
        inline const float& r() const { return entries[0]; }
        inline const float& g() const { return entries[1]; }
        inline const float& b() const { return entries[2]; }
        inline const float& a() const { return entries[3]; }
        inline const float& angle() const { return entries[3]; }
        inline float& x() { return entries[0]; }
        inline float& y() { return entries[1]; }
        inline float& z() { return entries[2]; }
        inline float& w() { return entries[3]; }
        inline float& r() { return entries[0]; }
        inline float& g() { return entries[1]; }
        inline float& b() { return entries[2]; }
        inline float& a() { return entries[3]; }
        inline float& angle() { return entries[3]; }

        inline vec2 xy() const {
            return vec2(entries[0], entries[1]);
        }

        inline vec3 xyz() const {
            return vec3(entries[0], entries[1], entries[2]);
        }

        inline void ToArray(float *array) const {
            array[0] = entries[0];
            array[1] = entries[1];
            array[2] = entries[2];
            array[3] = entries[3];
        }
        inline float *operator*()
        {
            return entries;
        }
};

inline std::ostream& operator<<(std::ostream& os, const vec4& v )
{
    os << "vec4(" << v[0] << "," << v[1] << "," << v[2] << "," << v[3] << ")";
    return os;
}
