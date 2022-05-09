//-----------------------------------------------------------------------------
//           Name: bulletobject.cpp
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
#include <Physics/bulletobject.h>
#include <Physics/physics.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>
#include <Math/quaternions.h>

#include <Graphics/geometry.h>
#include <Graphics/graphics.h>

#include <Internal/timer.h>
#include <LinearMath/btTransformUtil.h>
#include <Objects/object.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <btBulletDynamicsCommon.h>

extern Timer game_timer;

mat4 SafeGetOpenGLMatrix(const btTransform &bt_transform) {
    // Make sure that transform mat4 is 16-byte aligned
    // since Bullet may be reading to it from SSE intrinsics
    char aligned_mem[sizeof(mat4) + 16];
    uintptr_t first = ((uintptr_t)aligned_mem) % 16;
    mat4 *mat = (mat4 *)&aligned_mem[16 - first];
    bt_transform.getOpenGLMatrix(mat->entries);
    return *mat;
}

void ValidateTransform(const btTransform &bt_transform) {
    /*const btMatrix3x3& basis = bt_transform.getBasis();
    for(int i=0; i<3; ++i){
        const btVector3& row = basis[i];
        for(int j=0; j<3; ++j){
            const btScalar &val = row[j];
            if(val != val){
                DisplayError("Error", "Invalid transform!");
            }
        }
    }
    const btVector3& origin = bt_transform.getOrigin();
    for(int j=0; j<3; ++j){
        const btScalar &val = origin[j];
        if(val != val){
            DisplayError("Error", "Invalid transform!");
        }
    }*/
}

void BulletObject::SetPosition(const vec3 &position) {
    btTransform bt_transform = body->getWorldTransform();
    vec3 new_pos = position + GetRotation() * com_offset;
    bt_transform.setOrigin(btVector3(new_pos[0],
                                     new_pos[1],
                                     new_pos[2]));
    ValidateTransform(bt_transform);
    body->setWorldTransform(bt_transform);
}

void BulletObject::SetMargin(float _margin) {
    shape->setMargin(_margin);
}

float BulletObject::GetMargin() {
    return shape->getMargin();
}

void BulletObject::SetRotation(const mat4 &rotation) {
    btVector3 old_origin = body->getWorldTransform().getOrigin();
    btTransform new_transform;
    new_transform.setFromOpenGLMatrix(rotation.entries);
    new_transform.setOrigin(old_origin);
    ValidateTransform(new_transform);
    body->setWorldTransform(new_transform);
}

void BulletObject::SetRotation(const quaternion &rotation) {
    btVector3 old_origin = body->getWorldTransform().getOrigin();
    btTransform new_transform;
    new_transform.setRotation(btQuaternion(rotation.entries[0], rotation.entries[1], rotation.entries[2], rotation.entries[3]));
    new_transform.setOrigin(old_origin);
    ValidateTransform(new_transform);
    body->setWorldTransform(new_transform);
}

void BulletObject::SetPositionAndVel(const vec3 &position, int frames) {
    vec3 vel = (position - GetPosition()) / game_timer.timestep;
    vel /= (float)frames;
    // SetPosition(position);
    SetLinearVelocity(vel);
}

void BulletObject::SetRotationAndVel(const mat4 &rotation, int frames) {
    btVector3 old_origin = body->getWorldTransform().getOrigin();
    btTransform new_transform;
    new_transform.setFromOpenGLMatrix(rotation.entries);
    new_transform.setOrigin(old_origin);

    btVector3 axis;
    btScalar angle;
    btTransformUtil::calculateDiffAxisAngle(body->getWorldTransform(),
                                            new_transform,
                                            axis,
                                            angle);
    btVector3 angVel = axis * angle / game_timer.timestep;
    angVel /= (float)frames;

    body->setAngularVelocity(angVel);

    // body->setWorldTransform(new_transform);
}

