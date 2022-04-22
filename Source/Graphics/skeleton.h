//-----------------------------------------------------------------------------
//           Name: skeleton.h
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
#pragma once

#include <Math/vec3.h>
#include <Math/mat4.h>

#include <Asset/Asset/fzx_file.h>
#include <Asset/Asset/skeletonasset.h>

#include <Editors/save_state.h>
#include <Objects/softinfo.h>
#include <Physics/bulletworld.h>
#include <Graphics/bonetransform.h>
#include <Utility/flat_hash_map.hpp>

#include <map>

struct SelectedJoint;
class BulletObject;
class btTypedConstraint;
struct SphereCollision;

struct Bone {
    int points[2];
    float mass;
    vec3 center_of_mass;
};

struct PhysicsBone {
    int bone;
    BulletObject* bullet_object;
    BulletObject* col_bullet_object;
    float display_scale;
    vec3 initial_position;
    mat4 initial_rotation;
    float physics_mass;

    PhysicsBone();
};

enum JointType {
    ROTATION_JOINT,
    HINGE_JOINT,
    FIXED_JOINT,
    BALL_JOINT,
    NULL_JOINT
};

struct PhysicsJoint {
    JointType type;
    btTypedConstraint *bt_joint;
    btTypedConstraint *fixed_joint;
    bool fixed_joint_enabled;
    BulletObject* bt_bone[2];
    float stop_angle[6];
    float initial_angle;
    vec3 initial_axis;
    vec3 initial_axis2;
    vec3 anchor;

    PhysicsJoint() :bt_joint(NULL),
                    fixed_joint(NULL),
                    fixed_joint_enabled(true)
    {}
};

class Skeleton {
    BulletWorld *bullet_world;
    std::map<BulletObject*, int> phys_id;

    void AddModifiedSphere(btCompoundShape* compound_shape, const vec3 &scale, const vec3 &offset, const quaternion &rotation, const mat4 &capsule_transform, const vec3& skel_offset, float body_scale);
    void AddModifiedCapsule(btCompoundShape* compound_shape, const vec3 &scale, const vec3 &offset, const quaternion &rotation, const mat4 &capsule_transform, const vec3& skel_offset, float body_scale);
    void AddModifiedBox(btCompoundShape* compound_shape, const vec3 &scale, const vec3 &offset, const quaternion &rotation, const mat4 &capsule_transform, const vec3& skel_offset, float body_scale);

    // These pointers are owned by us
    std::vector<btConvexShape *> child_shapes;


public:    
    std::unique_ptr<BulletWorld> col_bullet_world;
    std::vector<int> parents;
    std::vector<PhysicsBone> physics_bones;
    std::vector<PhysicsJoint> physics_joints;
    std::vector<vec3> points;
    std::vector<Bone> bones;
    std::vector<btTypedConstraint*> null_constraints;
    std::vector<bool> has_verts_assigned;
    typedef ska::flat_hash_map<std::string, SimpleIKBone> SimpleIKBoneMap;
    SimpleIKBoneMap simple_ik_bones;
    std::vector<bool> fixed_obj;
    SkeletonAssetRef skeleton_asset_ref;

    void CreateHinge(const SelectedJoint &selected_joint, const vec3 &axis, float *initial_angle_ptr);
    void CreateFixed(const SelectedJoint &selected_joint);
    void CreateRotationalConstraint(const SelectedJoint &selected_joint);
    void CreateBallJoint(const SelectedJoint &selected_joint);
    void SetBulletWorld(BulletWorld *_bullet_world);
    int Read( const std::string &path, float scale, float mass_scale, const FZXAssetRef &fzx_ref);
    static void ApplyJointRange(PhysicsJoint *joint);
    void DeleteJoint(PhysicsJoint *joint);
    void CreatePhysicsSkeleton(float scale, const FZXAssetRef& fzx_ref);
    int GetAttachedBone( BulletObject* object );
    void Dispose();
    void SetGravity( bool enable );
    void ReduceROM(float how_much);
    void PhysicsDispose();
    void GetPhysicsObjectsFromSelectedJoint( std::vector<BulletObject*> &selected_bullet_bones, const SelectedJoint & selected_joint );
    void DeleteJointOnBones( std::vector<BulletObject*> &selected_bullet_bones );
    vec3 GetCenterOfMass();
    std::vector<float> GetBoneMasses();
    void UnlinkFromBulletWorld();
    void LinkToBulletWorld();
    void AddNullConstraints();
    void ShiftJoint( PhysicsJoint& joint, int obj_a, int obj_b );
    void UpdateTwistBones(bool update_transform);
    void SetGFStrength(PhysicsJoint& joint, float _strength);
    void RefreshFixedJoints(const std::vector<BoneTransform> &mats);
    void CheckForNAN();
    void AlternateHull( const std::string &model_name, const vec3 &old_center, float model_char_scale );
    void DrawCollisionBulletWorld();
    void GetSweptSphereCollisionCharacter( const vec3 & pos, const vec3 & pos2, float radius, SphereCollision & as_col );
    void UpdateCollisionWorldAABB();
    const btCollisionObject* CheckRayCollision( const vec3 & start, const vec3 & end, vec3* point, vec3* normal );
    int CheckRayCollisionBone( const vec3 & start, const vec3 & end );
    ~Skeleton();
};
