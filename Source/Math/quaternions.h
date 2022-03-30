//-----------------------------------------------------------------------------
//           Name: quaternions.h
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

#include <Math/vec3math.h>
#include <Math/mat4.h>
#include <Math/mat3.h>

class quaternion {
public:
    float entries[4];
    
    quaternion& operator+=( const quaternion &b );

    quaternion(bool Degree_Flag, vec3 Euler);
    quaternion(float x, float y, float z, float w);
    quaternion(vec4 AnAx); // vec4(vec3(axis), float angle [radians])
    quaternion(const quaternion &other);    
    quaternion();    
	float& operator[](int which){ return entries[which]; }
	const float& operator[](int which)const{ return entries[which]; }
};

float Length2(const quaternion& quat);
vec4 Quat_2_AA(quaternion Quat);
quaternion normalize(const quaternion &quat);
quaternion Quat_Mult(const quaternion &q1, const quaternion &q2);
quaternion QNormalize(quaternion Quat);
vec3 Quat2Vector(quaternion Quat);
void QuaternionInvert(quaternion * quat);
void QuaternionToAxisAngle(quaternion quat, vec3 * axis, float * angle);
quaternion QuaternionFromAxisAngle(vec3 axis, float angle);

quaternion QuaternionFromMat4(const mat4 &m);
mat4 Mat4FromQuaternion(const quaternion &Quat);
mat3 Mat3FromQuaternion(const quaternion &Quat);

quaternion invert(const quaternion &quat);
const quaternion operator*(const quaternion &a,
                           const quaternion &b);
bool operator!=(const quaternion &a,
                      const quaternion &b);
bool operator==(const quaternion &a,
                      const quaternion &b);
const quaternion operator+(const quaternion &a,
                           const quaternion &b);
const vec3 operator*(const quaternion &a,
                     const vec3 &b);
const quaternion operator*(const quaternion &a,
                            float b);
float dot( const quaternion &a, const quaternion &b );

quaternion Slerp(quaternion start, quaternion end, float t);
inline quaternion mix( const quaternion &a, const quaternion &b, float alpha )
{return Slerp(a,b,alpha);}

vec3 ASMult( quaternion a, vec3 b );
quaternion invert_by_val(quaternion quat);
void QuaternionMultiplyVector(const quaternion * quat, vec3 * vector);


inline vec3 QuaternionMultiplyVectorFast(const quaternion& quat, const vec3& vector) {
    // Adapted from https://github.com/g-truc/glm/blob/master/glm/detail/type_quat.inl
    // Also from Fabien Giesen, according to - https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
    vec3 quat_vector = *(vec3*)&quat.entries[0];
    vec3 uv = cross(quat_vector, vector);
    vec3 uuv = cross(quat_vector, uv);
    return vector + ((uv * quat[3]) + uuv) * 2.0f;
}

inline vec3 TransformVec3(const vec3& scale, const quaternion& rotation, const vec3& translation, const vec3& vertex) {
    vec3 result = vertex;
    result *= scale;
    result = QuaternionMultiplyVectorFast(rotation, result);
    result += translation;
    return result;
}


vec3 QuaternionToEuler(const quaternion& quat);
quaternion EulerToQuaternion(const vec3& euler);

inline std::ostream& operator<<(std::ostream& os, const quaternion& v )
{
    os << "quat(" << v[0] << "," << v[1] << "," << v[2] << "," << v[3] << ")";
    return os;
}
