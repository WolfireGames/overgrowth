//-----------------------------------------------------------------------------
//           Name: bulletworld.h
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

#include <Math/vec3.h>
#include <Math/mat4.h>

#include <Physics/bulletcollision.h>
#include <Physics/bulletobject.h>

#include <Asset/Asset/texture.h>

#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <opengl.h>

#include <memory>
#include <list>
#include <vector>
#include <set>

class btBroadphaseInterface;
class btCollisionDispatcher;
class btConstraintSolver;
class btDefaultCollisionConfiguration;
class btDiscreteDynamicsWorld;
class btCollisionShape;
class BulletObject;
class Model;
class ContactSlideCallback;
class ContactInfoCallback;
class SweptSlideCallback;
struct SlideCollisionInfo;
struct SimpleRayResultCallbackInfo;
class btPoint2PointConstraint;
class btTypedConstraint;
class btCollisionObject;
class btManifoldPoint;
class btShapeHull;
class btBvhTriangleMeshShape;
class btCompoundShape;
class btConvexHullShape;
class btConvexShape;
class btRigidBody;
class btGImpactMeshShape;

typedef unsigned char BWFlags;

const int DecalsOnlyFilter = (1 << 6);

struct CollideInfo {
    vec3 normal;
    bool true_impact;
};

enum BWFlagValues {
    BW_NO_FLAGS = 0,
    BW_NO_STATIC_COLLISIONS = (1 << 0),
    BW_NO_DYNAMIC_COLLISIONS = (1 << 1),
    BW_STATIC = (1 << 3),
    BW_DECALS_ONLY = (1 << 4),
    BW_ABSTRACT_ITEM = (1 << 5)
};

class BulletWorld {
   public:
    BulletWorld();
    ~BulletWorld();

    void Init();
    void Dispose();

    void Update(float timestep);
    void Draw(const TextureRef &light_cube);

