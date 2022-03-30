//-----------------------------------------------------------------------------
//           Name: mat4.cpp
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
#include "mat4.h"

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <cmath>
#include <memory.h>

mat4::mat4(float val) {
    memset(entries, 0, 16*sizeof(float));
    entries[0]=val;
    entries[5]=val;
    entries[10]=val;
    entries[15]=val;    
}

mat4::mat4(float e0, float e1, float e2, float e3,
           float e4, float e5, float e6, float e7,
           float e8, float e9, float e10, float e11,
           float e12, float e13, float e14, float e15)
{
    entries[0]=e0;
    entries[1]=e1;
    entries[2]=e2;
    entries[3]=e3;
    entries[4]=e4;
    entries[5]=e5;
    entries[6]=e6;
    entries[7]=e7;
    entries[8]=e8;
    entries[9]=e9;
    entries[10]=e10;
    entries[11]=e11;
    entries[12]=e12;
    entries[13]=e13;
    entries[14]=e14;
    entries[15]=e15;
}

mat4::mat4(const mat4 & rhs)
{
    memcpy(entries, rhs.entries, 16*sizeof(float));
}

mat4::mat4(const float * rhs)
{
    memcpy(entries, rhs, 16*sizeof(float));
}
    
vec4 mat4::GetRow(int position) const
{
    if(position==0)
        return vec4(entries[0], entries[4], entries[8], entries[12]);
    
    if(position==1)
        return vec4(entries[1], entries[5], entries[9], entries[13]);
    
    if(position==2)
        return vec4(entries[2], entries[6], entries[10], entries[14]);
    
    if(position==3)
        return vec4(entries[3], entries[7], entries[11], entries[15]);

    return vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

vec3 mat4::GetRowVec3(int position) const
{
    if(position==0)
        return vec3(entries[0], entries[4], entries[8]);

    if(position==1)
        return vec3(entries[1], entries[5], entries[9]);

    if(position==2)
        return vec3(entries[2], entries[6], entries[10]);

    if(position==3)
        return vec3(entries[3], entries[7], entries[11]);

    return vec3(0.0f, 0.0f, 0.0f);
}

void mat4::SetRow(int position, const vec4 &vec)
{
    entries[position] = vec[0];
    entries[position+4] = vec[1];
    entries[position+8] = vec[2];
    entries[position+12] = vec[3];
}

void mat4::SetRow(int position, const vec3 &vec)
{
    entries[position] = vec[0];
    entries[position+4] = vec[1];
    entries[position+8] = vec[2];
}

void mat4::ToVec4(vec4 *vector1, vec4 *vector2, vec4 *vector3, vec4 *vector4) const {
    vector1->entries[0] = entries[0];
    vector1->entries[1] = entries[4];
    vector1->entries[2] = entries[8];
    vector1->entries[3] = entries[12];
    vector2->entries[0] = entries[1];
    vector2->entries[1] = entries[5];
    vector2->entries[2] = entries[9];
    vector2->entries[3] = entries[13];
    vector3->entries[0] = entries[2];
    vector3->entries[1] = entries[6];
    vector3->entries[2] = entries[10];
    vector3->entries[3] = entries[14];
    vector4->entries[0] = entries[3];
    vector4->entries[1] = entries[7];
    vector4->entries[2] = entries[11];
    vector4->entries[3] = entries[15];
}

void mat4::FromVec4(const vec4& vector1, 
                    const vec4& vector2, 
                    const vec4& vector3, 
                    const vec4& vector4) 
{
    entries[0]  = vector1.entries[0];
    entries[4]  = vector1.entries[1];
    entries[8]  = vector1.entries[2];
    entries[12] = vector1.entries[3];
    entries[1]  = vector2.entries[0];
    entries[5]  = vector2.entries[1];
    entries[9]  = vector2.entries[2];
    entries[13] = vector2.entries[3];
    entries[2]  = vector3.entries[0];
    entries[6]  = vector3.entries[1];
    entries[10] = vector3.entries[2];
    entries[14] = vector3.entries[3];
    entries[3]  = vector4.entries[0];
    entries[7]  = vector4.entries[1];
    entries[11] = vector4.entries[2];
    entries[15] = vector4.entries[3];
}

vec4 mat4::GetColumn(int position) const
{
    if(position==0)
        return vec4(entries[0], entries[1], entries[2], entries[3]);
    
    if(position==1)
        return vec4(entries[4], entries[5], entries[6], entries[7]);
    
    if(position==2)
        return vec4(entries[8], entries[9], entries[10], entries[11]);
    
    if(position==3)
        return vec4(entries[12], entries[13], entries[14], entries[15]);

    return vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

vec3 mat4::GetColumnVec3(int position) const
{
    if(position==0)
        return vec3(entries[0], entries[1], entries[2]);

    if(position==1)
        return vec3(entries[4], entries[5], entries[6]);

    if(position==2)
        return vec3(entries[8], entries[9], entries[10]);

    if(position==3)
        return vec3(entries[12], entries[13], entries[14]);

    return vec3(0.0f, 0.0f, 0.0f);
}

void mat4::SetColumn(int position, const vec4 &vec)
{
    int start = position*4;
    entries[start] = vec[0];
    entries[start+1] = vec[1];
    entries[start+2] = vec[2];
    entries[start+3] = vec[3];
}

void mat4::SetColumn(int position, const vec3 &vec)
{
    int start = position*4;
    entries[start] = vec[0];
    entries[start+1] = vec[1];
    entries[start+2] = vec[2];
}

float mat4::GetDeterminant()
{
    float det;
    det = entries[0] * entries[5] * entries[10]    - 
          entries[0] * entries[6] * entries[9]    - 
          entries[1] * entries[4] * entries[10] + 
          entries[2] * entries[4] * entries[9]    + 
          entries[1] * entries[6] * entries[8]    - 
          entries[2] * entries[5] * entries[8];
    return det;
}

void mat4::LoadIdentity(void)
{
    memset(entries, 0, 16*sizeof(float));
    entries[0]=1.0f;
    entries[5]=1.0f;
    entries[10]=1.0f;
    entries[15]=1.0f;
}

mat4 mat4::operator+(const mat4 & rhs) const        //overloaded operators
{
    return mat4(    entries[0]+rhs.entries[0],
                        entries[1]+rhs.entries[1],
                        entries[2]+rhs.entries[2],
                        entries[3]+rhs.entries[3],
                        entries[4]+rhs.entries[4],
                        entries[5]+rhs.entries[5],
                        entries[6]+rhs.entries[6],
                        entries[7]+rhs.entries[7],
                        entries[8]+rhs.entries[8],
                        entries[9]+rhs.entries[9],
                        entries[10]+rhs.entries[10],
                        entries[11]+rhs.entries[11],
                        entries[12]+rhs.entries[12],
                        entries[13]+rhs.entries[13],
                        entries[14]+rhs.entries[14],
                        entries[15]+rhs.entries[15]);
}

mat4 mat4::operator-(const mat4 & rhs) const        //overloaded operators
{
    return mat4(    entries[0]-rhs.entries[0],
                        entries[1]-rhs.entries[1],
                        entries[2]-rhs.entries[2],
                        entries[3]-rhs.entries[3],
                        entries[4]-rhs.entries[4],
                        entries[5]-rhs.entries[5],
                        entries[6]-rhs.entries[6],
                        entries[7]-rhs.entries[7],
                        entries[8]-rhs.entries[8],
                        entries[9]-rhs.entries[9],
                        entries[10]-rhs.entries[10],
                        entries[11]-rhs.entries[11],
                        entries[12]-rhs.entries[12],
                        entries[13]-rhs.entries[13],
                        entries[14]-rhs.entries[14],
                        entries[15]-rhs.entries[15]);
}

mat4 mat4::operator*(const mat4 & rhs) const
{
    //Optimise for matrices in which bottom row is (0, 0, 0, 1) in both matrices
    if(    entries[3]==0.0f && entries[7]==0.0f && entries[11]==0.0f && entries[15]==1.0f    &&
        rhs.entries[3]==0.0f && rhs.entries[7]==0.0f &&
        rhs.entries[11]==0.0f && rhs.entries[15]==1.0f)
    {
        return mat4(    entries[0]*rhs.entries[0]+entries[4]*rhs.entries[1]+entries[8]*rhs.entries[2],
                            entries[1]*rhs.entries[0]+entries[5]*rhs.entries[1]+entries[9]*rhs.entries[2],
                            entries[2]*rhs.entries[0]+entries[6]*rhs.entries[1]+entries[10]*rhs.entries[2],
                            0.0f,
                            entries[0]*rhs.entries[4]+entries[4]*rhs.entries[5]+entries[8]*rhs.entries[6],
                            entries[1]*rhs.entries[4]+entries[5]*rhs.entries[5]+entries[9]*rhs.entries[6],
                            entries[2]*rhs.entries[4]+entries[6]*rhs.entries[5]+entries[10]*rhs.entries[6],
                            0.0f,
                            entries[0]*rhs.entries[8]+entries[4]*rhs.entries[9]+entries[8]*rhs.entries[10],
                            entries[1]*rhs.entries[8]+entries[5]*rhs.entries[9]+entries[9]*rhs.entries[10],
                            entries[2]*rhs.entries[8]+entries[6]*rhs.entries[9]+entries[10]*rhs.entries[10],
                            0.0f,
                            entries[0]*rhs.entries[12]+entries[4]*rhs.entries[13]+entries[8]*rhs.entries[14]+entries[12],
                            entries[1]*rhs.entries[12]+entries[5]*rhs.entries[13]+entries[9]*rhs.entries[14]+entries[13],
                            entries[2]*rhs.entries[12]+entries[6]*rhs.entries[13]+entries[10]*rhs.entries[14]+entries[14],
                            1.0f);
    }

    //Optimise for when bottom row of 1st matrix is (0, 0, 0, 1)
    if(    entries[3]==0.0f && entries[7]==0.0f && entries[11]==0.0f && entries[15]==1.0f)
    {
        return mat4(    entries[0]*rhs.entries[0]+entries[4]*rhs.entries[1]+entries[8]*rhs.entries[2]+entries[12]*rhs.entries[3],
                            entries[1]*rhs.entries[0]+entries[5]*rhs.entries[1]+entries[9]*rhs.entries[2]+entries[13]*rhs.entries[3],
                            entries[2]*rhs.entries[0]+entries[6]*rhs.entries[1]+entries[10]*rhs.entries[2]+entries[14]*rhs.entries[3],
                            rhs.entries[3],
                            entries[0]*rhs.entries[4]+entries[4]*rhs.entries[5]+entries[8]*rhs.entries[6]+entries[12]*rhs.entries[7],
                            entries[1]*rhs.entries[4]+entries[5]*rhs.entries[5]+entries[9]*rhs.entries[6]+entries[13]*rhs.entries[7],
                            entries[2]*rhs.entries[4]+entries[6]*rhs.entries[5]+entries[10]*rhs.entries[6]+entries[14]*rhs.entries[7],
                            rhs.entries[7],
                            entries[0]*rhs.entries[8]+entries[4]*rhs.entries[9]+entries[8]*rhs.entries[10]+entries[12]*rhs.entries[11],
                            entries[1]*rhs.entries[8]+entries[5]*rhs.entries[9]+entries[9]*rhs.entries[10]+entries[13]*rhs.entries[11],
                            entries[2]*rhs.entries[8]+entries[6]*rhs.entries[9]+entries[10]*rhs.entries[10]+entries[14]*rhs.entries[11],
                            rhs.entries[11],
                            entries[0]*rhs.entries[12]+entries[4]*rhs.entries[13]+entries[8]*rhs.entries[14]+entries[12]*rhs.entries[15],
                            entries[1]*rhs.entries[12]+entries[5]*rhs.entries[13]+entries[9]*rhs.entries[14]+entries[13]*rhs.entries[15],
                            entries[2]*rhs.entries[12]+entries[6]*rhs.entries[13]+entries[10]*rhs.entries[14]+entries[14]*rhs.entries[15],
                            rhs.entries[15]);
    }

    //Optimise for when bottom row of 2nd matrix is (0, 0, 0, 1)
    if(    rhs.entries[3]==0.0f && rhs.entries[7]==0.0f &&
        rhs.entries[11]==0.0f && rhs.entries[15]==1.0f)
    {
        return mat4(    entries[0]*rhs.entries[0]+entries[4]*rhs.entries[1]+entries[8]*rhs.entries[2],
                            entries[1]*rhs.entries[0]+entries[5]*rhs.entries[1]+entries[9]*rhs.entries[2],
                            entries[2]*rhs.entries[0]+entries[6]*rhs.entries[1]+entries[10]*rhs.entries[2],
                            entries[3]*rhs.entries[0]+entries[7]*rhs.entries[1]+entries[11]*rhs.entries[2],
                            entries[0]*rhs.entries[4]+entries[4]*rhs.entries[5]+entries[8]*rhs.entries[6],
                            entries[1]*rhs.entries[4]+entries[5]*rhs.entries[5]+entries[9]*rhs.entries[6],
                            entries[2]*rhs.entries[4]+entries[6]*rhs.entries[5]+entries[10]*rhs.entries[6],
                            entries[3]*rhs.entries[4]+entries[7]*rhs.entries[5]+entries[11]*rhs.entries[6],
                            entries[0]*rhs.entries[8]+entries[4]*rhs.entries[9]+entries[8]*rhs.entries[10],
                            entries[1]*rhs.entries[8]+entries[5]*rhs.entries[9]+entries[9]*rhs.entries[10],
                            entries[2]*rhs.entries[8]+entries[6]*rhs.entries[9]+entries[10]*rhs.entries[10],
                            entries[3]*rhs.entries[8]+entries[7]*rhs.entries[9]+entries[11]*rhs.entries[10],
                            entries[0]*rhs.entries[12]+entries[4]*rhs.entries[13]+entries[8]*rhs.entries[14]+entries[12],
                            entries[1]*rhs.entries[12]+entries[5]*rhs.entries[13]+entries[9]*rhs.entries[14]+entries[13],
                            entries[2]*rhs.entries[12]+entries[6]*rhs.entries[13]+entries[10]*rhs.entries[14]+entries[14],
                            entries[3]*rhs.entries[12]+entries[7]*rhs.entries[13]+entries[11]*rhs.entries[14]+entries[15]);
    }    
    
    return mat4(    entries[0]*rhs.entries[0]+entries[4]*rhs.entries[1]+entries[8]*rhs.entries[2]+entries[12]*rhs.entries[3],
                        entries[1]*rhs.entries[0]+entries[5]*rhs.entries[1]+entries[9]*rhs.entries[2]+entries[13]*rhs.entries[3],
                        entries[2]*rhs.entries[0]+entries[6]*rhs.entries[1]+entries[10]*rhs.entries[2]+entries[14]*rhs.entries[3],
                        entries[3]*rhs.entries[0]+entries[7]*rhs.entries[1]+entries[11]*rhs.entries[2]+entries[15]*rhs.entries[3],
                        entries[0]*rhs.entries[4]+entries[4]*rhs.entries[5]+entries[8]*rhs.entries[6]+entries[12]*rhs.entries[7],
                        entries[1]*rhs.entries[4]+entries[5]*rhs.entries[5]+entries[9]*rhs.entries[6]+entries[13]*rhs.entries[7],
                        entries[2]*rhs.entries[4]+entries[6]*rhs.entries[5]+entries[10]*rhs.entries[6]+entries[14]*rhs.entries[7],
                        entries[3]*rhs.entries[4]+entries[7]*rhs.entries[5]+entries[11]*rhs.entries[6]+entries[15]*rhs.entries[7],
                        entries[0]*rhs.entries[8]+entries[4]*rhs.entries[9]+entries[8]*rhs.entries[10]+entries[12]*rhs.entries[11],
                        entries[1]*rhs.entries[8]+entries[5]*rhs.entries[9]+entries[9]*rhs.entries[10]+entries[13]*rhs.entries[11],
                        entries[2]*rhs.entries[8]+entries[6]*rhs.entries[9]+entries[10]*rhs.entries[10]+entries[14]*rhs.entries[11],
                        entries[3]*rhs.entries[8]+entries[7]*rhs.entries[9]+entries[11]*rhs.entries[10]+entries[15]*rhs.entries[11],
                        entries[0]*rhs.entries[12]+entries[4]*rhs.entries[13]+entries[8]*rhs.entries[14]+entries[12]*rhs.entries[15],
                        entries[1]*rhs.entries[12]+entries[5]*rhs.entries[13]+entries[9]*rhs.entries[14]+entries[13]*rhs.entries[15],
                        entries[2]*rhs.entries[12]+entries[6]*rhs.entries[13]+entries[10]*rhs.entries[14]+entries[14]*rhs.entries[15],
                        entries[3]*rhs.entries[12]+entries[7]*rhs.entries[13]+entries[11]*rhs.entries[14]+entries[15]*rhs.entries[15]);
}

mat4 mat4::operator*(const float rhs) const
{
    return mat4(    entries[0]*rhs,
                        entries[1]*rhs,
                        entries[2]*rhs,
                        entries[3]*rhs,
                        entries[4]*rhs,
                        entries[5]*rhs,
                        entries[6]*rhs,
                        entries[7]*rhs,
                        entries[8]*rhs,
                        entries[9]*rhs,
                        entries[10]*rhs,
                        entries[11]*rhs,
                        entries[12]*rhs,
                        entries[13]*rhs,
                        entries[14]*rhs,
                        entries[15]*rhs);
}

mat4 mat4::operator/(const float rhs) const
{
    if (rhs==0.0f || rhs==1.0f)
        return (*this);
        
    float temp=1/rhs;

    return (*this)*temp;
}

mat4 operator*(float scaleFactor, const mat4 & rhs)
{
    return rhs*scaleFactor;
}

bool mat4::operator==(const mat4 & rhs) const
{
    for (int i = 0; i<16; i++)
    {
        if(entries[i]!=rhs.entries[i])
            return false;
    }
    return true;
}

bool mat4::operator!=(const mat4 & rhs) const
{
    return !((*this)==rhs);
}

void mat4::operator+=(const mat4 & rhs)
{
    (*this)=(*this)+rhs;
}

void mat4::operator-=(const mat4 & rhs)
{
    (*this)=(*this)-rhs;
}

void mat4::operator*=(const mat4 & rhs)
{
    (*this)=(*this)*rhs;
}

void mat4::operator*=(const float rhs)
{
    (*this)=(*this)*rhs;
}

void mat4::operator/=(const float rhs)
{
    (*this)=(*this)/rhs;
}

mat4 mat4::operator-(void) const
{
    mat4 result(*this);

    for (int i = 0; i<16; i++)
        result.entries[i]=-result.entries[i];

    return result;
}

vec4 mat4::operator*(const vec4 &rhs) const
{
    //Optimise for matrices in which bottom row is (0, 0, 0, 1)
    if(entries[3]==0.0f && entries[7]==0.0f && entries[11]==0.0f && entries[15]==1.0f)
    {
        return vec4(entries[0]*rhs.x()
                    +    entries[4]*rhs.y()
                    +    entries[8]*rhs.z()
                    +    entries[12]*rhs.w(),

                        entries[1]*rhs.x()
                    +    entries[5]*rhs.y()
                    +    entries[9]*rhs.z()
                    +    entries[13]*rhs.w(),

                        entries[2]*rhs.x()
                    +    entries[6]*rhs.y()
                    +    entries[10]*rhs.z()
                    +    entries[14]*rhs.w(),

                        rhs.w());
    }
    
    return vec4(    entries[0]*rhs.x()
                    +    entries[4]*rhs.y()
                    +    entries[8]*rhs.z()
                    +    entries[12]*rhs.w(),

                        entries[1]*rhs.x()
                    +    entries[5]*rhs.y()
                    +    entries[9]*rhs.z()
                    +    entries[13]*rhs.w(),

                        entries[2]*rhs.x()
                    +    entries[6]*rhs.y()
                    +    entries[10]*rhs.z()
                    +    entries[14]*rhs.w(),

                        entries[3]*rhs.x()
                    +    entries[7]*rhs.y()
                    +    entries[11]*rhs.z()
                    +    entries[15]*rhs.w());
}

vec3 mat4::operator*(const vec3 &rhs) const
{
    return (*this * vec4(rhs,1.0f)).xyz();
}

vec3 mat4::GetRotatedvec3(const vec3 & rhs) const
{
    return vec3(entries[0]*rhs.x() + entries[4]*rhs.y() + entries[8]*rhs.z(),
                entries[1]*rhs.x() + entries[5]*rhs.y() + entries[9]*rhs.z(),
                entries[2]*rhs.x() + entries[6]*rhs.y() + entries[10]*rhs.z());
}

vec3 mat4::GetInverseRotatedvec3(const vec3 & rhs) const
{
    //rotate by transpose:
    return vec3(entries[0]*rhs.x() + entries[1]*rhs.y() + entries[2]*rhs.z(),
                entries[4]*rhs.x() + entries[5]*rhs.y() + entries[6]*rhs.z(),
                entries[8]*rhs.x() + entries[9]*rhs.y() + entries[10]*rhs.z());
}

vec3 mat4::GetTranslatedvec3(const vec3 & rhs) const
{
    return vec3(rhs.x()+entries[12], rhs.y()+entries[13], rhs.z()+entries[14]);
}

vec3 mat4::GetInverseTranslatedvec3(const vec3 & rhs) const
{
    return vec3(rhs.x()-entries[12], rhs.y()-entries[13], rhs.z()-entries[14]);
}

mat4 invert(const mat4& mat) {
    return transpose(mat.GetInverseTranspose());
}

mat4 transpose(const mat4& mat) {
    return mat4(mat.entries[ 0], mat.entries[ 4], mat.entries[ 8], mat.entries[12],
                mat.entries[ 1], mat.entries[ 5], mat.entries[ 9], mat.entries[13],
                mat.entries[ 2], mat.entries[ 6], mat.entries[10], mat.entries[14],
                mat.entries[ 3], mat.entries[ 7], mat.entries[11], mat.entries[15]);
}

void mat4::InvertTranspose(void)
{
    *this=GetInverseTranspose();
}

mat4 mat4::GetInverseTranspose(void) const
{
    mat4 result;

    float tmp[12];                                                //temporary pair storage
    float det;                                                    //determinant

    //calculate pairs for first 8 elements (cofactors)
    tmp[0] = entries[10] * entries[15];
    tmp[1] = entries[11] * entries[14];
    tmp[2] = entries[9] * entries[15];
    tmp[3] = entries[11] * entries[13];
    tmp[4] = entries[9] * entries[14];
    tmp[5] = entries[10] * entries[13];
    tmp[6] = entries[8] * entries[15];
    tmp[7] = entries[11] * entries[12];
    tmp[8] = entries[8] * entries[14];
    tmp[9] = entries[10] * entries[12];
    tmp[10] = entries[8] * entries[13];
    tmp[11] = entries[9] * entries[12];

    //calculate first 8 elements (cofactors)
    result.entries[0] =        tmp[0]*entries[5] + tmp[3]*entries[6] + tmp[4]*entries[7]
                    -    tmp[1]*entries[5] - tmp[2]*entries[6] - tmp[5]*entries[7];

    result.entries[1] =        tmp[1]*entries[4] + tmp[6]*entries[6] + tmp[9]*entries[7]
                    -    tmp[0]*entries[4] - tmp[7]*entries[6] - tmp[8]*entries[7];

    result.entries[2] =        tmp[2]*entries[4] + tmp[7]*entries[5] + tmp[10]*entries[7]
                    -    tmp[3]*entries[4] - tmp[6]*entries[5] - tmp[11]*entries[7];

    result.entries[3] =        tmp[5]*entries[4] + tmp[8]*entries[5] + tmp[11]*entries[6]
                    -    tmp[4]*entries[4] - tmp[9]*entries[5] - tmp[10]*entries[6];

    result.entries[4] =        tmp[1]*entries[1] + tmp[2]*entries[2] + tmp[5]*entries[3]
                    -    tmp[0]*entries[1] - tmp[3]*entries[2] - tmp[4]*entries[3];

    result.entries[5] =        tmp[0]*entries[0] + tmp[7]*entries[2] + tmp[8]*entries[3]
                    -    tmp[1]*entries[0] - tmp[6]*entries[2] - tmp[9]*entries[3];

    result.entries[6] =        tmp[3]*entries[0] + tmp[6]*entries[1] + tmp[11]*entries[3]
                    -    tmp[2]*entries[0] - tmp[7]*entries[1] - tmp[10]*entries[3];

    result.entries[7] =        tmp[4]*entries[0] + tmp[9]*entries[1] + tmp[10]*entries[2]
                    -    tmp[5]*entries[0] - tmp[8]*entries[1] - tmp[11]*entries[2];

    //calculate pairs for second 8 elements (cofactors)
    tmp[0] = entries[2]*entries[7];
    tmp[1] = entries[3]*entries[6];
    tmp[2] = entries[1]*entries[7];
    tmp[3] = entries[3]*entries[5];
    tmp[4] = entries[1]*entries[6];
    tmp[5] = entries[2]*entries[5];
    tmp[6] = entries[0]*entries[7];
    tmp[7] = entries[3]*entries[4];
    tmp[8] = entries[0]*entries[6];
    tmp[9] = entries[2]*entries[4];
    tmp[10] = entries[0]*entries[5];
    tmp[11] = entries[1]*entries[4];

    //calculate second 8 elements (cofactors)
    result.entries[8] =        tmp[0]*entries[13] + tmp[3]*entries[14] + tmp[4]*entries[15]
                    -    tmp[1]*entries[13] - tmp[2]*entries[14] - tmp[5]*entries[15];

    result.entries[9] =        tmp[1]*entries[12] + tmp[6]*entries[14] + tmp[9]*entries[15]
                    -    tmp[0]*entries[12] - tmp[7]*entries[14] - tmp[8]*entries[15];

    result.entries[10] =        tmp[2]*entries[12] + tmp[7]*entries[13] + tmp[10]*entries[15]
                    -    tmp[3]*entries[12] - tmp[6]*entries[13] - tmp[11]*entries[15];

    result.entries[11] =        tmp[5]*entries[12] + tmp[8]*entries[13] + tmp[11]*entries[14]
                    -    tmp[4]*entries[12] - tmp[9]*entries[13] - tmp[10]*entries[14];

    result.entries[12] =        tmp[2]*entries[10] + tmp[5]*entries[11] + tmp[1]*entries[9]
                    -    tmp[4]*entries[11] - tmp[0]*entries[9] - tmp[3]*entries[10];

    result.entries[13] =        tmp[8]*entries[11] + tmp[0]*entries[8] + tmp[7]*entries[10]
                    -    tmp[6]*entries[10] - tmp[9]*entries[11] - tmp[1]*entries[8];

    result.entries[14] =        tmp[6]*entries[9] + tmp[11]*entries[11] + tmp[3]*entries[8]
                    -    tmp[10]*entries[11] - tmp[2]*entries[8] - tmp[7]*entries[9];

    result.entries[15] =        tmp[10]*entries[10] + tmp[4]*entries[8] + tmp[9]*entries[9]
                    -    tmp[8]*entries[9] - tmp[11]*entries[10] - tmp[5]*entries[8];

    // calculate determinant
    det    =     entries[0]*result.entries[0]
            +entries[1]*result.entries[1]
            +entries[2]*result.entries[2]
            +entries[3]*result.entries[3];

    if(det==0.0f)
    {
        mat4 id;
        return id;
    }
    
    result=result/det;

    return result;
}

//Invert if only composed of rotations & translations
void mat4::AffineInvert(void)
{
    (*this)=GetAffineInverse();
}

mat4 mat4::GetAffineInverse(void) const
{
    //return the transpose of the rotation part
    //and the negative of the inverse rotated translation part
    return mat4(    entries[0],
                        entries[4],
                        entries[8],
                        0.0f,
                        entries[1],
                        entries[5],
                        entries[9],
                        0.0f,
                        entries[2],
                        entries[6],
                        entries[10],
                        0.0f,
                        -(entries[0]*entries[12]+entries[1]*entries[13]+entries[2]*entries[14]),
                        -(entries[4]*entries[12]+entries[5]*entries[13]+entries[6]*entries[14]),
                        -(entries[8]*entries[12]+entries[9]*entries[13]+entries[10]*entries[14]),
                        1.0f);
}

void mat4::AffineInvertTranspose(void)
{
    (*this)=GetAffineInverseTranspose();
}

mat4 mat4::GetAffineInverseTranspose(void) const
{
    //return the transpose of the rotation part
    //and the negative of the inverse rotated translation part
    //transposed
    return mat4(    entries[0],
                        entries[1],
                        entries[2],
                        -(entries[0]*entries[12]+entries[1]*entries[13]+entries[2]*entries[14]),
                        entries[4],
                        entries[5],
                        entries[6],
                        -(entries[4]*entries[12]+entries[5]*entries[13]+entries[6]*entries[14]),
                        entries[8],
                        entries[9],
                        entries[10],
                        -(entries[8]*entries[12]+entries[9]*entries[13]+entries[10]*entries[14]),
                        0.0f, 0.0f, 0.0f, 1.0f);
}

void mat4::SetTranslation(const vec3 & translation)
{
    LoadIdentity();

    SetTranslationPart(translation);
}

void mat4::SetScale(const vec3 & scaleFactor)
{
    LoadIdentity();

    entries[0]=scaleFactor.x();
    entries[5]=scaleFactor.y();
    entries[10]=scaleFactor.z();
}

void mat4::SetUniformScale(const float scaleFactor)
{
    LoadIdentity();

    entries[0]=entries[5]=entries[10]=scaleFactor;
}

void mat4::SetRotationAxisDeg(const double angle, const vec3 & axis) {
    SetRotationAxisRad(PI*angle/180.0f,axis);
}

void mat4::SetRotationAxisRad(const double angle, const vec3 & axis) {
    vec3 u = normalize(axis);

    float sinAngle=(float)sin(angle);
    float cosAngle=(float)cos(angle);
    float oneMinusCosAngle=1.0f-cosAngle;

    LoadIdentity();

    entries[0]=(u.x())*(u.x()) + cosAngle*(1-(u.x())*(u.x()));
    entries[4]=(u.x())*(u.y())*(oneMinusCosAngle) - sinAngle*u.z();
    entries[8]=(u.x())*(u.z())*(oneMinusCosAngle) + sinAngle*u.y();

    entries[1]=(u.x())*(u.y())*(oneMinusCosAngle) + sinAngle*u.z();
    entries[5]=(u.y())*(u.y()) + cosAngle*(1-(u.y())*(u.y()));
    entries[9]=(u.y())*(u.z())*(oneMinusCosAngle) - sinAngle*u.x();
    
    entries[2]=(u.x())*(u.z())*(oneMinusCosAngle) - sinAngle*u.y();
    entries[6]=(u.y())*(u.z())*(oneMinusCosAngle) + sinAngle*u.x();
    entries[10]=(u.z())*(u.z()) + cosAngle*(1-(u.z())*(u.z()));
}

void mat4::SetRotationX(const float angle)
{
    LoadIdentity();

    //entries[5]=(float)cos(PI*angle/180);
    //entries[6]=(float)sin(PI*angle/180);

    entries[5]=cosf(angle);
    entries[6]=sinf(angle);

    entries[9]=-entries[6];
    entries[10]=entries[5];
}

void mat4::SetRotationY(const float angle)
{
    LoadIdentity();

    entries[0]=cosf(angle);
    entries[2]=-sinf(angle);

    //entries[0]=(float)cosf(angle);
    //entries[2]=-(float)sinf(angle);

    entries[8]=-entries[2];
    entries[10]=entries[0];
}

void mat4::SetRotationZ(const float angle)
{
    LoadIdentity();

    //entries[0]=(float)cos(PI*angle/180);
    //entries[1]=(float)sin(PI*angle/180);
    
    entries[0]=cosf(angle);
    entries[1]=sinf(angle);

    entries[4]=-entries[1];
    entries[5]=entries[0];
}

void mat4::SetRotationEuler(const double angleX, const double angleY, const double angleZ)
{
    LoadIdentity();

    SetRotationPartEuler(angleX, angleY, angleZ);
}

void mat4::SetPerspectiveInfinite( float fovyInDegrees, float aspectRatio, float znear, float zfar ) {
    float e = 1/tanf(fovyInDegrees*((float)PI/180.0f)*0.5f);
    float a = aspectRatio;
    float n = znear;
    float epsilon = 0.000001f;

    entries[0] = e/a;
    entries[1] = 0.0f;
    entries[2] = 0.0f;
    entries[3] = 0.0f;

    entries[4] = 0.0f;
    entries[5] = e;
    entries[6] = 0.0f;
    entries[7] = 0.0f;

    entries[8] = 0.0f;
    entries[9] = 0.0f;
    entries[10] = epsilon-1.0f;
    entries[11] = -1.0f;

    entries[12] = 0.0f;
    entries[13] = 0.0f;
    entries[14] = (epsilon-2.0f)*n;
    entries[15] = 0.0f;
}

void mat4::SetPerspective( float fovyInDegrees, float aspectRatio, float znear, float zfar )
{
    float e = 1/tanf(fovyInDegrees*((float)PI/180.0f)*0.5f);
    float a = aspectRatio;
    float f = zfar;
    float n = znear;

    entries[0] = e/a;
    entries[1] = 0.0f;
    entries[2] = 0.0f;
    entries[3] = 0.0f;

    entries[4] = 0.0f;
    entries[5] = e;
    entries[6] = 0.0f;
    entries[7] = 0.0f;

    entries[8] = 0.0f;
    entries[9] = 0.0f;
    entries[10] = (-f-n)/(f-n);
    entries[11] = -1.0f;

    entries[12] = 0.0f;
    entries[13] = 0.0f;
    entries[14] = (-2.0f*f*n) / (f-n);
    entries[15] = 0.0f;
}

void mat4::SetOrtho( float left, float right, float bottom, float top, float _near, float _far )
{
    entries[0] = 2.0f / (right - left);
    entries[1] = 0.0f;
    entries[2] = 0.0f;
    entries[3] = 0.0f;

    entries[4] = 0.0f;
    entries[5] = 2.0f / (top - bottom);
    entries[6] = 0.0f;
    entries[7] = 0.0f;

    entries[8] = 0.0f;
    entries[9] = 0.0f;
    entries[10] = -2.0f / (_far - _near);
    entries[11] = 0.0f;

    entries[12] = -(right + left)/(right - left);
    entries[13] = -(top + bottom)/(top - bottom);
    entries[14] = -(_far + _near)/(_far - _near);
    entries[15] = 1.0f;
}


void mat4::SetTranslationPart(const vec3 & translation)
{
    SetColumn(3,translation);
}

void mat4::AddTranslation(const vec3 & translation)
{
    SetColumn(3,translation+GetColumnVec3(3));
}


vec3 mat4::GetTranslationPart() const
{
    return GetColumn(3).xyz();
}


void mat4::SetRotationPartEuler(const double angleX, const double angleY, const double angleZ)
{
    double cr = cos( PI*angleX/180 );
    double sr = sin( PI*angleX/180 );
    double cp = cos( PI*angleY/180 );
    double sp = sin( PI*angleY/180 );
    double cy = cos( PI*angleZ/180 );
    double sy = sin( PI*angleZ/180 );

    entries[0] = ( float )( cp*cy );
    entries[1] = ( float )( cp*sy );
    entries[2] = ( float )( -sp );

    double srsp = sr*sp;
    double crsp = cr*sp;

    entries[4] = ( float )( srsp*cy-cr*sy );
    entries[5] = ( float )( srsp*sy+cr*cy );
    entries[6] = ( float )( sr*cp );

    entries[8] = ( float )( crsp*cy+sr*sy );
    entries[9] = ( float )( crsp*sy-sr*cy );
    entries[10] = ( float )( cr*cp );
}

mat4 mat4::GetRotationPart() const
{
    mat4 temp;
    temp.entries[0] = entries[0];
    temp.entries[1] = entries[1];
    temp.entries[2] = entries[2];
    temp.entries[4] = entries[4];
    temp.entries[5] = entries[5];
    temp.entries[6] = entries[6];
    temp.entries[8] = entries[8];
    temp.entries[9] = entries[9];
    temp.entries[10] = entries[10];

    return temp;
}


void mat4::SetRotationPart( mat4 temp )
{
    entries[0] = temp.entries[0];
    entries[1] = temp.entries[1];
    entries[2] = temp.entries[2];
    entries[4] = temp.entries[4];
    entries[5] = temp.entries[5];
    entries[6] = temp.entries[6];
    entries[8] = temp.entries[8];
    entries[9] = temp.entries[9];
    entries[10] = temp.entries[10];
}

void mat4::NormalizeBases()
{
    float vec_length;
    int index = 0;
    for(int i=0; i<3; i++){
        vec_length = sqrt(square(entries[index+0])+
                                 square(entries[index+1])+
                                 square(entries[index+2]));
        entries[index+0] /= vec_length;
        entries[index+1] /= vec_length;
        entries[index+2] /= vec_length;
        index += 4;
    }
}

void mat4::OrthoNormalizeBases()
{
    vec3 col0(entries[0],entries[1],entries[2]);
    vec3 col1(entries[4],entries[5],entries[6]);
    vec3 col2(entries[8],entries[9],entries[10]);

    col0 = normalize(col0);
    col1 = normalize(col1-dot(col0,col1)*col0);
    col2 = normalize(col2-dot(col0,col2)*col0);

    SetColumn(0,col0);
    SetColumn(1,col1);
    SetColumn(2,col2);
}

void mat4::SetLookAt( const vec3& camera, const vec3& target, const vec3 &up )
{
    vec3 dir = normalize(target - camera);
    vec3 right = normalize(cross(dir, up));
    vec3 new_up = normalize(cross(right, dir));

    mat4 rotation;
    rotation.SetRow(0, right);
    rotation.SetRow(1, new_up);
    rotation.SetRow(2, -dir);
    
    mat4 translation;
    translation.SetTranslationPart(-camera);

    (*this) = rotation * translation;
}

void mat4::AddRotation( const vec3& rotation )
{
    float angle = length(rotation);
    if(angle == 0.0f){
        return;
    }
    const vec3 axis = rotation / angle; 
    vec3 bases[3];
    bases[0] = GetColumnVec3(0);
    bases[1] = GetColumnVec3(1);
    bases[2] = GetColumnVec3(2);
    bases[0] = AngleAxisRotationRadian(bases[0], axis, angle);
    bases[1] = AngleAxisRotationRadian(bases[1], axis, angle);
    bases[2] = AngleAxisRotationRadian(bases[2], axis, angle);
    SetColumn(0, bases[0]);
    SetColumn(1, bases[1]);
    SetColumn(2, bases[2]);
}

bool operator<( const mat4 &a, const mat4 &b )
{
    for(int i=0; i<16; i++){
        if(a.entries[i]<b.entries[i]){
            return true;
        }
    }
    return false;
}


// Adapted from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToAngle/index.htm
vec4 AxisAngleFromMat4( const mat4& m )
{
    //return vec4(1.0f,0.0f,0.0f,0.0f);

    float angle,x,y,z; // variables for result
    float epsilon = 0.00001f; // margin to allow for rounding errors
    float epsilon2 = 0.1f; // margin to distinguish between 0 and 180 degrees
    // optional check that input is pure rotation, 'isRotationMatrix' is defined at:
    // http://www.euclideanspace.com/maths/algebra/matrix/orthogonal/rotation/
    if ((fabs(m(0,1)-m(1,0))< epsilon)
        && (fabs(m(0,2)-m(2,0))< epsilon)
        && (fabs(m(1,2)-m(2,1))< epsilon)) {
            // singularity found
            // first check for identity matrix which must have +1 for all terms
            //  in leading diagonaland zero in other terms
            if ((fabs(m(0,1)+m(1,0)) < epsilon2)
                && (fabs(m(0,2)+m(2,0)) < epsilon2)
                && (fabs(m(1,2)+m(2,1)) < epsilon2)
                && (fabs(m(0,0)+m(1,1)+m(2,2)-3) < epsilon2)) {
                    // this singularity is identity matrix so angle = 0
                    return vec4(1.0f,0.0f,0.0f,0.0f); // zero angle, arbitrary axis
            }
            // otherwise this singularity is angle = 180
            angle = (float)PI;
            float xx = (m(0,0)+1.0f)/2.0f;
            float yy = (m(1,1)+1.0f)/2.0f;
            float zz = (m(2,2)+1.0f)/2.0f;
            float xy = (m(0,1)+m(1,0))/4.0f;
            float xz = (m(0,2)+m(2,0))/4.0f;
            float yz = (m(1,2)+m(2,1))/4.0f;
            if ((xx > yy) && (xx > zz)) { // m(0,0) is the largest diagonal term
                if (xx< epsilon) {
                    x = 0.0f;
                    y = 0.7071f;
                    z = 0.7071f;
                } else {
                    x = sqrtf(xx);
                    y = xy/x;
                    z = xz/x;
                }
            } else if (yy > zz) { // m(1,1) is the largest diagonal term
                if (yy< epsilon) {
                    x = 0.7071f;
                    y = 0.0f;
                    z = 0.7071f;
                } else {
                    y = sqrtf(yy);
                    x = xy/y;
                    z = yz/y;
                }    
            } else { // m(2,2) is the largest diagonal term so base result on this
                if (zz< epsilon) {
                    x = 0.7071f;
                    y = 0.7071f;
                    z = 0.0f;
                } else {
                    z = sqrtf(zz);
                    x = xz/z;
                    y = yz/z;
                }
            }
            return vec4(x,y,z,angle); // return 180 deg rotation
    }
    // as we have reached here there are no singularities so we can handle normally
    float s = sqrtf((m(2,1) - m(1,2))*(m(2,1) - m(1,2))
        +(m(0,2) - m(2,0))*(m(0,2) - m(2,0))
        +(m(1,0) - m(0,1))*(m(1,0) - m(0,1))); // used to normalise
    if (fabs(s) < 0.001) s=1; 
    // prevent divide by zero, should not happen if matrix is orthogonal and should be
    // caught by singularity test above, but I've left it in just in case
    float test = ( m(0,0) + m(1,1) + m(2,2) - 1.0f)/2.0f;
    angle = acosf(test);
    x = (m(2,1) - m(1,2))/s;
    y = (m(0,2) - m(2,0))/s;
    z = (m(1,0) - m(0,1))/s;
    return vec4(x,y,z,angle);
}

mat4 orthonormalize( const mat4& mat )
{
    vec3 basis1(mat(0,0),mat(0,1),mat(0,2));
    vec3 basis2(mat(1,0),mat(1,1),mat(1,2));
    vec3 basis3(mat(2,0),mat(2,1),mat(2,2));

    basis1 = normalize(basis1);

    basis2 -= basis1 * dot(basis1,basis2);
    basis2 = normalize(basis2);

    basis3 -= basis1 * dot(basis1,basis3);
    basis3 -= basis2 * dot(basis2,basis3);
    basis3 = normalize(basis3);

    mat4 to_return = mat;
    to_return(0,0) = basis1.entries[0];
    to_return(0,1) = basis1.entries[1];
    to_return(0,2) = basis1.entries[2];
    to_return(1,0) = basis2.entries[0];
    to_return(1,1) = basis2.entries[1];
    to_return(1,2) = basis2.entries[2];
    to_return(2,0) = basis3.entries[0];
    to_return(2,1) = basis3.entries[1];
    to_return(2,2) = basis3.entries[2];
    
    return to_return;
}