void BulletObject::SetRotationAndVel(const quaternion &rotation, int frames) {
    btVector3 old_origin = body->getWorldTransform().getOrigin();
    btTransform new_transform;
    new_transform.setRotation(btQuaternion(rotation.entries[0], rotation.entries[1], rotation.entries[2], rotation.entries[3]));
    new_transform.setOrigin(old_origin);

    btVector3 axis;
    btScalar angle;
    btTransformUtil::calculateDiffAxisAngle(body->getWorldTransform(),
                                            new_transform,
                                            axis,
                                            angle);
    btVector3 angVel = axis * angle / game_timer.timestep;
    angVel /= (float)frames;

    body->setAngularVelocity(angVel);

    // body->setWorldTransform(new_transform);
}

void BulletObject::SetTransform(const vec4 &position, const mat4 &rotation, const vec4 &scale) {
    btTransform bt_transform;
    bt_transform.setIdentity();
    bt_transform.setFromOpenGLMatrix((btScalar *)rotation.entries);
    vec3 new_pos = position.xyz() + GetRotation() * com_offset;
    bt_transform.setOrigin(btVector3(new_pos[0],
                                     new_pos[1],
                                     new_pos[2]));
    shape->setLocalScaling(btVector3(scale[0],
                                     scale[1],
                                     scale[2]));
    ValidateTransform(bt_transform);
    body->setWorldTransform(bt_transform);
}

void BulletObject::SetTransform(const mat4 &transform) {
    btTransform bt_transform;
    bt_transform.setFromOpenGLMatrix((btScalar *)transform.entries);
    ValidateTransform(bt_transform);
    body->setWorldTransform(bt_transform);
}

void BulletObject::ApplyTransform(const mat4 &transform) {
    mat4 old_transform_mat4 = SafeGetOpenGLMatrix(body->getWorldTransform());
    old_transform_mat4 = transform * old_transform_mat4;
    btTransform new_transform;
    new_transform.setFromOpenGLMatrix(old_transform_mat4.entries);
    ValidateTransform(new_transform);
    body->setWorldTransform(new_transform);
}

void BulletObject::SetGravity(bool gravity) {
    if (!gravity) {
        body->setGravity(btVector3(0.0f, 0.0f, 0.0f));
    } else {
        const vec3 &grav = Physics::Instance()->gravity;
        body->setGravity(btVector3(grav[0], grav[1], grav[2]));
    }
}

vec3 BulletObject::GetVelocityAtLocalPoint(const vec3 &point) {
    btVector3 vel = body->getVelocityInLocalPoint(
        btVector3(point[0], point[1], point[2]));
    return vec3(vel[0], vel[1], vel[2]);
}

void BulletObject::SetDamping(float damping) {
    body->setDamping(damping, damping);
}

void BulletObject::SetDamping(float lin_damping, float ang_damping) {
    body->setDamping(lin_damping, ang_damping);
}

vec3 BulletObject::GetInterpPosition() {
    return mix(old_transform.GetTranslationPart(),
               transform.GetTranslationPart(),
               game_timer.GetInterpWeight());
}

mat4 BulletObject::GetInterpRotation() {
    return mix(old_transform.GetRotationPart(),
               transform.GetRotationPart(),
               game_timer.GetInterpWeight());
}

vec3 BulletObject::GetInterpWeightPosition(float weight) {
    return mix(old_transform.GetTranslationPart(),
               transform.GetTranslationPart(),
               weight);
}

mat4 BulletObject::GetInterpWeightRotation(float weight) {
    return mix(old_transform.GetRotationPart(),
               transform.GetRotationPart(),
               weight);
}

vec3 BulletObject::GetInterpPositionX(int num, int progress) {
    return mix(old_transform.GetTranslationPart(),
               transform.GetTranslationPart(),
               game_timer.GetInterpWeightX(num, progress));
}

mat4 BulletObject::GetInterpRotationX(int num, int progress) {
    return mix(old_transform.GetRotationPart(),
               transform.GetRotationPart(),
               game_timer.GetInterpWeightX(num, progress));
}

