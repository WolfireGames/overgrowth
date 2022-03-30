//-----------------------------------------------------------------------------
//           Name: simd_mat4.h
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

#include <xmmintrin.h>

class simd_mat4
{
public:
    __m128 col[4];

    void assign(const float* entries){
        col[0] = _mm_loadu_ps(&entries[0]);
        col[1] = _mm_loadu_ps(&entries[4]);
        col[2] = _mm_loadu_ps(&entries[8]);
        col[3] = _mm_loadu_ps(&entries[12]);

        /*__declspec(align(16)) float entries_rearranged[] = {
            entries[0], entries[4], entries[8],  entries[12],
            entries[1], entries[5], entries[9],  entries[13],
            entries[2], entries[6], entries[10], entries[14],
            entries[3], entries[7], entries[11], entries[15]};
        row[0] = _mm_load_ps(&entries_rearranged[0]);
        row[1] = _mm_load_ps(&entries_rearranged[4]);
        row[2] = _mm_load_ps(&entries_rearranged[8]);
        row[3] = _mm_load_ps(&entries_rearranged[12]);*/
    }

    simd_mat4& operator=(const simd_mat4 &other){
        col[0] = other.col[0];
        col[1] = other.col[1];
        col[2] = other.col[2];
        col[3] = other.col[3];
        return *this;
    }
    
    simd_mat4 operator*(float val){
        __m128 _m_val = _mm_load1_ps(&val);
        simd_mat4 result;
        result.col[0] = _mm_mul_ps(col[0], _m_val);
        result.col[1] = _mm_mul_ps(col[1], _m_val);
        result.col[2] = _mm_mul_ps(col[2], _m_val);
        result.col[3] = _mm_mul_ps(col[3], _m_val);
        return result;
    }

    void operator+=(const simd_mat4 &other){
        col[0] = _mm_add_ps(col[0], other.col[0]);
        col[1] = _mm_add_ps(col[1], other.col[1]);
        col[2] = _mm_add_ps(col[2], other.col[2]);
        col[3] = _mm_add_ps(col[3], other.col[3]);
    }

    void AddRotatedVec3(const float *rhs) {
#ifdef _WIN32
        __declspec(align(16)) float me[16];
#else
        static float me[16] __attribute__((aligned(16)));
#endif
        
        _mm_store_ps(&me[0], col[0]);
        _mm_store_ps(&me[4], col[1]);
        _mm_store_ps(&me[8], col[2]);
        _mm_store_ps(&me[12], col[3]);

        me[12] += me[0]*rhs[0] + me[4]*rhs[1] + me[8]*rhs[2];
        me[13] += me[1]*rhs[0] + me[5]*rhs[1] + me[9]*rhs[2];
        me[14] += me[2]*rhs[0] + me[6]*rhs[1] + me[10]*rhs[2];

        col[0] = _mm_load_ps(&me[0]);
        col[1] = _mm_load_ps(&me[4]);
        col[2] = _mm_load_ps(&me[8]);
        col[3] = _mm_load_ps(&me[12]);
    }
    
    void ToVec4(float *vector1, float *vector2, float *vector3, float *vector4){
#ifdef _WIN32
        __declspec(align(16)) float entries[16];
#else
        static float entries[16] __attribute__((aligned(16)));
#endif
        _mm_store_ps(&entries[0], col[0]);
        _mm_store_ps(&entries[4], col[1]);
        _mm_store_ps(&entries[8], col[2]);
        _mm_store_ps(&entries[12], col[3]);

        vector1[0] = entries[0];
        vector1[1] = entries[4];
        vector1[2] = entries[8];
        vector1[3] = entries[12];
        vector2[0] = entries[1];
        vector2[1] = entries[5];
        vector2[2] = entries[9];
        vector2[3] = entries[13];
        vector3[0] = entries[2];
        vector3[1] = entries[6];
        vector3[2] = entries[10];
        vector3[3] = entries[14];
        vector4[0] = entries[3];
        vector4[1] = entries[7];
        vector4[2] = entries[11];
        vector4[3] = entries[15];
    }

    void AddRotatedVec3AndSave(const float *rhs, float *vector1, float *vector2, float *vector3, float *vector4) {
#ifdef _WIN32
        __declspec(align(16)) float me[16];
#else
        static float me[16] __attribute__((aligned(16)));
#endif
        _mm_store_ps(&me[0], col[0]);
        _mm_store_ps(&me[4], col[1]);
        _mm_store_ps(&me[8], col[2]);
        _mm_store_ps(&me[12], col[3]);

        me[12] += me[0]*rhs[0] + me[4]*rhs[1] + me[8]*rhs[2];
        me[13] += me[1]*rhs[0] + me[5]*rhs[1] + me[9]*rhs[2];
        me[14] += me[2]*rhs[0] + me[6]*rhs[1] + me[10]*rhs[2];

        vector1[0] = me[0];
        vector1[1] = me[4];
        vector1[2] = me[8];
        vector1[3] = me[12];
        vector2[0] = me[1];
        vector2[1] = me[5];
        vector2[2] = me[9];
        vector2[3] = me[13];
        vector3[0] = me[2];
        vector3[1] = me[6];
        vector3[2] = me[10];
        vector3[3] = me[14];
        vector4[0] = me[3];
        vector4[1] = me[7];
        vector4[2] = me[11];
        vector4[3] = me[15];
    }
};
