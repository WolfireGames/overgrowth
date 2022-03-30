//-----------------------------------------------------------------------------
//           Name: ivec4.h
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

#include <Math/ivec3.h>

#include <iostream>

class ivec4{
    public:
        int entries[4];
        
        inline int& operator[](const int which) {
            return entries[which];
        }
        inline const int& operator[](const int which) const {
            return entries[which];
        }

        inline explicit ivec4() {
            entries[0] = 0;
            entries[1] = 0;
            entries[2] = 0;
            entries[3] = 1;
        }

        inline ivec4(int val) {
            entries[0] = val;
            entries[1] = val;
            entries[2] = val;
            entries[3] = val;
        }

        inline ivec4(int newx, int newy, int newz, int neww = 1.0f) {
            entries[0] = newx;
            entries[1] = newy;
            entries[2] = newz;
            entries[3] = neww;
        }

        inline ivec4(const ivec3 &vec, int val = 1.0f) {
            entries[0] = vec[0];
            entries[1] = vec[1];
            entries[2] = vec[2];
            entries[3] = val;
        }

        inline ivec4& operator=(int param){
            entries[0]=param;
            entries[1]=param;
            entries[2]=param;
            entries[3]=param;
          return *this;
        }

        inline ivec4& operator=(const ivec4 &param){
            entries[0]=param.entries[0];
            entries[1]=param.entries[1];
            entries[2]=param.entries[2];
            entries[3]=param.entries[3];
            return *this;
        }

        inline ivec4& operator=(const ivec3 &param){
            entries[0]=param.entries[0];
            entries[1]=param.entries[1];
            entries[2]=param.entries[2];
            return *this;
        }

        inline const int& x() const { return entries[0]; }
        inline const int& y() const { return entries[1]; }
        inline const int& z() const { return entries[2]; }
        inline const int& w() const { return entries[3]; }
        inline int& x() { return entries[0]; }
        inline int& y() { return entries[1]; }
        inline int& z() { return entries[2]; }
        inline int& w() { return entries[3]; }

        inline ivec2 xy() const {
            return ivec2(entries[0], entries[1]);
        }

        inline ivec3 xyz() const {
            return ivec3(entries[0], entries[1], entries[2]);
        }

        inline void ToArray(int *array) const {
            array[0] = entries[0];
            array[1] = entries[1];
            array[2] = entries[2];
            array[3] = entries[3];
        }
        inline int *operator*()
        {
            return entries;
        }
};

inline std::ostream& operator<<(std::ostream& os, const ivec4& v )
{
    os << "ivec4(" << v[0] << "," << v[1] << "," << v[2] << "," << v[3] << ")";
    return os;
}