vec3 BulletObject::GetHistoryInterpPositionX(int num, int progress, float offset) {
    float t_e = game_timer.timestep_error - offset;
    float mult = (1.0f / (float)num);
    float temp_timestep_error = mult * (float)progress + t_e * mult;
    unsigned base = 0;
    while (temp_timestep_error < 0) {
        ++base;
        temp_timestep_error += 1.0f;
    }
    if (base + 1 >= transform_history.size()) {
        transform_history.resize(base + 2, transform_history.back());
    }
    return transform_history[base].GetTranslationPart() * temp_timestep_error + transform_history[base + 1].GetTranslationPart() * (1.0f - temp_timestep_error);
}

mat4 BulletObject::GetHistoryInterpRotationX(int num, int progress, float offset) {
    float t_e = game_timer.timestep_error - offset;
    float mult = (1.0f / (float)num);
    float temp_timestep_error = mult * (float)progress + t_e * mult;
    unsigned base = 0;
    while (temp_timestep_error < 0) {
        ++base;
        temp_timestep_error += 1.0f;
    }
    if (base + 1 >= transform_history.size()) {
        transform_history.resize(base + 2, transform_history.back());
    }
    return transform_history[base].GetRotationPart() * temp_timestep_error + transform_history[base + 1].GetRotationPart() * (1.0f - temp_timestep_error);
}

void BulletObject::ClearVelocities() {
    body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
}

void BulletObject::SetAngularVelocity(const vec3 &vel) {
    body->setAngularVelocity(btVector3(vel[0], vel[1], vel[2]));
}

void BulletObject::SetLinearVelocity(const vec3 &vel) {
    body->setLinearVelocity(btVector3(vel[0], vel[1], vel[2]));
}

void BulletObject::AddAngularVelocity(const vec3 &vel) {
    body->setAngularVelocity(body->getAngularVelocity() +
                             btVector3(vel[0], vel[1], vel[2]));
}

void BulletObject::AddLinearVelocity(const vec3 &vel) {
    body->setLinearVelocity(body->getLinearVelocity() +
                            btVector3(vel[0], vel[1], vel[2]));
}

vec3 BulletObject::GetPosition() const {
    btVector3 bt_origin = body->getWorldTransform().getOrigin();
    vec3 com_display_offset = GetRotation() * -com_offset;
    return vec3(bt_origin[0], bt_origin[1], bt_origin[2]) + com_display_offset;
}

float BulletObject::GetMass() {
    return 1.0f / body->getInvMass();
}

vec3 BulletObject::GetLinearVelocity() const {
    btVector3 vec = body->getLinearVelocity();
    return vec3(vec[0], vec[1], vec[2]);
}

mat4 BulletObject::GetRotation() const {
    mat4 rotation = SafeGetOpenGLMatrix(body->getWorldTransform());
    return rotation.GetRotationPart();
}

mat4 BulletObject::GetTransform() const {
    return SafeGetOpenGLMatrix(body->getWorldTransform());
}

quaternion BulletObject::GetQuatRotation() {
    btQuaternion quat = body->getWorldTransform().getRotation();
    return quaternion(vec4(quat.getX(), quat.getY(), quat.getZ(), quat.getW()));
}

vec3 BulletObject::ObjectToWorld(const vec3 &point) {
    transform = SafeGetOpenGLMatrix(body->getWorldTransform());
    return transform * point;
}

void BulletObject::ApplyForceAtWorldPoint(const vec3 &point, const vec3 &impulse) {
    vec3 local_point = WorldToObject(point);
    body->applyForce(btVector3(impulse[0], impulse[1], impulse[2]),
                     btVector3(local_point[0], local_point[1], local_point[2]));
    body->activate();
}

vec3 BulletObject::WorldToObject(const vec3 &world_point) {
    transform = SafeGetOpenGLMatrix(body->getWorldTransform());
    return invert(transform) * world_point;
}

void BulletObject::SetVisibility(const bool _visible) {
    visible = _visible;
}

bool BulletObject::IsVisible() const {
    return visible;
}