    // Collision checks
    vec3 CheckCapsuleCollision(const vec3 &start, const vec3 &end, float radius);
    vec3 CheckCapsuleCollisionSlide(const vec3 &start, const vec3 &end, float radius);
    const btCollisionObject *CheckRayCollision(const vec3 &start, const vec3 &end, vec3 *point = NULL, vec3 *normal = NULL, bool static_col = true) const;
    void CheckRayCollisionInfo(const vec3 &start, const vec3 &end, SimpleRayResultCallbackInfo &cb, bool static_only = true);
    void CheckRayTriCollisionInfo(const vec3 &start, const vec3 &end, SimpleRayTriResultCallback &cb, bool static_only = true);
    vec3 CheckSphereCollisionSlide(const vec3 &start, float radius);
    void GetSphereCollisions(const vec3 &pos, float radius, btCollisionWorld::ContactResultCallback &cb);
    void GetSweptSphereCollisions(const vec3 &start, const vec3 &end, float radius, SweptSlideCallback &callback);
    void GetSweptBoxCollisions(const vec3 &start, const vec3 &end, const vec3 &dimensions, SweptSlideCallback &callback);
    void GetSweptShapeCollisions(const vec3 &start, const vec3 &end, const btConvexShape &shape, SweptSlideCallback &callback);
    void GetSweptCylinderCollisions(const vec3 &start, const vec3 &end, float radius, float height, SweptSlideCallback &callback);
    void GetShapeCollisions(const vec3 &pos, btCollisionShape &shape, btCollisionWorld::ContactResultCallback &callback);
    void GetShapeCollisions(const mat4 &_transform, btCollisionShape &shape, btCollisionWorld::ContactResultCallback &callback);
    void GetCylinderCollisions(const vec3 &pos, float radius, float height, ContactSlideCallback &callback);
    bool CheckCollision(BulletObject *obj_a, BulletObject *obj_b);
    int CheckRayCollisionObj(const vec3 &start, const vec3 &end, const BulletObject &obj, vec3 *point, vec3 *normal);
    void SphereCollisionTriList(const vec3 &pos, float radius, TriListResults &tri_list);
    void BoxCollisionTriList(const vec3 &pos, const vec3 &dimensions, TriListResults &tri_list);
    void GetBoxCollisions(const vec3 &pos, const vec3 &dimensions, btCollisionWorld::ContactResultCallback &cb);
    void GetPairCollisions(btCollisionObject &a, btCollisionObject &b, btCollisionWorld::ContactResultCallback &cb);
    void GetScaledSphereCollisions(const vec3 &pos, float radius, const vec3 &scale, btCollisionWorld::ContactResultCallback &callback);
    std::map<void *, std::set<void *> > &GetCollisions();
    void GetConvexHullCollisions(const std::string &path, const mat4 &transform, btCollisionWorld::ContactResultCallback &cb);
    // Collision response
    vec3 ApplySphereSlide(const vec3 &pos, float radius, SlideCollisionInfo &info);
    vec3 ApplyScaledSphereSlide(const vec3 &pos, float radius, const vec3 &scale, SlideCollisionInfo &info);
    // Creating objects
    BulletObject *CreateBox(const vec3 &position, const vec3 &dimensions, BWFlags flags = 0);
    BulletObject *CreateRigidBody(SharedShapePtr shape, float mass, BWFlags flags = 0, vec3 *com = NULL);
    BulletObject *CreateCapsule(vec3 start, vec3 end, float radius, float mass = 1.0f, BWFlags flags = 0, vec3 *com = NULL);
    BulletObject *CreatePlane(const vec3 &normal, float d);
    BulletObject *CreateConvexModel(vec3 pos, std::string path, vec3 *offset = NULL, BWFlags flags = 0);
    btBvhTriangleMeshShape *CreateMeshShape(const Model *the_mesh, ShapeDisposalData &data);
    BulletObject *CreateStaticHull(const std::string &path);
    BulletObject *CreateConvexObject(const std::vector<vec3> &verts, const std::vector<int> &faces, bool is_static = false);
    BulletObject *CreateSphere(const vec3 &position, float radius, BWFlags flags);
    BulletObject *CreateStaticMesh(const Model *the_mesh, int id, BWFlags flags);
    // Creating constraints
    btTypedConstraint *AddBallJoint(BulletObject *obj_a, BulletObject *obj_b, const vec3 &world_point);
    btTypedConstraint *AddFixedJoint(BulletObject *obj_a, BulletObject *obj_b, const vec3 &anchor);
    btTypedConstraint *AddNullConstraint(BulletObject *obj_a, BulletObject *obj_b);
    btTypedConstraint *AddHingeJoint(BulletObject *obj_a, BulletObject *obj_b, const vec3 &anchor, const vec3 &axis, float low_limit, float high_limit, float *initial_angle_ptr);
    btTypedConstraint *AddAngleConstraints(BulletObject *obj_a, BulletObject *obj_b, const vec3 &anchor, float limit);
    btTypedConstraint *AddAngularStop(BulletObject *obj_a, BulletObject *obj_b, const vec3 &_anchor, const vec3 &axis1, const vec3 &axis2, const vec3 &axis3, float low_stop, float high_stop);
    void CreateTempDragConstraint(BulletObject *obj, const vec3 &from, const vec3 &to);
    void CreateBoneConstraint(BulletObject *obj, const vec3 &from, const vec3 &to);
    void CreateSpikeConstraint(BulletObject *obj, const vec3 &from, const vec3 &to, const vec3 &pos);
    void ClearBoneConstraints();
    // Manipulating constraints
    void RemoveJoint(btTypedConstraint **bt_joint);
    void ApplyD6Torque(btTypedConstraint *constraint, float axis_0_torque);
    vec3 GetD6Axis(btTypedConstraint *constraint, int which);
    void CreateD6TempTwist(btTypedConstraint *constraint, float how_much);
    vec3 GetConstraintAnchor(btTypedConstraint *joint);
    void SetD6Limits(btTypedConstraint *joint, vec3 low, vec3 high);
    void RemoveTempConstraints();
    static void UpdateFixedJoint(btTypedConstraint *_constraint, const mat4 &mat_a, const mat4 &mat_b);
    void SetJointStrength(btTypedConstraint *joint, float strength);
    // Update functions
    void HandleCollisionEffects();
    void UpdateBulletObjectTransforms();
    void HandleContactEffect(const btCollisionObject *obj_a, const btCollisionObject *obj_b, const btManifoldPoint &pt);
    void UpdateAABB();
    // Linking and unlinking
    void UnlinkObject(BulletObject *bullet_object);
    void LinkObject(BulletObject *bullet_object);
    void LinkConstraint(btTypedConstraint *constraint);
    void UnlinkConstraint(btTypedConstraint *constraint);
    void UnlinkObjectPermanent(BulletObject *bullet_object);
    void RemoveObject(BulletObject **_bullet_object);
    void RemoveStaticObject(BulletObject **_bullet_object);
    // Misc
    void SetGravity(const vec3 &gravity);
    void MergeShape(BulletObject &a, const BulletObject &b);
    void UpdateSingleAABB(BulletObject *bullet_object);
    int NumObjects();
    void FinalizeStaticEntries();
    static mat4 GetCapsuleTransform(vec3 start, vec3 end);
    static btGImpactMeshShape *CreateDynamicMeshShape(std::vector<int> &indices, std::vector<float> &vertices, ShapeDisposalData &data);
    btSoftBody *AddCloth(const vec3 &pos);
    void CreateCustomHullShape(const std::string &key, const std::vector<vec3> &points);

   public:
    btSoftBodyWorldInfo soft_body_world_info_;
#ifdef ALLOW_SOFTBODY
    btSoftRigidDynamicsWorld *dynamics_world_;
#else
    btDiscreteDynamicsWorld *dynamics_world_;
#endif
    btBroadphaseInterface *broadphase_interface_;
    btCollisionDispatcher *collision_dispatcher_;
    btConstraintSolver *constraint_solver_;
    btDefaultCollisionConfiguration *collision_configuration_;

    typedef std::list<BulletObject *> BulletObjectList;
    BulletObjectList dynamic_objects_;
    BulletObjectList static_objects_;

    std::vector<std::pair<BulletObject *, btTypedConstraint *> > temp_constraints_;
    std::vector<std::pair<BulletObject *, btTypedConstraint *> > fixed_constraints_;

    struct StaticEntry {
        int model_id;
        mat4 transform;
        void *owner_object;
    };

    std::vector<StaticEntry> static_entries;
    std::vector<int> vert_indices;
    std::vector<float> vertices;
    BulletObject *merged_obj;

    std::map<void *, std::set<void *> > collision_report_;
    typedef std::map<std::string, btConvexHullShape *> HullShapeCacheMap;
    HullShapeCacheMap hull_shape_cache_;

    btConvexHullShape &GetHullShape(const std::string &path);
    void Reset();
};

GLuint CreateHullDisplayList(btShapeHull *hull);
btConvexHullShape *CreateConvexHullShape(const std::string &path, GLuint *display_list, vec3 *offset = NULL);
btConvexHullShape *CreateConvexHullShape(const std::vector<vec3> &verts);
mat4 GetBoneMat(const vec3 &start, const vec3 &end);

void GetSimplifiedHull(const std::vector<vec3> &input, std::vector<vec3> &output, std::vector<int> &faces);
