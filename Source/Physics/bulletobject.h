//-----------------------------------------------------------------------------
//           Name: bulletobject.h
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

#include <Math/quaternions.h>

#include <opengl.h>

#include <vector>
#include <memory>

class btRigidBody;
class btCollisionShape;
class btTransform;
class Object;
struct CollideInfo;

enum CDLItype { _CDLI_CAPSULE };

class btTriangleIndexVertexArray;
struct btTriangleInfoMap;
struct ShapeDisposalData {
    std::vector<int> faces;
    btTriangleIndexVertexArray *index_vert_array;
    btTriangleInfoMap *triangle_info_map;
    ShapeDisposalData();
    ~ShapeDisposalData();
};

typedef std::shared_ptr<btCollisionShape> SharedShapePtr;

class BulletObject {
public:
    mat4 transform;
    mat4 old_transform;
    std::vector<mat4> transform_history;
    btRigidBody *body;
    SharedShapePtr shape;
    std::unique_ptr<ShapeDisposalData> shape_disposal_data;
    bool visible;
    vec3 com_offset;
    short collision_group;
    short collision_flags;
    Object *owner_object;
    vec3 color;
    bool linked;
    bool keep_history;

    BulletObject();
    ~BulletObject();

    void Collided(const vec3& pos, float impulse, const CollideInfo &collide_info);
    void SetTransform( const vec4 &position, const mat4 &rotation, const vec4 &scale );
    void SetTransform( const mat4 &transform );
    void SetPosition( const vec3 &position);
    void ApplyTransform(const mat4& transform);
    void SetGravity(bool _gravity);
    void SetDamping( float damping );
    void SetDamping( float lin_damping, float ang_damping );
    void ApplyTorque(const vec3 &torque);
    vec3 GetInterpPosition();
    mat4 GetInterpRotation();
    void SetRotation( const mat4 &rotation);
    void SetRotation( const quaternion &rotation);
    void ClearVelocities();
    void SetAngularVelocity(const vec3& vel);
    void SetLinearVelocity(const vec3& vel);
    vec3 GetPosition() const;
    vec3 GetLinearVelocity() const;
    vec3 GetAngularVelocity() const;
    mat4 GetRotation() const;
    vec3 ObjectToWorld(const vec3 &point);
    void ApplyForceAtWorldPoint(const vec3 &point, const vec3 &impulse);
    vec3 WorldToObject(const vec3 &world_point);
    void SetVisibility(const bool _visible);
    bool IsVisible() const;
    void Activate();
    void Freeze();
    void UnFreeze();
    void FixDiscontinuity();
    void Dispose();
    float GetMomentOfInertia(const vec3& axis);
    void CopyObjectTransform(const BulletObject* other);
    void MixObjectTransform( const BulletObject* other, float how_much );
    float GetMass();
    void SetMass(float mass);
    void GetDisplayTransform(btTransform* display_transform);
    quaternion GetQuatRotation();
    void GetQuatDeltaRotation() const;
    void UpdateTransform();
    void SetPositionAndVel( const vec3 &position, int frames = 1);
    void SetRotationAndVel( const mat4 &rotation, int frames = 1);
    void SetRotationAndVel( const quaternion &rotation, int frames = 1);
    void AddAngularVelocity( const vec3& vel );
    void AddLinearVelocity( const vec3& vel );
    bool IsActive();
    void MixObjectVel( const BulletObject* other, float how_much );
    void CopyObjectVel( const BulletObject* other );
    mat4 GetTransform() const;
    vec3 GetInterpPositionX( int num, int progress );
    mat4 GetInterpRotationX( int num, int progress );
    void Sleep();
    void SetMargin( float _margin );
    vec3 GetVelocityAtLocalPoint(const vec3& point);
    void NoSleep();
    void CanSleep();
    void CheckForNAN();
    float GetMargin( );
    void SetShape(SharedShapePtr _shape);
    void SetColShape(btCollisionShape *_shape);
    vec3 GetHistoryInterpPositionX( int num, int progress, float offset );
    mat4 GetHistoryInterpRotationX( int num, int progress, float offset );
    vec3 GetInterpWeightPosition(float weight);
    mat4 GetInterpWeightRotation(float weight);
};

void ValidateTransform(const btTransform &bt_transform);