void BulletObject::Activate() {
    body->activate();
}

void BulletObject::Freeze() {
    body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT |
                            btCollisionObject::CF_NO_CONTACT_RESPONSE);
    body->setActivationState(ISLAND_SLEEPING);
}

void BulletObject::Sleep() {
    body->setActivationState(ISLAND_SLEEPING);
}

void BulletObject::NoSleep() {
    body->setActivationState(DISABLE_DEACTIVATION);
}

void BulletObject::CanSleep() {
    body->forceActivationState(ACTIVE_TAG);
    body->activate(true);
}

void BulletObject::UnFreeze() {
    body->setCollisionFlags(0);
    body->activate();
}

void BulletObject::FixDiscontinuity() {
    btTransform bt_transform;
    GetDisplayTransform(&bt_transform);
    transform = SafeGetOpenGLMatrix(bt_transform);
    old_transform = transform;
}

void BulletObject::Dispose() {
    if (body->getMotionState()) {
        delete body->getMotionState();
    }
    delete body;
    body = NULL;
    shape.reset();
}

void BulletObject::SetShape(SharedShapePtr _shape) {
    shape = _shape;
    body->setCollisionShape(shape.get());
}

BulletObject::BulletObject() : body(NULL),
                               com_offset(0.0f),
                               owner_object(NULL),
                               color(1.0f),
                               keep_history(false) {}

BulletObject::~BulletObject() {
    LOG_ASSERT(body == NULL);
    // These should be NULL if Dispose() was called correctly
}

void BulletObject::CopyObjectTransform(const BulletObject *other) {
    // btTransform transform = other->body->getWorldTransform();
    // transform.setOrigin(transform.getOrigin()*0.5f);
    // body->setWorldTransform(transform);
    ValidateTransform(other->body->getWorldTransform());
    body->setWorldTransform(other->body->getWorldTransform());
}

void BulletObject::MixObjectTransform(const BulletObject *other,
                                      float how_much) {
    const btTransform &other_transform = other->body->getWorldTransform();
    btTransform this_transform = body->getWorldTransform();
    /*
    btQuaternion q1 = this_transform.getRotation();
    btQuaternion q2 = other_transform.getRotation();
    */
    btQuaternion result = slerp(this_transform.getRotation(),
                                other_transform.getRotation(),
                                how_much);
    if (result[0] != result[0]) {
        /*
        btQuaternion result = slerp(this_transform.getRotation(),
            other_transform.getRotation(),
            how_much);
        */
        return;
    }
    this_transform.setRotation(result);
    this_transform.setOrigin(lerp(this_transform.getOrigin(),
                                  other_transform.getOrigin(),
                                  how_much));
    ValidateTransform(this_transform);
    body->setWorldTransform(this_transform);
}

void BulletObject::MixObjectVel(const BulletObject *other,
                                float how_much) {
    const btVector3 &other_lin_vel = other->body->getLinearVelocity();
    const btVector3 &other_ang_vel = other->body->getAngularVelocity();
    const btVector3 &this_lin_vel = body->getLinearVelocity();
    const btVector3 &this_ang_vel = body->getAngularVelocity();

    body->setLinearVelocity(this_lin_vel * (1.0f - how_much) + other_lin_vel * how_much);
    body->setAngularVelocity(this_ang_vel * (1.0f - how_much) + other_ang_vel * how_much);
}

void BulletObject::CopyObjectVel(const BulletObject *other) {
    body->setLinearVelocity(other->body->getLinearVelocity());
    body->setAngularVelocity(other->body->getAngularVelocity());
}

void BulletObject::GetDisplayTransform(btTransform *display_transform) {
    (*display_transform) = body->getWorldTransform();
    btVector3 scale = shape->getLocalScaling();
    scale[0] = fabs(scale[0]);
    scale[1] = fabs(scale[1]);
    scale[2] = fabs(scale[2]);
    vec3 com_display_offset = GetRotation() * -com_offset;
    display_transform->setOrigin(display_transform->getOrigin() +
                                 btVector3(com_display_offset[0],
                                           com_display_offset[1],
                                           com_display_offset[2]));
    /*btMatrix3x3 basis = display_transform->getBasis();
    basis[0] *= scale[0];
    basis[4] *= scale[1];
    basis[8] *= scale[2];
    display_transform->setBasis(basis);*/
    display_transform->setBasis(display_transform->getBasis().scaled(scale));
}

