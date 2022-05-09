//-----------------------------------------------------------------------------
//           Name: mat4.h
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

class mat4 {
   public:
    mat4(float val = 1.0f);
    mat4(float e0, float e1, float e2, float e3,
         float e4, float e5, float e6, float e7,
         float e8, float e9, float e10, float e11,
         float e12, float e13, float e14, float e15);
    mat4(const float* rhs);
    mat4(const mat4& rhs);
    ~mat4() {}  // empty

    vec4 GetRow(int position) const;
    vec4 GetColumn(int position) const;
    vec3 GetRowVec3(int position) const;
    vec3 GetColumnVec3(int position) const;

    void SetRow(int position, const vec4& vec);
    void SetRow(int position, const vec3& vec);
    void SetColumn(int position, const vec4& vec);
    void SetColumn(int position, const vec3& vec);

    void LoadIdentity(void);

    // binary operators
    mat4 operator+(const mat4& rhs) const;
    mat4 operator-(const mat4& rhs) const;
    mat4 operator*(const mat4& rhs) const;
    mat4 operator*(const float rhs) const;
    mat4 operator/(const float rhs) const;
    friend mat4 operator*(float scaleFactor, const mat4& rhs);

    bool operator==(const mat4& rhs) const;
    bool operator!=(const mat4& rhs) const;

    // self-add etc
    void operator+=(const mat4& rhs);
    void operator-=(const mat4& rhs);
    void operator*=(const mat4& rhs);
    void operator*=(const float rhs);
    void operator/=(const float rhs);

    // unary operators
    mat4 operator-(void) const;
    mat4 operator+(void) const { return (*this); }

    void ToVec4(vec4* vector1, vec4* vector2, vec4* vector3, vec4* vector4) const;

    // multiply a vector by this matrix
    vec4 operator*(const vec4& rhs) const;
    vec3 operator*(const vec3& rhs) const;

    // rotate a 3d vector by rotation part
    void Rotatevec3(vec3& rhs) const { rhs = GetRotatedvec3(rhs); }

    void InverseRotatevec3(vec3& rhs) const { rhs = GetInverseRotatedvec3(rhs); }

    vec3 GetRotatedvec3(const vec3& rhs) const;
    vec3 GetInverseRotatedvec3(const vec3& rhs) const;

    // translate a 3d vector by translation part
    void Translatevec3(vec3& rhs) const { rhs = GetTranslatedvec3(rhs); }

    void InverseTranslatevec3(vec3& rhs) const { rhs = GetInverseTranslatedvec3(rhs); }

    vec3 GetTranslatedvec3(const vec3& rhs) const;
    vec3 GetInverseTranslatedvec3(const vec3& rhs) const;

    // Other methods
    float GetDeterminant();
    void InvertTranspose(void);
    mat4 GetInverseTranspose(void) const;

    // Inverse of a rotation/translation only matrix
    void AffineInvert(void);
    mat4 GetAffineInverse(void) const;
    void AffineInvertTranspose(void);
    mat4 GetAffineInverseTranspose(void) const;

    void SetOrtho(float left, float right, float bottom, float top, float near, float far);
    void SetPerspective(float fovyInDegrees, float aspectRatio, float znear, float zfar);
    void SetPerspectiveInfinite(float fovyInDegrees, float aspectRatio, float znear, float zfar);

    // set to perform an operation on space - removes other entries
    void SetTranslation(const vec3& translation);
    void SetScale(const vec3& scaleFactor);
    void SetUniformScale(const float scaleFactor);
    void SetRotationAxisDeg(const double angle, const vec3& axis);
    void SetRotationAxisRad(const double angle, const vec3& axis);
    void SetRotationX(const float angle);
    void SetRotationY(const float angle);
    void SetRotationZ(const float angle);
    void SetRotationEuler(const double angleX, const double angleY, const double angleZ);

    // set parts of the matrix
    void SetTranslationPart(const vec3& translation);
    vec3 GetTranslationPart() const;
    mat4 GetRotationPart() const;
    void SetRotationPart(mat4 rot);
    void SetRotationPartEuler(const double angleX, const double angleY, const double angleZ);
    void SetRotationPartEuler(const vec3& rotations) {
        SetRotationPartEuler((double)rotations.x(), (double)rotations.y(), (double)rotations.z());
    }

    void NormalizeBases();
    void OrthoNormalizeBases();

    // cast to pointer to a (float *) for glGetFloatv etc
    operator float*() const { return (float*)this; }
    operator const float*() const { return (const float*)this; }

    inline float& operator()(int i, int j) { return entries[i + j * 4]; }
    inline const float& operator()(int i, int j) const { return entries[i + j * 4]; }

    inline float& operator[](int i) { return entries[i]; }
    inline const float& operator[](int i) const { return entries[i]; }
    void SetLookAt(const vec3& camera, const vec3& target, const vec3& up);
    void AddTranslation(const vec3& translation);
    void FromVec4(const vec4& vector1, const vec4& vector2, const vec4& vector3, const vec4& vector4);
    void AddRotation(const vec3& rotation);
    // member variables
    float entries[16];
};

mat4 invert(const mat4& mat);
mat4 transpose(const mat4& mat);
mat4 orthonormalize(const mat4& mat);

vec4 AxisAngleFromMat4(const mat4& mat);

bool operator<(const mat4& a, const mat4& b);

inline std::ostream& operator<<(std::ostream& os, const mat4& v) {
    os << "mat4(";
    os << "[" << v[0] << "," << v[4] << "," << v[8] << "," << v[12] << "]";
    os << "[" << v[1] << "," << v[5] << "," << v[9] << "," << v[13] << "]";
    os << "[" << v[2] << "," << v[6] << "," << v[10] << "," << v[14] << "]";
    os << "[" << v[3] << "," << v[7] << "," << v[11] << "," << v[15] << "]";
    os << ")";
    return os;
}
