//-----------------------------------------------------------------------------
//           Name: bonetransform.cpp
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "bonetransform.h"

#include <Math/enginemath.h>
#include <Math/vec3math.h>
#include <Math/quaternions.h>

#include <cmath>

const BoneTransform& BoneTransform::operator=( const mat4 &other )
{
    rotation = QuaternionFromMat4(other);
    origin = other.GetTranslationPart();
    return (*this);
}

BoneTransform invert( const BoneTransform& transform )
{
    BoneTransform inverted;
    inverted.rotation = invert(transform.rotation);
    inverted.origin = inverted.rotation * -transform.origin;
    return inverted;
}

BoneTransform operator*( const BoneTransform& a, const BoneTransform& b )
{
    BoneTransform result;
    result.rotation = a.rotation * b.rotation;
    result.origin = a.rotation * b.origin + a.origin;
    //result = a.GetMat4() * b.GetMat4();
    return result;
}

BoneTransform operator*( const quaternion& a, const BoneTransform& b )
{
    BoneTransform result;
    result.rotation = a * b.rotation;
    result.origin = a * b.origin;
    return result;
}

vec3 operator*( const BoneTransform& a, const vec3& b )
{
    vec3 result = a.rotation * b + a.origin;
    return result;
}

BoneTransform mix( const BoneTransform &a, const BoneTransform &b, float alpha )
{
    BoneTransform result;
    result.origin = mix(a.origin, b.origin, alpha);
    result.rotation = mix(a.rotation, b.rotation, alpha);
    return result;
}

mat4 BoneTransform::GetMat4() const
{
    mat4 matrix = Mat4FromQuaternion(rotation);
    matrix.SetTranslationPart(origin);
    return matrix;
}

BoneTransform::BoneTransform( const mat4 &other )
{
    (*this) = other;
}

BoneTransform::BoneTransform()
{}

BoneTransform ApplyParentRotations(const std::vector<BoneTransform> &matrices,
                                   int id, 
                                   const std::vector<int> &parents) 
{
    int parent_id = parents[id];
    if(parent_id == -1){
        return matrices[id];    
    } else {
        return ApplyParentRotations(matrices,parent_id,parents) *
               matrices[id];
    }
}

bool operator==(const BoneTransform& a, const BoneTransform& b) {
    return (a.origin == b.origin && a.rotation == b.rotation);
}

void BoneTransform::LoadIdentity() {
    origin = vec3(0.0f);
    rotation = quaternion();
}