vec3 BulletObject::GetAngularVelocity() const {
    btVector3 vec = body->getAngularVelocity();
    return vec3(vec[0], vec[1], vec[2]);
}

void BulletObject::ApplyTorque(const vec3 &torque) {
    btVector3 bt_torque(torque[0], torque[1], torque[2]);
    body->applyTorque(bt_torque);
}

void BulletObject::UpdateTransform() {
    btTransform bt_transform;
    GetDisplayTransform(&bt_transform);
    old_transform = transform;
    transform = SafeGetOpenGLMatrix(bt_transform);
    if (keep_history) {
        if (transform_history.empty()) {
            transform_history.resize(1);
        }
        const size_t _history_size = transform_history.size();
        for (size_t i = 0; i < _history_size - 1; ++i) {
            transform_history[_history_size - 1 - i] = transform_history[_history_size - 2 - i];
        }
        transform_history[0] = transform;
    }
}

void BulletObject::CheckForNAN() {
    btTransform bt_transform;
    GetDisplayTransform(&bt_transform);
    mat4 test_mat = SafeGetOpenGLMatrix(body->getWorldTransform());
    for (float entry : test_mat.entries) {
        if (entry != entry) {
            LOGE << "NAN found in BulletObject" << std::endl;
            break;
        }
    }
    btMatrix3x3 mat = bt_transform.getBasis().inverse();
    if (mat.getColumn(0)[0] != mat.getColumn(0)[0]) {
        LOGE << "NAN found in BulletObject" << std::endl;
    }
    const btVector3 &vel = body->getLinearVelocity();
    for (unsigned i = 0; i < 3; ++i) {
        if (vel[i] != vel[i]) {
            LOGE << "NAN found in BulletObject" << std::endl;
            break;
        }
    }
    const btVector3 &vel2 = body->getAngularVelocity();
    for (unsigned i = 0; i < 3; ++i) {
        if (vel2[i] != vel2[i]) {
            LOGE << "NAN found in BulletObject" << std::endl;
            break;
        }
    }
}

float BulletObject::GetMomentOfInertia(const vec3 &axis) {
    btVector3 bt_axis(axis[0], axis[1], axis[2]);
    // bt_axis = quatRotate(body->getWorldTransform().getRotation().inverse(),bt_axis);
    return 1.0f / ((body->getInvInertiaTensorWorld() * bt_axis * body->getAngularFactor()).dot(bt_axis));
}

bool BulletObject::IsActive() {
    int activation_state = body->getActivationState();
    return (activation_state == ACTIVE_TAG || activation_state == DISABLE_DEACTIVATION);
}

void BulletObject::Collided(const vec3 &pos, float impulse, const CollideInfo &collide_info) {
    if (owner_object) {
        owner_object->Collided(pos, impulse, collide_info, this);
    }
}

void BulletObject::SetMass(float mass) {
    btVector3 inv_inertia = body->getInvInertiaDiagLocal();
    inv_inertia[0] = 1.0f / inv_inertia[0];
    inv_inertia[1] = 1.0f / inv_inertia[1];
    inv_inertia[2] = 1.0f / inv_inertia[2];
    body->setMassProps(mass, inv_inertia);
    const vec3 &grav = Physics::Instance()->gravity;
    body->setGravity(btVector3(grav[0], grav[1], grav[2]));
}

ShapeDisposalData::ShapeDisposalData() : index_vert_array(NULL),
                                         triangle_info_map(NULL) {}

ShapeDisposalData::~ShapeDisposalData() {
    delete index_vert_array;
    delete triangle_info_map;
}
