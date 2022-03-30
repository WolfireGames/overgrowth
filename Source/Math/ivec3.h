//-----------------------------------------------------------------------------
//           Name: ivec3.h
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

#include <cassert>
#include <iostream>

class ivec3{
    public:
        int entries[3];
        
        inline explicit ivec3(int val = 0.0f) {
            entries[0] = val;
            entries[1] = val;
            entries[2] = val;
        }

        inline ivec3(int newx, int newy, int newz) {
            entries[0] = newx;
            entries[1] = newy;
            entries[2] = newz;
        }

        inline ivec3(const ivec2 &vec, int newz) {
            entries[0] = vec[0];
            entries[1] = vec[1];
            entries[2] = newz;
        }

        inline ivec3& operator=(int param) {
            entries[0] = param;
            entries[1] = param;
            entries[2] = param;
            return *this;
        }

        inline ivec3& operator=(const ivec3 &param) {
            entries[0] = param.entries[0];
            entries[1] = param.entries[1];
            entries[2] = param.entries[2];
            return *this;
        }

        inline int& operator[](const int which) { 
            assert(which < 3);
            return entries[which]; 
        }

        inline const int& operator[](const int which) const { 
            return entries[which]; 
        }

        inline const int& x() const { return entries[0]; }
        inline const int& y() const { return entries[1]; }
        inline const int& z() const { return entries[2]; }
        inline int& x() { return entries[0]; }
        inline int& y() { return entries[1]; }
        inline int& z() { return entries[2]; }
        inline const int& r() const { return entries[0]; }
        inline const int& g() const { return entries[1]; }
        inline const int& b() const { return entries[2]; }
        inline int& r() { return entries[0]; }
        inline int& g() { return entries[1]; }
        inline int& b() { return entries[2]; }

        inline ivec2 xy() const {
            return ivec2(entries[0], entries[1]);
        }

        inline int *operator*() { return entries; }
};

inline std::ostream& operator<<(std::ostream &os, const ivec3& vec) {
    os << "ivec3(" << vec[0] << ", " << vec[1] << ", " << vec[2] << ")";
    return os;
}
