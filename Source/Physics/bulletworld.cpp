//-----------------------------------------------------------------------------
//           Name: bulletworld.cpp
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

#include <Physics/bulletworld.h>
#include <Physics/physics.h>
#include <Physics/bulletobject.h>
#include <Physics/bulletcollision.h>

#include <Graphics/camera.h>
#include <Graphics/geometry.h>
#include <Graphics/particles.h>
#include <Graphics/textures.h>
#include <Graphics/geometry.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/model.h>
#include <Graphics/models.h>

#include <Internal/dialogues.h>
#include <Internal/checksum.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>
#include <Internal/timer.h>

#include <Sound/sound.h>
#include <UserInput/input.h>
#include <Math/vec3math.h>
#include <GUI/gui.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btGeometryUtil.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>
#include <BulletCollision/Gimpact/btGImpactShape.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include <set>

extern bool g_simple_shadows;
extern bool g_level_shadows;
extern Timer game_timer;
//#define ALLOW_SOFTBODY true

inline btScalar calculateCombinedFriction(float friction0, float friction1) {
    btScalar friction = friction0 * friction1;
    const btScalar MAX_FRICTION = 10.f;
    if (friction < -MAX_FRICTION)
        friction = -MAX_FRICTION;
    if (friction > MAX_FRICTION)
        friction = MAX_FRICTION;
    return friction;
}

inline btScalar calculateCombinedRestitution(float restitution0, float restitution1) {
    return restitution0 * restitution1;
}

static bool CustomMaterialCombinerCallback(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1) {
    // Modify normal to use single-sided collision with triangle meshes
    if (colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE) {
        const btTriangleShape *tri_shape = static_cast<const btTriangleShape *>(colObj1Wrap->getCollisionShape());
        btVector3 tri_normal;
        tri_shape->calcNormal(tri_normal);
        cp.m_normalWorldOnB = colObj1Wrap->getWorldTransform().getBasis() * tri_normal;
    }
    // Uncomment to draw collision normals:
    // const btVector3& start = cp.getPositionWorldOnB();
    // const btVector3& end = start + cp.m_normalWorldOnB;
    // DebugDraw::Instance()->AddLine(vec3(start[0],start[1],start[2]), vec3(end[0],end[1],end[2]), vec4(1.0f), _delete_on_update);
    float friction0 = colObj0Wrap->getCollisionObject()->getFriction();
    float friction1 = colObj1Wrap->getCollisionObject()->getFriction();
    float restitution0 = colObj0Wrap->getCollisionObject()->getRestitution();
    float restitution1 = colObj1Wrap->getCollisionObject()->getRestitution();
    cp.m_combinedFriction = calculateCombinedFriction(friction0, friction1);
    cp.m_combinedRestitution = calculateCombinedRestitution(restitution0, restitution1);
    return true;
}

extern ContactAddedCallback gContactAddedCallback;
void BulletWorld::Init() {
    Dispose();
    gContactAddedCallback = CustomMaterialCombinerCallback;
    // collision_configuration_ = new btDefaultCollisionConfiguration(); // 4 mb ram
#ifdef ALLOW_SOFTBODY
    collision_configuration_ = new btSoftBodyRigidBodyCollisionConfiguration();
#else
    collision_configuration_ = new btDefaultCollisionConfiguration();
#endif
    collision_dispatcher_ = new btCollisionDispatcher(collision_configuration_);
    btGImpactCollisionAlgorithm::registerAlgorithm(collision_dispatcher_);
    broadphase_interface_ = new btDbvtBroadphase();
    constraint_solver_ = new btSequentialImpulseConstraintSolver;
#ifdef ALLOW_SOFTBODY
    dynamics_world_ = new btSoftRigidDynamicsWorld(collision_dispatcher_,
                                                   broadphase_interface_,
                                                   constraint_solver_,
                                                   collision_configuration_);
#else
    dynamics_world_ = new btDiscreteDynamicsWorld(collision_dispatcher_,
                                                  broadphase_interface_,
                                                  constraint_solver_,
                                                  collision_configuration_);
#endif
    dynamics_world_->setForceUpdateAllAabbs(false);

    soft_body_world_info_.m_dispatcher = collision_dispatcher_;
    soft_body_world_info_.m_broadphase = broadphase_interface_;
    soft_body_world_info_.m_sparsesdf.Initialize();
}

void BulletWorld::SetGravity(const vec3 &gravity) {
    dynamics_world_->setGravity(btVector3(gravity[0], gravity[1], gravity[2]));
    soft_body_world_info_.m_gravity.setValue(gravity[0], gravity[1], gravity[2]);
}

btSoftBody *BulletWorld::AddCloth(const vec3 &pos) {
#ifdef ALLOW_SOFTBODY
    // TRACEDEMO
    btVector3 bt_pos(pos[0], pos[1], pos[2]);
    const btScalar s = 0.8;
    btSoftBody *psb = btSoftBodyHelpers::CreatePatch(soft_body_world_info_, btVector3(-s, 0, -s) + bt_pos,
                                                     btVector3(+s, 0, -s) + bt_pos,
                                                     btVector3(-s, 0, +s) + bt_pos,
                                                     btVector3(+s, 0, +s) + bt_pos,
                                                     31, 31,
                                                     //		31,31,
                                                     0, true);

    psb->getCollisionShape()->setMargin(0.05);
    btSoftBody::Material *pm = psb->appendMaterial();
    pm->m_kLST = 0.4;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;
    psb->generateBendingConstraints(2, pm);
    psb->setTotalMass(15);
    dynamics_world_->addSoftBody(psb);
    return psb;
#else
    return NULL;
#endif
}

void BulletWorld::Reset() {
    if (dynamics_world_) {
        btDispatcher *dispatcher = dynamics_world_->getDispatcher();
        dynamics_world_->getBroadphase()->resetPool(dispatcher);
        dynamics_world_->getConstraintSolver()->reset();
    }
}

void BulletWorld::Dispose() {
    if (dynamics_world_) {
        // Delete all constraints
        int num_constraints = dynamics_world_->getNumConstraints();

        if (num_constraints > 0) {
            LOGE << "There are still " << num_constraints << " joints in the dynamics_world_, should be cleaned up from where they were created. "
                 << "It can't be done from here, because constraints have to be cleared before their objects." << std::endl;
        }
    }
    delete dynamics_world_;
    dynamics_world_ = NULL;
    delete constraint_solver_;
    constraint_solver_ = NULL;
    delete broadphase_interface_;
    broadphase_interface_ = NULL;
    delete collision_dispatcher_;
    collision_dispatcher_ = NULL;
    delete collision_configuration_;
    collision_configuration_ = NULL;
    for (auto object : dynamic_objects_) {
        object->Dispose();
        delete object;
    }
    dynamic_objects_.clear();
    for (auto object : static_objects_) {
        object->Dispose();
        delete object;
    }
    static_objects_.clear();
    for (auto &it : hull_shape_cache_) {
        delete it.second;
    }
    hull_shape_cache_.clear();
}

void BulletWorld::Update(float timestep) {
    if (dynamics_world_) {
        dynamics_world_->stepSimulation(1.0f, 1, timestep);
        HandleCollisionEffects();
        UpdateBulletObjectTransforms();
        RemoveTempConstraints();
    }
}

std::map<void *, std::set<void *> > &BulletWorld::GetCollisions() {
    PROFILER_ZONE(g_profiler_ctx, "BulletWorld::GetCollisions()");
    collision_report_.clear();
    if (false) {
        // PROFILER_ZONE(g_profiler_ctx, "performDiscreteCollisionDetection");
        // dynamics_world_->performDiscreteCollisionDetection();
        PROFILER_ENTER(g_profiler_ctx, "getDispatchInfo");
        btDispatcherInfo &dispatchInfo = dynamics_world_->getDispatchInfo();
        PROFILER_LEAVE(g_profiler_ctx);

        dynamics_world_->setForceUpdateAllAabbs(true);

        PROFILER_ENTER(g_profiler_ctx, "updateAabbs");
        dynamics_world_->updateAabbs();
        PROFILER_LEAVE(g_profiler_ctx);

        PROFILER_ENTER(g_profiler_ctx, "computeOverlappingPairs");
        dynamics_world_->computeOverlappingPairs();
        PROFILER_LEAVE(g_profiler_ctx);

        PROFILER_ENTER(g_profiler_ctx, "getDispatcher");
        btDispatcher *dispatcher = dynamics_world_->getDispatcher();
        PROFILER_LEAVE(g_profiler_ctx);
        {
            BT_PROFILE("dispatchAllCollisionPairs");
            if (dispatcher) {
                PROFILER_ENTER(g_profiler_ctx, "dispatchAllCollisionPairs");
                dispatcher->dispatchAllCollisionPairs(dynamics_world_->m_broadphasePairCache->getOverlappingPairCache(), dispatchInfo, dynamics_world_->m_dispatcher1);
                PROFILER_LEAVE(g_profiler_ctx);
            }
        }
    }

    PROFILER_ENTER(g_profiler_ctx, "stepSimulation");
    dynamics_world_->stepSimulation(1, 1, (btScalar)0.000001);
    PROFILER_LEAVE(g_profiler_ctx);
    /*
    btOverlappingPairCache* pairs = dynamics_world_->m_broadphasePairCache->getOverlappingPairCache();
    btBroadphasePairArray& pair_array = pairs->getOverlappingPairArray();
    for(int i=0, len=pair_array.size(); i<len; ++i){
            btVector3 a = (pair_array[i].m_pProxy0->m_aabbMin + pair_array[i].m_pProxy0->m_aabbMax)*0.5;
            btVector3 b = (pair_array[i].m_pProxy1->m_aabbMin + pair_array[i].m_pProxy1->m_aabbMax)*0.5;
            DebugDraw::Instance()->AddLine(*((vec3*)&a),*((vec3*)&b), vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 1.0f), _delete_on_update);
            btVector3 size = pair_array[i].m_pProxy0->m_aabbMax - pair_array[i].m_pProxy0->m_aabbMin;
            DebugDraw::Instance()->AddWireBox(*((vec3*)&a),*((vec3*)&size), vec4(1.0f,0.0f,0.0f,0.3f), _delete_on_update);
            size = pair_array[i].m_pProxy1->m_aabbMax - pair_array[i].m_pProxy1->m_aabbMin;
            DebugDraw::Instance()->AddWireBox(*((vec3*)&b),*((vec3*)&size), vec4(1.0f,0.0f,0.0f,0.3f), _delete_on_update);
    }*/

    {
        PROFILER_ZONE(g_profiler_ctx, "Prepare report");
        int numManifolds = collision_dispatcher_->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            btPersistentManifold *contactManifold =
                collision_dispatcher_->getManifoldByIndexInternal(i);
            int numContacts = contactManifold->getNumContacts();
            if (numContacts == 0) {
                continue;
            }
            const btCollisionObject *obA =
                static_cast<const btCollisionObject *>(contactManifold->getBody0());
            const btCollisionObject *obB =
                static_cast<const btCollisionObject *>(contactManifold->getBody1());
            void *ptrA = obA->getUserPointer();
            void *ptrB = obB->getUserPointer();
            if (!ptrA || !ptrB) {
                continue;
            }
            ptrA = ((BulletObject *)ptrA)->owner_object;
            ptrB = ((BulletObject *)ptrB)->owner_object;
            if (!ptrA || !ptrB) {
                continue;
            }
            if (ptrA < ptrB) {
                collision_report_[ptrA].insert(ptrB);
            } else {
                collision_report_[ptrB].insert(ptrA);
            }
            /*
            for (int j=0;j<numContacts;j++)
            {
                    btManifoldPoint& pt = contactManifold->getContactPoint(j);
                    const btVector3& ptA = pt.getPositionWorldOnA();
                    const btVector3& ptB = pt.getPositionWorldOnB();
                    btVector3 bt_col = (ptA + ptB)*0.5f;
                    vec3 collision_point(bt_col[0], bt_col[1], bt_col[2]);
                    DebugDraw::Instance()->AddWireSphere(collision_point,0.1f,vec4(1.0f,0.0f,0.0f,1.0f),_delete_on_update);
            }*/
        }
    }
    return collision_report_;
}

void BulletWorld::GetConvexHullCollisions(const std::string &path, const mat4 &transform, btCollisionWorld::ContactResultCallback &cb) {
    GetShapeCollisions(transform, GetHullShape(path), cb);
}

void BulletWorld::GetSphereCollisions(const vec3 &pos, float radius, btCollisionWorld::ContactResultCallback &cb) {
    btSphereShape shape(radius);
    GetShapeCollisions(pos, shape, cb);
}

void BulletWorld::GetBoxCollisions(const vec3 &pos, const vec3 &dimensions, btCollisionWorld::ContactResultCallback &cb) {
    btBoxShape shape(btVector3(dimensions[0] * 0.5f,
                               dimensions[1] * 0.5f,
                               dimensions[2] * 0.5f));
    GetShapeCollisions(pos, shape, cb);
}

void BulletWorld::GetScaledSphereCollisions(const vec3 &pos,
                                            float radius,
                                            const vec3 &scale,
                                            btCollisionWorld::ContactResultCallback &callback) {
    btSphereShape shape(radius);
    shape.setLocalScaling(btVector3(scale[0], scale[1], scale[2]));
    GetShapeCollisions(pos, shape, callback);
}

void BulletWorld::GetCylinderCollisions(const vec3 &pos,
                                        float radius,
                                        float height,
                                        ContactSlideCallback &callback) {
    btCylinderShape shape(btVector3(radius, height * 0.5f, radius));
    GetShapeCollisions(pos, shape, callback);
}

void BulletWorld::GetShapeCollisions(const vec3 &pos,
                                     btCollisionShape &shape,
                                     btCollisionWorld::ContactResultCallback &callback) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(pos[0], pos[1], pos[2]));

    btCollisionObject temp_object;
    temp_object.setCollisionShape(&shape);
    ValidateTransform(transform);
    temp_object.setWorldTransform(transform);

    dynamics_world_->contactTest(&temp_object, callback);
}

void BulletWorld::GetShapeCollisions(const mat4 &_transform,
                                     btCollisionShape &shape,
                                     btCollisionWorld::ContactResultCallback &callback) {
    btTransform bt_transform;
    bt_transform.setFromOpenGLMatrix((btScalar *)_transform.entries);

    btCollisionObject temp_object;
    temp_object.setCollisionShape(&shape);
    ValidateTransform(bt_transform);
    temp_object.setWorldTransform(bt_transform);

    dynamics_world_->contactTest(&temp_object, callback);
}

struct DepthWithID {
    float depth;
    int id;
};

class DepthSorter {
   public:
    bool operator()(const DepthWithID &a, const DepthWithID &b) {
        return a.depth > b.depth;
    }
};

vec3 BulletWorld::ApplySphereSlide(const vec3 &pos,
                                   float radius,
                                   SlideCollisionInfo &info) {
    std::vector<DepthWithID> dwi(info.contacts.size());
    for (int i = 0, len = info.contacts.size(); i < len; ++i) {
        const vec3 &plane_normal = info.contacts[i].normal;
        const vec3 &plane_point = info.contacts[i].point;
        const vec3 &obj_point = pos;
        float d = dot(plane_normal, plane_point);
        float d2 = dot(plane_normal, obj_point);
        float depth = d - (d2 - radius);
        dwi[i].id = i;
        dwi[i].depth = depth;
    }
    std::sort(dwi.begin(), dwi.end(), DepthSorter());
    vec3 final_pos = pos;
    for (int i = 0, len = info.contacts.size(); i < len; ++i) {
        const vec3 &plane_normal = info.contacts[dwi[i].id].normal;
        const vec3 &plane_point = info.contacts[dwi[i].id].point;
        const vec3 &obj_point = final_pos;
        float d = dot(plane_normal, plane_point);
        float d2 = dot(plane_normal, obj_point);
        float depth = d - (d2 - radius);
        if (depth > 0.00001f) {
            final_pos += plane_normal * depth;
        }
    }
    return final_pos;
}

vec3 BulletWorld::ApplyScaledSphereSlide(const vec3 &pos,
                                         float radius,
                                         const vec3 &scale,
                                         SlideCollisionInfo &info) {
    std::vector<DepthWithID> dwi(info.contacts.size());
    for (int i = 0, len = info.contacts.size(); i < len; ++i) {
        const vec3 &plane_normal = info.contacts[i].normal;
        const vec3 &plane_point = info.contacts[i].point;
        const vec3 &obj_point = pos;
        float d = dot(plane_normal, plane_point);
        float d2 = dot(plane_normal, obj_point);
        btScalar scaled_radius = radius *
                                 sqrtf(square(scale[0] * plane_normal[0]) +
                                       square(scale[1] * plane_normal[1]) +
                                       square(scale[2] * plane_normal[2]));
        btScalar depth = d - (d2 - scaled_radius);
        dwi[i].id = i;
        dwi[i].depth = depth;
    }
    vec3 final_pos = pos;
    std::sort(dwi.begin(), dwi.end(), DepthSorter());
    for (int i = 0, len = info.contacts.size(); i < len; ++i) {
        const vec3 &plane_normal = info.contacts[dwi[i].id].normal;
        const vec3 &plane_point = info.contacts[dwi[i].id].point;
        const vec3 &obj_point = final_pos;
        float d = dot(plane_normal, plane_point);
        float d2 = dot(plane_normal, obj_point);
        btScalar scaled_radius = radius *
                                 sqrtf(square(scale[0] * plane_normal[0]) +
                                       square(scale[1] * plane_normal[1]) +
                                       square(scale[2] * plane_normal[2]));
        btScalar depth = d - (d2 - scaled_radius);
        if (depth > 0.00001f) {
            final_pos += plane_normal * depth;
        }
    }
    return final_pos;
}

vec3 BulletWorld::CheckSphereCollisionSlide(const vec3 &pos,
                                            float radius) {
    ContactSlideCallback cb;

    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    GetSphereCollisions(pos, radius, cb);
    return ApplySphereSlide(pos, radius, cb.collision_info);
}

void BulletWorld::SphereCollisionTriList(const vec3 &pos,
                                         float radius,
                                         TriListResults &tri_list) {
    TriListCallback cb(tri_list);

    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    GetSphereCollisions(pos, radius, cb);
}

void BulletWorld::BoxCollisionTriList(const vec3 &pos,
                                      const vec3 &dimensions,
                                      TriListResults &tri_list) {
    TriListCallback cb(tri_list);

    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    GetBoxCollisions(pos, dimensions, cb);
}

void BulletWorld::GetSweptSphereCollisions(const vec3 &start,
                                           const vec3 &end,
                                           float radius,
                                           SweptSlideCallback &callback) {
    btSphereShape sphere_shape(radius);
    GetSweptShapeCollisions(start, end, sphere_shape, callback);
}

void BulletWorld::GetSweptCylinderCollisions(const vec3 &start,
                                             const vec3 &end,
                                             float radius,
                                             float height,
                                             SweptSlideCallback &callback) {
    btCylinderShape shape(btVector3(radius, height * 0.5f, radius));
    GetSweptShapeCollisions(start, end, shape, callback);
}

void BulletWorld::GetSweptShapeCollisions(const vec3 &start,
                                          const vec3 &end,
                                          const btConvexShape &shape,
                                          SweptSlideCallback &callback) {
    btTransform start_transform;
    start_transform.setIdentity();
    start_transform.setOrigin(btVector3(start[0], start[1], start[2]));
    btTransform end_transform;
    end_transform.setIdentity();
    end_transform.setOrigin(btVector3(end[0], end[1], end[2]));

    callback.start_pos = btVector3(start[0], start[1], start[2]);
    callback.end_pos = btVector3(end[0], end[1], end[2]);

    // TODO: If distance between start and pos is really small, we should do a non-sweep collision check here.
    // there doesn' seem to be a trivial replacement, so for now, let's try and just not do the collisions check.
    // The reason why is that bullet will throw an assertion if we do.
    if (length_squared(start - end) > std::numeric_limits<float>::epsilon()) {
        dynamics_world_->convexSweepTest(&shape,
                                         start_transform,
                                         end_transform,
                                         callback);
    } else {
        LOGW << "Tried to do an invalid sweep collision check with a zero distance between start and end" << std::endl;
    }
}

void BulletWorld::GetSweptBoxCollisions(const vec3 &start,
                                        const vec3 &end,
                                        const vec3 &dimensions,
                                        SweptSlideCallback &callback) {
    btBoxShape box_shape(btVector3(dimensions[0] * 0.5f,
                                   dimensions[1] * 0.5f,
                                   dimensions[2] * 0.5f));
    GetSweptShapeCollisions(start, end, box_shape, callback);
}

vec3 BulletWorld::CheckCapsuleCollisionSlide(const vec3 &start,
                                             const vec3 &end,
                                             float radius) {
    SweptSlideCallback callback;
    callback.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    GetSweptSphereCollisions(start, end, radius, callback);
    return ApplySphereSlide(end, radius, callback.collision_info);
}

#include "Graphics/glstate.h"
#include "Graphics/graphics.h"
#include "Graphics/textures.h"
#include "Graphics/shaders.h"
#include "Graphics/camera.h"
#include "Graphics/sky.h"

void DrawBulletObject(BulletObject &object) {
    if (!object.IsVisible()) {
        return;
    }
    DebugDraw *debug_draw = DebugDraw::Instance();

    Shaders *shaders = Shaders::Instance();
    shaders->SetUniformVec4("color_tint", vec4(object.color, 1.0f));

    const mat4 &old_transform = object.old_transform;
    const mat4 &new_transform = object.transform;
    mat4 interp = mix(old_transform, new_transform, game_timer.GetInterpWeight());
    interp.AddTranslation(interp.GetRotatedvec3(object.com_offset));

    // Add cross at center of mass
    float com_length = 0.01f;
    debug_draw->AddLine(interp * vec3(-com_length, 0, 0), interp * vec3(com_length, 0, 0), vec3(0, 0, 1), _delete_on_draw, _DD_XRAY);
    debug_draw->AddLine(interp * vec3(0, -com_length, 0), interp * vec3(0, com_length, 0), vec3(0, 0, 1), _delete_on_draw, _DD_XRAY);
    debug_draw->AddLine(interp * vec3(0, 0, -com_length), interp * vec3(0, 0, com_length), vec3(0, 0, 1), _delete_on_draw, _DD_XRAY);

    shaders->SetUniformMat4("model_mat", interp);
    shaders->SetUniformMat3("model_rotation_mat", interp.GetRotationPart());
    CHECK_GL_ERROR();
    CHECK_GL_ERROR();
}

void BulletWorld::Draw(const TextureRef &light_cube) {
    Shaders *shaders = Shaders::Instance();
    Graphics *graphics = Graphics::Instance();
    Textures *textures = Textures::Instance();
    Camera *cam = ActiveCameras::Get();

    GLState gl_state;
    gl_state.blend = false;
    gl_state.cull_face = true;
    gl_state.depth_test = true;
    gl_state.depth_write = true;

    graphics->setGLState(gl_state);

    shaders->setProgram(shaders->returnProgram("cubemapdiffuse"));
    textures->bindTexture(light_cube, 2);
    shaders->SetUniformVec3("cam_pos", cam->GetPos());
    shaders->SetUniformMat4("projection_view_mat", cam->GetProjMatrix() * cam->GetViewMatrix());
    std::vector<mat4> shadow_matrix;
    shadow_matrix.resize(4);
    for (int i = 0; i < 4; ++i) {
        shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
    }
    if (g_simple_shadows || !g_level_shadows) {
        shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
    }
    shaders->SetUniformMat4Array("shadow_matrix", shadow_matrix);

    std::list<BulletObject *>::iterator iter;
    for (iter = dynamic_objects_.begin();
         iter != dynamic_objects_.end();
         iter++) {
        BulletObject &object = *(*iter);
        DrawBulletObject(object);
    }
    for (iter = static_objects_.begin();
         iter != static_objects_.end();
         iter++) {
        BulletObject &object = *(*iter);
        DrawBulletObject(object);
    }
    CHECK_GL_ERROR();
}

btBvhTriangleMeshShape *BulletWorld::CreateMeshShape(const Model *the_mesh, ShapeDisposalData &data) {
    PROFILER_ZONE(g_profiler_ctx, "CreateMeshShape");
    const Model &mesh = (*the_mesh);

    int index_stride = 3 * sizeof(int);
    int vertex_stride = 3 * sizeof(GLfloat);

    std::vector<int> &faces = data.faces;
    faces.resize(mesh.faces.size());
    for (unsigned i = 0; i < faces.size(); i++) {
        faces[i] = mesh.faces[i];
    }

    PROFILER_ENTER(g_profiler_ctx, "Creating btTriangleIndexVertexArray");
    data.index_vert_array = new btTriangleIndexVertexArray(mesh.faces.size() / 3,
                                                           &faces[0],
                                                           index_stride,
                                                           mesh.vertices.size() / 3,
                                                           (btScalar *)&mesh.vertices[0],
                                                           vertex_stride);
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Creating btBvhTriangleMeshShape");
    btBvhTriangleMeshShape *shape = new btBvhTriangleMeshShape(data.index_vert_array, true);
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "btGenerateInternalEdgeInfo");
    data.triangle_info_map = new btTriangleInfoMap();
    btGenerateInternalEdgeInfo(shape, data.triangle_info_map);
    PROFILER_LEAVE(g_profiler_ctx);

    return shape;
}

btGImpactMeshShape *BulletWorld::CreateDynamicMeshShape(std::vector<int> &indices, std::vector<float> &vertices, ShapeDisposalData &data) {
    int index_stride = 3 * sizeof(int);
    int vertex_stride = 3 * sizeof(GLfloat);

    data.index_vert_array = new btTriangleIndexVertexArray(indices.size() / 3,
                                                           &indices.front(),
                                                           index_stride,
                                                           vertices.size() / 3,
                                                           (btScalar *)&vertices.front(),
                                                           vertex_stride);

    btGImpactMeshShape *shape = new btGImpactMeshShape(data.index_vert_array);
    shape->updateBound();
    return shape;
}

struct CachedShape {
    SharedShapePtr shape;
    ShapeDisposalData disposal_data;
};

typedef std::map<int, CachedShape> CachedMeshShapes;
static CachedMeshShapes cached_mesh_shapes;

BulletObject *BulletWorld::CreateStaticMesh(const Model *the_mesh, int id, BWFlags flags) {
    SharedShapePtr shape;
    ShapeDisposalData *sdd = NULL;

    CachedMeshShapes::iterator iter = cached_mesh_shapes.find(id);
    if (id == -1) {
        sdd = new ShapeDisposalData();
        shape.reset(CreateMeshShape(the_mesh, *sdd));
    } else if (iter != cached_mesh_shapes.end()) {
        CachedShape &cached_shape = iter->second;
        shape.reset(new btScaledBvhTriangleMeshShape((btBvhTriangleMeshShape *)cached_shape.shape.get(), btVector3(1.0f, 1.0f, 1.0f)));
    } else {
        CachedShape &cached_shape = cached_mesh_shapes[id];
        cached_shape.shape.reset(CreateMeshShape(the_mesh, cached_shape.disposal_data));
        shape.reset(new btScaledBvhTriangleMeshShape((btBvhTriangleMeshShape *)cached_shape.shape.get(), btVector3(1.0f, 1.0f, 1.0f)));
    }

    // Use this for now until hooking up btScaledBvhTriangleMeshShape
    // sdd = new ShapeDisposalData();
    // shape.reset(CreateMeshShape(the_mesh, *sdd));

    BulletObject *obj = CreateRigidBody(shape, 0.0f, flags);
    if (sdd) {
        obj->shape_disposal_data.reset(sdd);
    }
    obj->body->setCollisionFlags(obj->body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
    return obj;
}

BulletObject *BulletWorld::CreateStaticHull(const std::string &path) {
    SharedShapePtr shape(CreateConvexHullShape(path, NULL));
    BulletObject *obj = CreateRigidBody(shape, 0.0f);
    obj->body->setCollisionFlags(obj->body->getCollisionFlags() |
                                 btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
    return obj;
}

BulletWorld::BulletWorld() : dynamics_world_(NULL),
                             broadphase_interface_(NULL),
                             collision_dispatcher_(NULL),
                             constraint_solver_(NULL),
                             collision_configuration_(NULL) {
}

BulletWorld::~BulletWorld() {
    Dispose();
}

BulletObject *BulletWorld::CreateRigidBody(SharedShapePtr shape,
                                           float mass,
                                           BWFlags flags,
                                           vec3 *com) {
    btTransform transform;
    transform.setIdentity();
    btVector3 local_inertia(0, 0, 0);
    if (mass != 0.0f) {
        shape->calculateLocalInertia(mass, local_inertia);
    }
    btDefaultMotionState *motion_state = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motion_state, shape.get(), local_inertia);
    btRigidBody *body = new btRigidBody(rbInfo);
    body->setFriction(1.0f);
    body->setRestitution(0.4f);
    // body->setSleepingThresholds(0.8f,1.0f);
    body->setSleepingThresholds(2.4f, 3.0f);
    std::list<BulletObject *> &object_list = mass != 0.0f ? dynamic_objects_ : static_objects_;
    object_list.resize(object_list.size() + 1);
    object_list.back() = new BulletObject();
    BulletObject *object = object_list.back();
    object->body = body;
    object->SetShape(shape);
    object->visible = true;
    short &collision_group = object->collision_group;
    short &collision_flags = object->collision_flags;
    if (mass != 0.0f) {
        collision_flags = btBroadphaseProxy::AllFilter;
        collision_group = btBroadphaseProxy::DefaultFilter;
    } else {
        collision_flags = btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter;
        collision_group = btBroadphaseProxy::StaticFilter;
    }
    if ((flags & BW_NO_STATIC_COLLISIONS) != 0) {
        collision_flags = collision_flags ^ btBroadphaseProxy::StaticFilter;
    }
    if ((flags & BW_NO_DYNAMIC_COLLISIONS) != 0) {
        collision_flags = collision_flags ^ btBroadphaseProxy::DefaultFilter;
    }
    if ((flags & BW_DECALS_ONLY) != 0) {
        collision_flags = DecalsOnlyFilter;
    }
    if ((flags & BW_ABSTRACT_ITEM) != 0) {
        collision_flags = btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter;
        collision_group = btBroadphaseProxy::StaticFilter;
    }
    dynamics_world_->addRigidBody(body, object->collision_group, object->collision_flags);
    object->linked = true;
    body->setUserPointer(object);
    return object;
}

mat4 GetBoneMat(const vec3 &start, const vec3 &end) {
    /*
        vec3 dir = normalize(end-start);
        vec3 right;
        vec3 up;
        PlaneSpace(dir, right, up);
        mat4 transform;
        transform.SetColumn(0,right);
        transform.SetColumn(1,up);
        transform.SetColumn(2,dir);
        transform.SetTranslationPart((start + end) * 0.5f);
        return transform;
        */

    vec3 dir = normalize(end - start);
    vec3 right;
    vec3 up;
    up = vec3(0, 0, 1);
    if (fabs(dot(up, dir)) > 0.95f) {
        up = vec3(1, 0, 0);
        right = normalize(cross(up, dir));
        up = normalize(cross(dir, right));
    } else {
        right = normalize(cross(up, dir));
        up = normalize(cross(dir, right));
    }
    // PlaneSpace(dir, right, up);
    mat4 transform;
    transform.SetColumn(0, right);
    transform.SetColumn(1, up);
    transform.SetColumn(2, dir);
    transform.SetTranslationPart((start + end) * 0.5f);
    return transform;
}

mat4 BulletWorld::GetCapsuleTransform(vec3 start, vec3 end) {
    vec3 dir = normalize(end - start);
    vec3 right;
    vec3 up;
    up = vec3(0, 0, 1);
    if (fabs(dot(up, dir)) > 0.95f) {
        up = vec3(1, 0, 0);
        right = normalize(cross(up, dir));
        up = normalize(cross(dir, right));
    } else {
        right = normalize(cross(up, dir));
        up = normalize(cross(dir, right));
    }
    // PlaneSpace(dir, right, up);
    mat4 transform;
    transform.SetColumn(0, right);
    transform.SetColumn(1, up);
    transform.SetColumn(2, dir);
    transform.SetColumn(3, (start + end) * 0.5f);
    return transform;
}

BulletObject *BulletWorld::CreateCapsule(vec3 start,
                                         vec3 end,
                                         float radius,
                                         float mass,
                                         BWFlags flags,
                                         vec3 *com) {
    vec3 average = (start + end) * 0.5f;
    start -= average;
    end -= average;

    float height = distance(start, end);

    btVector3 positions[2];
    positions[0] = btVector3(0.0f, 0.0f, height * 0.5f);
    positions[1] = btVector3(0.0f, 0.0f, -height * 0.5f);
    // positions[0].setValue(start[0], start[1], start[2]);
    // positions[1].setValue(end[0], end[1], end[2]);

    btScalar radii[2];
    radii[0] = radius;
    radii[1] = radius;

    SharedShapePtr shape(new btMultiSphereShape(positions, radii, 2));

    vec3 v_positions[2];
    v_positions[0] = vec3(0.0f, 0.0f, height * 0.5f);
    v_positions[1] = vec3(0.0f, 0.0f, -height * 0.5f);
    // float v_radii[2];
    // v_radii[0] = radius;
    // v_radii[1] = radius;

    vec3 dir = normalize(end - start);
    vec3 right;
    vec3 up;
    up = vec3(0, 0, 1);
    if (fabs(dot(up, dir)) > 0.95f) {
        up = vec3(1, 0, 0);
        right = normalize(cross(up, dir));
        up = normalize(cross(dir, right));
    } else {
        right = normalize(cross(up, dir));
        up = normalize(cross(dir, right));
    }
    // PlaneSpace(dir, right, up);
    mat4 transform;
    transform.SetColumn(0, right);
    transform.SetColumn(1, up);
    transform.SetColumn(2, dir);

    vec3 transformed_offset;

    BulletObject *object =
        CreateRigidBody(shape, mass, flags, &transformed_offset);

    object->SetTransform(average, transform, vec4(1.0f));

    return object;
}

BulletObject *BulletWorld::CreateBox(const vec3 &position,
                                     const vec3 &dimensions,
                                     BWFlags flags) {
    SharedShapePtr shape(new btBoxShape(btVector3(dimensions[0] * 0.5f,
                                                  dimensions[1] * 0.5f,
                                                  dimensions[2] * 0.5f)));
    float mass = 1.0f;
    if (flags & BW_STATIC) {
        mass = 0.0f;
    }

    BulletObject *object = CreateRigidBody(shape, mass, flags);
    object->SetPosition(position);

    return object;
}

BulletObject *BulletWorld::CreateSphere(const vec3 &position,
                                        float radius,
                                        BWFlags flags) {
    SharedShapePtr shape(new btSphereShape(radius));
    float mass = 1.0f;
    if (flags & BW_STATIC) {
        mass = 0.0f;
    }

    BulletObject *object = CreateRigidBody(shape, mass, flags);
    object->SetPosition(position);

    return object;
}

void BulletWorld::RemoveObject(BulletObject **bullet_object) {
    if (!*bullet_object) {
        return;
    }
    UnlinkObject(*bullet_object);
    (*bullet_object)->Dispose();
    delete *bullet_object;
    *bullet_object = NULL;
}

void BulletWorld::UnlinkObject(BulletObject *bullet_object) {
    std::vector<std::pair<BulletObject *, btTypedConstraint *> >::iterator constraint_it;
    for (constraint_it = temp_constraints_.begin(); constraint_it != temp_constraints_.end(); constraint_it++) {
        if (constraint_it->first == bullet_object) {
            if (constraint_it->second) {
                dynamics_world_->removeConstraint(constraint_it->second);
                delete constraint_it->second;
                constraint_it->second = NULL;
            }
        }
    }

    for (constraint_it = fixed_constraints_.begin(); constraint_it != fixed_constraints_.end(); constraint_it++) {
        if (constraint_it->first == bullet_object) {
            if (constraint_it->second) {
                dynamics_world_->removeConstraint(constraint_it->second);
                delete constraint_it->second;
                constraint_it->second = NULL;
            }
        }
    }

    std::list<BulletObject *>::iterator iter;
    iter = std::find(dynamic_objects_.begin(), dynamic_objects_.end(), bullet_object);
    if (iter != dynamic_objects_.end()) {
        dynamic_objects_.erase(iter);
    }
    iter = std::find(static_objects_.begin(), static_objects_.end(), bullet_object);
    if (iter != static_objects_.end()) {
        static_objects_.erase(iter);
    }
    dynamics_world_->removeRigidBody(bullet_object->body);
}

void BulletWorld::UnlinkObjectPermanent(BulletObject *bullet_object) {
    bullet_object->linked = false;
    UnlinkObject(bullet_object);
}

void BulletWorld::LinkObject(BulletObject *bullet_object) {
    dynamic_objects_.push_back(bullet_object);
    dynamics_world_->addRigidBody(bullet_object->body,
                                  bullet_object->collision_group,
                                  bullet_object->collision_flags);
    bullet_object->linked = true;
}

void BulletWorld::UnlinkConstraint(btTypedConstraint *constraint) {
    dynamics_world_->removeConstraint(constraint);
}

void BulletWorld::LinkConstraint(btTypedConstraint *constraint) {
    dynamics_world_->addConstraint(constraint, true);
}

void BulletWorld::RemoveStaticObject(BulletObject **_bullet_object) {
    BulletObject *bullet_object = *_bullet_object;

    std::vector<std::pair<BulletObject *, btTypedConstraint *> >::iterator constraint_it;
    for (constraint_it = temp_constraints_.begin(); constraint_it != temp_constraints_.end(); constraint_it++) {
        if (constraint_it->first == bullet_object) {
            if (constraint_it->second) {
                dynamics_world_->removeConstraint(constraint_it->second);
                delete constraint_it->second;
                constraint_it->second = NULL;
            }
        }
    }

    for (constraint_it = fixed_constraints_.begin(); constraint_it != fixed_constraints_.end(); constraint_it++) {
        if (constraint_it->first == bullet_object) {
            if (constraint_it->second) {
                dynamics_world_->removeConstraint(constraint_it->second);
                delete constraint_it->second;
                constraint_it->second = NULL;
            }
        }
    }

    std::list<BulletObject *>::iterator iter;
    iter = std::find(static_objects_.begin(), static_objects_.end(), bullet_object);
    LOG_ASSERT(iter != static_objects_.end());
    BulletObject *object = (*iter);
    static_objects_.erase(iter);

    dynamics_world_->removeRigidBody(object->body);
    object->Dispose();
    delete object;
    *_bullet_object = NULL;
}

const btCollisionObject *BulletWorld::CheckRayCollision(const vec3 &start,
                                                        const vec3 &end,
                                                        vec3 *point,
                                                        vec3 *normal,
                                                        bool static_col) const {
    btVector3 btstart(start[0], start[1], start[2]);
    btVector3 btend(end[0], end[1], end[2]);
    SimpleRayResultCallback callback;

    if (static_col) {
        callback.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    }

    if (length_squared(start - end) > std::numeric_limits<float>::epsilon()) {
        dynamics_world_->rayTest(btstart, btend, callback);
        if (point) {
            *point = end * callback.m_closestHitFraction +
                     start * (1.0f - callback.m_closestHitFraction);
        }
        if (normal) {
            *normal = callback.m_hit_normal;
        }
    } else {
        LOGW_ONCE("Tried to do a ray-cast with a zero-length ray, skipping test and returning null");
    }

    return callback.m_collisionObject;
}

int BulletWorld::CheckRayCollisionObj(const vec3 &start,
                                      const vec3 &end,
                                      const BulletObject &obj,
                                      vec3 *point,
                                      vec3 *normal) {
    btVector3 btstart(start[0], start[1], start[2]);
    btVector3 btend(end[0], end[1], end[2]);
    SimpleRayTriResultCallback callback;
    callback.m_flags = btTriangleRaycastCallback::kF_FilterBackfaces;

    btTransform ray_from, ray_to;
    ray_from.setOrigin(btstart);
    ray_to.setOrigin(btend);

    dynamics_world_->rayTestSingle(ray_from,
                                   ray_to,
                                   obj.body,
                                   obj.shape.get(),
                                   obj.body->getWorldTransform(),
                                   callback);
    if (point) {
        *point = end * callback.m_closestHitFraction +
                 start * (1.0f - callback.m_closestHitFraction);
    }
    if (normal) {
        *normal = callback.m_hit_normal;
    }
    if (callback.hasHit()) {
        return callback.m_tri;
    } else {
        return -1;
    }
}

btTypedConstraint *BulletWorld::AddBallJoint(BulletObject *obj_a,
                                             BulletObject *obj_b,
                                             const vec3 &world_point) {
    btVector3 bt_world_point(world_point[0], world_point[1], world_point[2]);
    btTransform inverse_a = obj_a->body->getWorldTransform().inverse();
    btTransform inverse_b = obj_b->body->getWorldTransform().inverse();
    btVector3 a_local_point = inverse_a * bt_world_point;
    btVector3 b_local_point = inverse_b * bt_world_point;

    btPoint2PointConstraint *constraint =
        new btPoint2PointConstraint(*(obj_a->body),
                                    *(obj_b->body),
                                    a_local_point,
                                    b_local_point);

    dynamics_world_->addConstraint(constraint, true);

    return constraint;
}

void BulletWorld::UpdateFixedJoint(btTypedConstraint *_constraint, const mat4 &mat_a, const mat4 &mat_b) {
    btGeneric6DofConstraint *constraint = (btGeneric6DofConstraint *)_constraint;

    btTransform a, b;
    a.setFromOpenGLMatrix((btScalar *)mat_a.entries);
    b.setFromOpenGLMatrix((btScalar *)mat_b.entries);

    btVector3 anchor = (a.getOrigin() + b.getOrigin()) * 0.5f;

    btTransform frameInW;
    frameInW.setIdentity();
    frameInW.setOrigin(anchor);

    btTransform frameInA = a.inverse() * frameInW;
    btTransform frameInB = b.inverse() * frameInW;
    constraint->getFrameOffsetA().setBasis(frameInA.getBasis());
    constraint->getFrameOffsetB().setBasis(frameInB.getBasis());
}

btTypedConstraint *BulletWorld::AddFixedJoint(BulletObject *obj_a, BulletObject *obj_b, const vec3 &_anchor) {
    btVector3 parentAxis(1.f, 0.f, 0.f);
    btVector3 childAxis(0.f, 0.f, 1.f);
    btVector3 anchor = btVector3(_anchor[0], _anchor[1], _anchor[2]);

    btVector3 zAxis = parentAxis.normalize();
    btVector3 yAxis = childAxis.normalize();
    btVector3 xAxis = yAxis.cross(zAxis);  // we want right coordinate system
    btTransform frameInW;
    frameInW.setIdentity();
    frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
                                 xAxis[1], yAxis[1], zAxis[1],
                                 xAxis[2], yAxis[2], zAxis[2]);
    frameInW.setOrigin(anchor);

    btRigidBody *pBodyA = obj_a->body;
    btRigidBody *pBodyB = obj_b->body;
    btTransform frameInA = pBodyA->getCenterOfMassTransform().inverse() * frameInW;
    btTransform frameInB = pBodyB->getCenterOfMassTransform().inverse() * frameInW;

    btGeneric6DofConstraint *constraint = new btGeneric6DofConstraint(*pBodyA, *pBodyB, frameInA, frameInB, true);

    constraint->setLinearLowerLimit(btVector3(0., 0., 0.));
    constraint->setLinearUpperLimit(btVector3(0., 0., 0.));
    constraint->setAngularLowerLimit(btVector3(0.f, 0.0f, 0.0f));
    constraint->setAngularUpperLimit(btVector3(0.f, 0.0f, 0.0f));
    /*
     for(int i=3; i<6; ++i){
         constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.0f, i);
         constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f, i);
         constraint->setParam(BT_CONSTRAINT_CFM, 1.0f, i);
     }
  */
    dynamics_world_->addConstraint(constraint, true);

    return constraint;
}

btTypedConstraint *BulletWorld::AddNullConstraint(BulletObject *obj_a, BulletObject *obj_b) {
    btVector3 bt_world_point = (obj_a->body->getWorldTransform().getOrigin() +
                                obj_b->body->getWorldTransform().getOrigin()) *
                               0.5f;
    btTransform inverse_a = obj_a->body->getWorldTransform().inverse();
    btTransform inverse_b = obj_b->body->getWorldTransform().inverse();
    btVector3 a_local_point = inverse_a * bt_world_point;
    btVector3 b_local_point = inverse_b * bt_world_point;

    btPoint2PointConstraint *constraint =
        new btPoint2PointConstraint(*(obj_a->body),
                                    *(obj_b->body),
                                    a_local_point,
                                    b_local_point);

    constraint->m_setting.m_impulseClamp = 0.000001f;

    dynamics_world_->addConstraint(constraint, true);

    return constraint;
}

struct BinaryContactCallback : public btCollisionWorld::ContactResultCallback {
    bool hit_something;

    BinaryContactCallback() : ContactResultCallback(),
                              hit_something(false) {}

    btScalar addSingleResult(btManifoldPoint &cp,
                             const btCollisionObjectWrapper *colObjWrap0,
                             int partId0,
                             int index0,
                             const btCollisionObjectWrapper *colObjWrap1,
                             int partId1,
                             int index1) override {
        if (cp.getDistance() < 0.0f) {
            hit_something = true;
            return 0.0f;
        } else {
            return 0.0f;
        }
    }

    bool HitSomething() {
        return hit_something;
    }
};

bool BulletWorld::CheckCollision(BulletObject *obj_a, BulletObject *obj_b) {
    BinaryContactCallback callback;
    dynamics_world_->contactPairTest(obj_a->body, obj_b->body, callback);
    return callback.HitSomething();
}

void BulletWorld::RemoveJoint(btTypedConstraint **bt_joint) {
    dynamics_world_->removeConstraint(*bt_joint);
    delete *bt_joint;
    *bt_joint = NULL;
}

btTypedConstraint *BulletWorld::AddHingeJoint(BulletObject *obj_a, BulletObject *obj_b, const vec3 &anchor, const vec3 &axis, float low_limit, float high_limit, float *initial_angle_ptr) {
    btVector3 bt_world_point(anchor[0], anchor[1], anchor[2]);
    btVector3 bt_world_axis(axis[0], axis[1], axis[2]);
    btTransform inverse_a = obj_a->body->getWorldTransform().inverse();
    btTransform inverse_b = obj_b->body->getWorldTransform().inverse();
    btVector3 a_local_point = inverse_a * bt_world_point;
    btVector3 b_local_point = inverse_b * bt_world_point;
    btVector3 a_local_axis = obj_a->body->getWorldTransform().inverse().getBasis() * bt_world_axis;
    btVector3 b_local_axis = obj_b->body->getWorldTransform().inverse().getBasis() * bt_world_axis;

    btHingeConstraint *constraint =
        new btHingeConstraint(*(obj_a->body),
                              *(obj_b->body),
                              a_local_point,
                              b_local_point,
                              a_local_axis,
                              b_local_axis);

    float initial_angle = constraint->getHingeAngle(obj_a->body->getWorldTransform(), obj_b->body->getWorldTransform());
    constraint->setLimit(low_limit + initial_angle, high_limit + initial_angle);
    if (initial_angle_ptr) {
        *initial_angle_ptr = initial_angle;
    }

    dynamics_world_->addConstraint(constraint, true);

    // btHingeConstraint* hinge = (btHingeConstraint*)constraint;

    // float bt_rot = hinge->getHingeAngle(hinge->getRigidBodyA().getWorldTransform(),
    //                                     hinge->getRigidBodyB().getWorldTransform());
    // float bt_min = hinge->getLowerLimit();
    // float bt_max = hinge->getUpperLimit();
    // printf("Hinge: %f < %f < %f\n", bt_min, bt_rot, bt_max);

    return constraint;
}

btTypedConstraint *BulletWorld::AddAngularStop(BulletObject *obj_a,
                                               BulletObject *obj_b,
                                               const vec3 &_anchor,
                                               const vec3 &axis1,
                                               const vec3 &axis2,
                                               const vec3 &axis3,
                                               float low_stop,
                                               float high_stop) {
    btVector3 parentAxis(axis1[0], axis1[1], axis1[2]);
    btVector3 childAxis(axis2[0], axis2[1], axis2[2]);
    btVector3 anchor(_anchor[0], _anchor[1], _anchor[2]);

    // build frame basis
    // 6DOF constraint uses Euler angles and to define limits
    // it is assumed that rotational order is :
    // Z - first, allowed limits are (-PI,PI);
    // new position of Y - second (allowed limits are (-PI/2 + epsilon, PI/2 - epsilon), where epsilon is a small positive number
    // used to prevent constraint from instability on poles;
    // new position of X, allowed limits are (-PI,PI);
    // So to simulate ODE Universal joint we should use parent axis as Z, child axis as Y and limit all other DOFs
    // Build the frame in world coordinate system first
    btVector3 xAxis = parentAxis.normalize();
    btVector3 yAxis = childAxis.normalize();
    btVector3 zAxis = xAxis.cross(yAxis);  // we want right coordinate system
    btTransform frameInW;
    frameInW.setIdentity();
    frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
                                 xAxis[1], yAxis[1], zAxis[1],
                                 xAxis[2], yAxis[2], zAxis[2]);
    frameInW.setOrigin(anchor);

    btRigidBody *pBodyA = obj_a->body;
    btRigidBody *pBodyB = obj_b->body;
    btTransform frameInA = pBodyA->getCenterOfMassTransform().inverse() * frameInW;
    btTransform frameInB = pBodyB->getCenterOfMassTransform().inverse() * frameInW;

    btGeneric6DofConstraint *constraint = new btGeneric6DofConstraint(*pBodyA, *pBodyB, frameInA, frameInB, true);

    constraint->setLinearLowerLimit(btVector3(0., 0., 0.));
    constraint->setLinearUpperLimit(btVector3(0., 0., 0.));
    constraint->setAngularLowerLimit(btVector3(low_stop, low_stop, low_stop));
    constraint->setAngularUpperLimit(btVector3(high_stop, high_stop, high_stop));
    dynamics_world_->addConstraint(constraint, true);

    return constraint;
}

btTypedConstraint *BulletWorld::AddAngleConstraints(BulletObject *obj_a,
                                                    BulletObject *obj_b,
                                                    const vec3 &anchor,
                                                    float limit) {
    btTransform display_transform;
    obj_a->GetDisplayTransform(&display_transform);
    btVector3 pos_a = display_transform.getOrigin();
    btVector3 bt_vec = pos_a - btVector3(anchor[0], anchor[1], anchor[2]);

    vec3 vec = normalize(vec3(bt_vec[0], bt_vec[1], bt_vec[2]));
    vec3 right = normalize(cross(vec, vec3(0.0001f, 1.0001f, 0.000003f)));
    vec3 up = normalize(cross(vec, right));
    right = normalize(cross(vec, up));

    return AddAngularStop(obj_a, obj_b,
                          anchor,
                          vec, right, up,
                          -limit, limit);
}

void BulletWorld::ApplyD6Torque(btTypedConstraint *constraint,
                                float axis_0_torque) {
    btGeneric6DofConstraint *genericdof = (btGeneric6DofConstraint *)constraint;
    btVector3 world_axis = genericdof->getAxis(0);
    btRigidBody *obj_a = &genericdof->getRigidBodyA();
    btRigidBody *obj_b = &genericdof->getRigidBodyB();
    btMatrix3x3 inverse_a = obj_a->getWorldTransform().getBasis().inverse();
    btMatrix3x3 inverse_b = obj_b->getWorldTransform().getBasis().inverse();
    obj_a->applyTorque(inverse_a * (axis_0_torque * world_axis));
    obj_b->applyTorque(inverse_b * (-axis_0_torque * world_axis));
    obj_a->activate();
    obj_b->activate();
}

BulletObject *BulletWorld::CreatePlane(const vec3 &normal, float d) {
    btVector3 bt_normal(normal[0],
                        normal[1],
                        normal[2]);
    SharedShapePtr shape(new btStaticPlaneShape(bt_normal, d));
    return CreateRigidBody(shape, 0.0f);
}

vec3 BulletWorld::GetD6Axis(btTypedConstraint *constraint, int which) {
    btGeneric6DofConstraint *genericdof = (btGeneric6DofConstraint *)constraint;
    btVector3 bt_axis = genericdof->getAxis(which);
    return vec3(bt_axis[0], bt_axis[1], bt_axis[2]);
}

void BulletWorld::CreateD6TempTwist(btTypedConstraint *constraint, float how_much) {
    btGeneric6DofConstraint *genericdof = (btGeneric6DofConstraint *)constraint;
    btRigidBody *pBodyA = &genericdof->getRigidBodyA();
    btRigidBody *pBodyB = &genericdof->getRigidBodyB();
    btTransform frameInA = genericdof->getFrameOffsetA();
    btTransform frameInB = genericdof->getFrameOffsetB();

    btGeneric6DofConstraint *temp_constraint =
        new btGeneric6DofConstraint(*pBodyA,
                                    *pBodyB,
                                    frameInA,
                                    frameInB,
                                    true);

    temp_constraint->setLinearLowerLimit(btVector3(0., 0., 0.));
    temp_constraint->setLinearUpperLimit(btVector3(0., 0., 0.));

    btRotationalLimitMotor *limits[3];
    for (int i = 0; i < 3; i++) {
        limits[i] = genericdof->getRotationalLimitMotor(i);
    }

    temp_constraint->setAngularLowerLimit(btVector3(genericdof->getAngle(0) + how_much,
                                                    limits[1]->m_loLimit,
                                                    limits[2]->m_loLimit));
    temp_constraint->setAngularUpperLimit(btVector3(genericdof->getAngle(0) + how_much,
                                                    limits[1]->m_hiLimit,
                                                    limits[2]->m_hiLimit));

    pBodyA->activate();
    pBodyB->activate();

    dynamics_world_->addConstraint(temp_constraint, false);
    temp_constraints_.push_back(std::pair<BulletObject *, btTypedConstraint *>(NULL, temp_constraint));
}

vec3 BulletWorld::GetConstraintAnchor(btTypedConstraint *joint) {
    if (joint->getConstraintType() == HINGE_CONSTRAINT_TYPE) {
        btHingeConstraint *hinge = (btHingeConstraint *)joint;
        btRigidBody *obj_a = &hinge->getRigidBodyA();
        btTransform a_world = obj_a->getWorldTransform();
        btTransform a_frame = hinge->getAFrame();
        btVector3 bt_anchor = (a_world * a_frame).getOrigin();
        return vec3(bt_anchor[0], bt_anchor[1], bt_anchor[2]);
    }
    if (joint->getConstraintType() == D6_CONSTRAINT_TYPE) {
        btGeneric6DofConstraint *generic_dof = (btGeneric6DofConstraint *)joint;
        btRigidBody *obj_a = &generic_dof->getRigidBodyA();
        btTransform a_world = obj_a->getWorldTransform();
        btTransform a_frame = generic_dof->getFrameOffsetA();
        btVector3 bt_anchor = (a_world * a_frame).getOrigin();
        return vec3(bt_anchor[0], bt_anchor[1], bt_anchor[2]);
    }
    LOG_ASSERT(NULL);  // Should never get here
    return vec3(0.0f);
}

void BulletWorld::SetD6Limits(btTypedConstraint *joint, vec3 low, vec3 high) {
    LOG_ASSERT(joint->getConstraintType() == D6_CONSTRAINT_TYPE);
    btGeneric6DofConstraint *generic_dof = (btGeneric6DofConstraint *)joint;
    generic_dof->setAngularLowerLimit(btVector3(low[0], low[1], low[2]));
    generic_dof->setAngularUpperLimit(btVector3(high[0], high[1], high[2]));
    generic_dof->getRigidBodyA().activate();
    generic_dof->getRigidBodyB().activate();
}

void BulletWorld::HandleContactEffect(const btCollisionObject *obj_a,
                                      const btCollisionObject *obj_b,
                                      const btManifoldPoint &pt) {
    bool true_impact = false;
    if (pt.getDistance() < 0.f &&
        pt.getLifeTime() < 3 &&
        pt.getAppliedImpulse() > 0.1f) {
        true_impact = true;
    }
    const btVector3 &ptA = pt.getPositionWorldOnA();
    const btVector3 &ptB = pt.getPositionWorldOnB();

    btVector3 bt_col = (ptA + ptB) * 0.5f;
    vec3 collision_point(bt_col[0], bt_col[1], bt_col[2]);

    CollideInfo collide_info;
    collide_info.normal = vec3(pt.m_normalWorldOnB[0],
                               pt.m_normalWorldOnB[1],
                               pt.m_normalWorldOnB[2]);
    collide_info.true_impact = true_impact;

    if (obj_a->getUserPointer()) {
        ((BulletObject *)(obj_a->getUserPointer()))->Collided(collision_point, pt.getAppliedImpulse(), collide_info);
    }
    if (obj_b->getUserPointer()) {
        ((BulletObject *)(obj_b->getUserPointer()))->Collided(collision_point, pt.getAppliedImpulse(), collide_info);
    }
}

void BulletWorld::HandleCollisionEffects() {
    int numManifolds = collision_dispatcher_->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold *contactManifold =
            collision_dispatcher_->getManifoldByIndexInternal(i);
        const btCollisionObject *obA =
            static_cast<const btCollisionObject *>(contactManifold->getBody0());
        const btCollisionObject *obB =
            static_cast<const btCollisionObject *>(contactManifold->getBody1());

        bool active_collision = false;
        if (obA->getUserPointer()) {
            BulletObject *obj_a = (BulletObject *)obA->getUserPointer();
            if (obj_a->IsActive()) {
                active_collision = true;
            }
        }
        if (obB->getUserPointer()) {
            BulletObject *obj_b = (BulletObject *)obB->getUserPointer();
            if (obj_b->IsActive()) {
                active_collision = true;
            }
        }
        if (active_collision) {
            int numContacts = contactManifold->getNumContacts();
            for (int j = 0; j < numContacts; j++) {
                btManifoldPoint &pt = contactManifold->getContactPoint(j);
                HandleContactEffect(obA, obB, pt);
            }
        }
    }
}

void BulletWorld::UpdateBulletObjectTransforms() {
    std::list<BulletObject *>::iterator iter = dynamic_objects_.begin();
    for (; iter != dynamic_objects_.end(); ++iter) {
        BulletObject &object = **iter;
        object.UpdateTransform();
    }
}

void BulletWorld::RemoveTempConstraints() {
    for (auto &temp_constraint : temp_constraints_) {
        if (temp_constraint.second) {
            dynamics_world_->removeConstraint(temp_constraint.second);
            delete temp_constraint.second;
        }
    }
    temp_constraints_.clear();
}

void CenterAtCOMTri(Model &mesh) {
    std::vector<int> vertex_faces(mesh.vertices.size() / 3);
    for (int i = 0, len = mesh.faces.size(); i < len; i += 3) {
        ++vertex_faces[mesh.faces[i + 0]];
        ++vertex_faces[mesh.faces[i + 1]];
        ++vertex_faces[mesh.faces[i + 2]];
    }
    vec3 avg_pos;
    int num_avg = 0;
    for (int vertex_face : vertex_faces) {
        if (vertex_face == 1) {
            avg_pos += vec3(mesh.vertices[vertex_face * 3 + 0],
                            mesh.vertices[vertex_face * 3 + 1],
                            mesh.vertices[vertex_face * 3 + 2]);
            ++num_avg;
        }
    }
    if (num_avg != 3) {
        return;
    }
    avg_pos /= (float)num_avg;
    for (int i = 0, len = mesh.vertices.size() / 3; i < len; ++i) {
        mesh.vertices[i * 3 + 0] -= avg_pos[0];
        mesh.vertices[i * 3 + 1] -= avg_pos[1];
        mesh.vertices[i * 3 + 2] -= avg_pos[2];
    }
    int mesh_vert_num = mesh.vertices.size() / 3;
    for (int i = 0; i < mesh_vert_num; ++i) {
        while (vertex_faces[i] == 1 && i < mesh_vert_num) {
            vertex_faces[i] = vertex_faces[mesh_vert_num - 1];
            mesh.vertices[i * 3 + 0] = mesh.vertices[(mesh_vert_num - 1) * 3 + 0];
            mesh.vertices[i * 3 + 1] = mesh.vertices[(mesh_vert_num - 1) * 3 + 1];
            mesh.vertices[i * 3 + 2] = mesh.vertices[(mesh_vert_num - 1) * 3 + 2];
            --mesh_vert_num;
        }
    }
    mesh.vertices.resize(mesh_vert_num * 3);
    mesh.old_center += avg_pos;
}

btConvexHullShape *CreateConvexHullShape(const std::string &path, GLuint *display_list, vec3 *offset) {
    std::string hull_path = path.substr(0, path.size() - 4) + "hull.obj";
    char abs_path[kPathSize];
    if (FindFilePath(hull_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) != -1) {
        Model mesh;
        mesh.LoadObj(hull_path.c_str(), _MDL_SIMPLE | _MDL_CENTER);
        mesh.old_center = vec3(0.0f);
        CenterAtCOMTri(mesh);
        btConvexHullShape *convex_hull_shape = new btConvexHullShape();
        for (int i = 0, len = mesh.vertices.size() / 3; i < len; ++i) {
            convex_hull_shape->addPoint(btVector3(mesh.vertices[i * 3 + 0],
                                                  mesh.vertices[i * 3 + 1],
                                                  mesh.vertices[i * 3 + 2]));
        }
        convex_hull_shape->setMargin(0.00f);
        if (offset) {
            (*offset) = mesh.old_center;
        }
        return convex_hull_shape;
    } else {
        DisplayError("Warning", ("No convex hull found for " + path).c_str());
    }

    Model mesh;
    mesh.LoadObj(path.c_str());

    int index_stride = 3 * sizeof(int);
    int vertex_stride = 3 * sizeof(GLfloat);

    std::vector<int> faces(mesh.faces.size());
    for (unsigned i = 0; i < faces.size(); i++) {
        faces[i] = mesh.faces[i];
    }
    btTriangleIndexVertexArray *m_indexVertexArrays;
    m_indexVertexArrays = new btTriangleIndexVertexArray(mesh.faces.size() / 3,
                                                         &faces[0],
                                                         index_stride,
                                                         mesh.vertices.size() / 3,
                                                         (btScalar *)&mesh.vertices[0],
                                                         vertex_stride);

    btConvexTriangleMeshShape *tmpshape = new btConvexTriangleMeshShape(m_indexVertexArrays);

    btShapeHull *hull = new btShapeHull(tmpshape);
    hull->buildHull(0.0f);
    if (display_list) {
        *display_list = CreateHullDisplayList(hull);
    }

    btAlignedObjectArray<btVector3> vertices;
    for (int i = 0; i < hull->numVertices(); i++) {
        vertices.push_back(hull->getVertexPointer()[i]);
    }

    int sz;
    /*btAlignedObjectArray<btVector3> planes;
    btGeometryUtil::getPlaneEquationsFromVertices(vertices, planes);
    sz = planes.size();
    for (int i=0 ; i<sz ; ++i) {
            planes[i][3] += CONVEX_DISTANCE_MARGIN;
    }
    vertices.clear();
    btGeometryUtil::getVerticesFromPlaneEquations(planes, vertices);
*/
    sz = vertices.size();
    btConvexHullShape *convex_hull_shape = new btConvexHullShape();
    for (int i = 0; i < sz; ++i) {
        convex_hull_shape->addPoint(vertices[i]);
    }
    convex_hull_shape->setMargin(0.00f);

    delete tmpshape;
    delete hull;

    return convex_hull_shape;
}

void GetSimplifiedHull(const std::vector<vec3> &input, std::vector<vec3> &output, std::vector<int> &faces) {
    btConvexHullShape *tmpshape = CreateConvexHullShape(input);
    btShapeHull *hull = new btShapeHull(tmpshape);
    hull->buildHull(0.0f);

    const unsigned int *hull_faces = hull->getIndexPointer();
    unsigned index = 0;
    for (int i = 0; i < hull->numTriangles(); i++) {
        faces.push_back(hull_faces[index]);
        faces.push_back(hull_faces[index + 1]);
        faces.push_back(hull_faces[index + 2]);
        index += 3;
    }

    output.resize(hull->numVertices());
    for (unsigned i = 0; i < output.size(); ++i) {
        output[i] = ToVec3(hull->getVertexPointer()[i]);
    }
    delete tmpshape;
    delete hull;
}

btConvexHullShape *CreateConvexHullShape(const std::vector<vec3> &verts) {
    btConvexHullShape *convex_hull_shape = new btConvexHullShape();
    for (const auto &vert : verts) {
        convex_hull_shape->addPoint(btVector3(vert[0], vert[1], vert[2]));
    }
    convex_hull_shape->setMargin(0.00f);
    return convex_hull_shape;
}

BulletObject *BulletWorld::CreateConvexObject(const std::vector<vec3> &verts, const std::vector<int> &faces, bool is_static) {
    SharedShapePtr shape(CreateConvexHullShape(verts));

    BulletObject *obj = CreateRigidBody(shape, is_static ? 0.0f : 1.0f);

    return obj;
}

BulletObject *BulletWorld::CreateConvexModel(vec3 pos, std::string path, vec3 *offset, BWFlags flags) {
    GLuint display_list;
    SharedShapePtr shape(CreateConvexHullShape(path, &display_list, offset));

    BulletObject *obj = CreateRigidBody(shape, 1.0f, flags);

    obj->SetPosition(pos);

    return obj;
}

GLuint CreateHullDisplayList(btShapeHull *hull) {
    GLuint display_list = glGenLists(1);
    glNewList(display_list, GL_COMPILE);

    const unsigned int *faces = hull->getIndexPointer();
    const btVector3 *vertices = hull->getVertexPointer();

    glBegin(GL_TRIANGLES);
    unsigned index = 0;
    for (int i = 0; i < hull->numTriangles(); i++) {
        btVector3 normal =
            (vertices[faces[index + 1]] - vertices[faces[index + 0]]).cross((vertices[faces[index + 2]] - vertices[faces[index + 0]])).normalize();
        glNormal3f(normal[0], normal[1], normal[2]);
        glVertex3f(vertices[faces[index + 0]][0],
                   vertices[faces[index + 0]][1],
                   vertices[faces[index + 0]][2]);
        glVertex3f(vertices[faces[index + 1]][0],
                   vertices[faces[index + 1]][1],
                   vertices[faces[index + 1]][2]);
        glVertex3f(vertices[faces[index + 2]][0],
                   vertices[faces[index + 2]][1],
                   vertices[faces[index + 2]][2]);
        index += 3;
    }
    glEnd();

    glEndList();

    return display_list;
}

vec3 BulletWorld::CheckCapsuleCollision(const vec3 &start, const vec3 &end, float radius) {
    SweptSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    GetSweptSphereCollisions(start,
                             end,
                             radius,
                             cb);

    if (cb.true_closest_hit_fraction == 1.0f) {
        return end;
    }

    return start + (end - start) * cb.true_closest_hit_fraction;
}

void BulletWorld::SetJointStrength(btTypedConstraint *joint, float strength) {
    if (joint->getConstraintType() != D6_CONSTRAINT_TYPE) {
        return;
    }
    for (int i = 3; i < 6; ++i) {
        joint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, i);
        joint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, i);
        joint->setParam(BT_CONSTRAINT_CFM, 1.0f - strength, i);
    }
}

void BulletWorld::CheckRayCollisionInfo(const vec3 &start,
                                        const vec3 &end,
                                        SimpleRayResultCallbackInfo &cb,
                                        bool static_only) {
    btVector3 btstart(start[0], start[1], start[2]);
    btVector3 btend(end[0], end[1], end[2]);
    if (static_only) {
        cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    }

    dynamics_world_->rayTest(btstart, btend, cb);
    for (int i = 0; i < cb.contact_info.size(); ++i) {
        RayContactInfo &ci = cb.contact_info[i];
        ci.point = end * ci.hit_fraction + start * (1.0f - ci.hit_fraction);
    }
}

void BulletWorld::CheckRayTriCollisionInfo(const vec3 &start,
                                           const vec3 &end,
                                           SimpleRayTriResultCallback &cb,
                                           bool static_only) {
    btVector3 btstart(start[0], start[1], start[2]);
    btVector3 btend(end[0], end[1], end[2]);
    if (static_only) {
        cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    }
    dynamics_world_->rayTest(btstart, btend, cb);
    if (cb.hasHit()) {
        cb.m_hit_pos = end * cb.m_closestHitFraction + start * (1.0f - cb.m_closestHitFraction);
    }
}

void BulletWorld::UpdateAABB() {
    dynamics_world_->updateAabbs();
}

btConvexHullShape &BulletWorld::GetHullShape(const std::string &path) {
    HullShapeCacheMap::iterator hsc_iter = hull_shape_cache_.find(path);
    if (hsc_iter == hull_shape_cache_.end()) {
        Model mesh;
        mesh.LoadObj(path.c_str(), _MDL_SIMPLE);
        btConvexHullShape *shape = new btConvexHullShape();
        for (unsigned i = 0; i < mesh.vertices.size(); i += 3) {
            shape->addPoint(btVector3(mesh.vertices[i + 0], mesh.vertices[i + 1], mesh.vertices[i + 2]));
        }
        shape->setMargin(0.00f);
        hull_shape_cache_[path] = shape;
        return *shape;
    } else {
        return *hsc_iter->second;
    }
}

void BulletWorld::CreateCustomHullShape(const std::string &key, const std::vector<vec3> &points) {
    HullShapeCacheMap::iterator hsc_iter = hull_shape_cache_.find(key);
    if (hsc_iter != hull_shape_cache_.end()) {
        delete hsc_iter->second;
    }
    btConvexHullShape *shape = new btConvexHullShape();
    for (const auto &point : points) {
        shape->addPoint(btVector3(point[0], point[1], point[2]));
    }
    shape->setMargin(0.00f);
    hull_shape_cache_[key] = shape;
}

void BulletWorld::MergeShape(BulletObject &a, const BulletObject &b) {
    btCollisionShape *my_shape = a.shape.get();
    btCollisionShape *other_shape = b.shape.get();
    if (my_shape->getShapeType() == MULTI_SPHERE_SHAPE_PROXYTYPE &&
        other_shape->getShapeType() == MULTI_SPHERE_SHAPE_PROXYTYPE) {
        btMultiSphereShape *mss[2];
        mss[0] = (btMultiSphereShape *)my_shape;
        mss[1] = (btMultiSphereShape *)other_shape;
        vec3 points[4];
        {
            const btVector3 &vec = mss[0]->getSpherePosition(0);
            points[0] = vec3(vec[0], vec[1], vec[2]);
        }
        {
            const btVector3 &vec = mss[0]->getSpherePosition(1);
            points[1] = vec3(vec[0], vec[1], vec[2]);
        }
        {
            const btVector3 &vec = mss[1]->getSpherePosition(0);
            points[2] = vec3(vec[0], vec[1], vec[2]);
        }
        {
            const btVector3 &vec = mss[1]->getSpherePosition(1);
            points[3] = vec3(vec[0], vec[1], vec[2]);
        }
        points[0] = a.transform * points[0];
        points[1] = a.transform * points[1];
        points[2] = b.transform * points[2];
        points[3] = b.transform * points[3];

        float dist, farthest_dist = 0.0f;
        std::pair<int, int> farthest_pair(-1, -1);
        for (unsigned i = 0; i < 4 - 1; ++i) {
            for (unsigned j = i + 1; j < 4; ++j) {
                dist = distance_squared(points[i], points[j]);
                if (farthest_pair.first == -1 || dist > farthest_dist) {
                    farthest_dist = dist;
                    farthest_pair.first = i;
                    farthest_pair.second = j;
                }
            }
        }

        vec3 capsule_points[2];
        capsule_points[0] = invert(a.transform) * points[farthest_pair.first];
        capsule_points[1] = invert(a.transform) * points[farthest_pair.second];

        // float capsule_radii[2];
        // capsule_radii[0] = mss[0]->getSphereRadius(0)*3.0f;
        // capsule_radii[1] = mss[0]->getSphereRadius(0)*3.0f;

        btVector3 bt_capsule_points[2];
        bt_capsule_points[0] = btVector3(capsule_points[0][0],
                                         capsule_points[0][1],
                                         capsule_points[0][2]);
        bt_capsule_points[1] = btVector3(capsule_points[1][0],
                                         capsule_points[1][1],
                                         capsule_points[1][2]);

        /*
        btScalar bt_capsule_radii[2];
        bt_capsule_radii[0] = mss[0]->getSphereRadius(0);
        bt_capsule_radii[1] = mss[0]->getSphereRadius(0);
        */

        // SwapShape(new btMultiSphereShape(bt_capsule_points, bt_capsule_radii, 2), &a);

        LOGE << "Two multispheres" << std::endl;
    }
}

void BulletWorld::GetPairCollisions(btCollisionObject &a, btCollisionObject &b, btCollisionWorld::ContactResultCallback &cb) {
    dynamics_world_->contactPairTest(&a, &b, cb);
}

int BulletWorld::NumObjects() {
    return dynamic_objects_.size() + static_objects_.size();
}

void BulletWorld::CreateTempDragConstraint(BulletObject *obj, const vec3 &from, const vec3 &to) {
    btRigidBody *body = obj->body;
    body->activate();
    const btVector3 drag_point_world(from[0], from[1], from[2]);
    const btVector3 drag_point_local =
        body->getCenterOfMassTransform().inverse() * drag_point_world;

    btPoint2PointConstraint *constraint =
        new btPoint2PointConstraint(*body, drag_point_local);

    constraint->setPivotB(btVector3(to[0], to[1], to[2]));

    const float _drag_clamping = 3.0f;

    constraint->m_setting.m_impulseClamp = _drag_clamping;
    constraint->m_setting.m_tau = 0.001f;

    // float strength = 0.9f;
    // constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, -1);
    // constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, -1);

    dynamics_world_->addConstraint(constraint);

    temp_constraints_.push_back(std::pair<BulletObject *, btTypedConstraint *>(obj, constraint));
}

void BulletWorld::CreateBoneConstraint(BulletObject *obj, const vec3 &from, const vec3 &to) {
    btRigidBody *body = obj->body;
    body->activate();
    const btVector3 drag_point_world(from[0], from[1], from[2]);
    const btVector3 drag_point_local =
        body->getCenterOfMassTransform().inverse() * drag_point_world;

    btPoint2PointConstraint *constraint =
        new btPoint2PointConstraint(*body, drag_point_local);

    constraint->setPivotB(btVector3(to[0], to[1], to[2]));

    const float _drag_clamping = 0.5f;

    constraint->m_setting.m_impulseClamp = _drag_clamping;
    // constraint->m_setting.m_tau = 0.001f;

    // float strength = 0.9f;
    // constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, -1);
    // constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, -1);

    dynamics_world_->addConstraint(constraint);
    fixed_constraints_.push_back(std::pair<BulletObject *, btTypedConstraint *>(obj, constraint));
}

void BulletWorld::CreateSpikeConstraint(BulletObject *obj, const vec3 &from, const vec3 &to, const vec3 &pos) {
    vec3 axis = normalize(to - from);
    vec3 temp_perp = vec3(31.1341f, 51513.6f, 6314123.1231f);  // Arbitrary number
    vec3 perp = normalize(cross(axis, temp_perp));

    vec3 _anchor = pos;
    btVector3 parentAxis(axis[0], axis[1], axis[2]);
    btVector3 childAxis(perp[0], perp[1], perp[2]);
    btVector3 anchor = btVector3(_anchor[0], _anchor[1], _anchor[2]);

    btVector3 zAxis = parentAxis.normalize();
    btVector3 yAxis = childAxis.normalize();
    btVector3 xAxis = yAxis.cross(zAxis);  // we want right coordinate system
    btTransform frameInW;
    frameInW.setIdentity();
    frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
                                 xAxis[1], yAxis[1], zAxis[1],
                                 xAxis[2], yAxis[2], zAxis[2]);
    frameInW.setOrigin(anchor);

    btRigidBody *pBodyB = obj->body;
    btTransform frameInB = pBodyB->getCenterOfMassTransform().inverse() * frameInW;

    btGeneric6DofConstraint *constraint = new btGeneric6DofConstraint(*pBodyB, frameInB, true);

    constraint->setLinearLowerLimit(btVector3(0, 0., -distance(from, pos)));
    constraint->setLinearUpperLimit(btVector3(0, 0., distance(to, pos)));
    constraint->setAngularLowerLimit(btVector3(0.f, 0.0f, 1));
    constraint->setAngularUpperLimit(btVector3(0.f, 0.0f, -1));

    // Soften constraint a bit to stabilize
    float strength = 0.999f;
    constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, 0);  // x
    constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, 0);

    constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, 1);  // y
    constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, 1);

    constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, 2);  // z
    constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, 2);

    constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, 3);  // angular
    constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, 3);

    constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, 4);  // angular
    constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, 4);

    constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2f * strength, 5);  // angular
    constraint->setParam(BT_CONSTRAINT_STOP_CFM, 1.0f - strength, 5);

    // Add some  friction
    constraint->getTranslationalLimitMotor()->m_enableMotor[2] = true;
    constraint->getTranslationalLimitMotor()->m_targetVelocity[2] = 0.0f;
    constraint->getTranslationalLimitMotor()->m_maxMotorForce[2] = 5.0f;

    constraint->getRotationalLimitMotor(2)->m_enableMotor = true;
    constraint->getRotationalLimitMotor(2)->m_targetVelocity = 0.0f;
    constraint->getRotationalLimitMotor(2)->m_maxMotorForce = 5.0f;

    dynamics_world_->addConstraint(constraint);
    fixed_constraints_.push_back(std::pair<BulletObject *, btTypedConstraint *>(obj, constraint));
}

void BulletWorld::ClearBoneConstraints() {
    for (auto &fixed_constraint : fixed_constraints_) {
        if (fixed_constraint.second) {
            dynamics_world_->removeConstraint(fixed_constraint.second);
            delete fixed_constraint.second;
        }
    }
    fixed_constraints_.clear();
}

void BulletWorld::UpdateSingleAABB(BulletObject *bullet_object) {
    dynamics_world_->updateSingleAabb(bullet_object->body);
}

void BulletWorld::FinalizeStaticEntries() {
    // Create meta model of all models combined
    vert_indices.clear();
    vertices.clear();
    for (auto &static_entrie : static_entries) {
        Model &model = Models::Instance()->GetModel(static_entrie.model_id);
        mat4 &transform = static_entrie.transform;
        // int start_vert_indices = vert_indices.size();
        int start_vertices = vertices.size() / 3;
        for (int j = 0, len = model.vertices.size(); j < len; j += 3) {
            vec3 vec(model.vertices[j + 0], model.vertices[j + 1], model.vertices[j + 2]);
            vec = transform * vec;
            vertices.push_back(vec[0]);
            vertices.push_back(vec[1]);
            vertices.push_back(vec[2]);
        }
        for (unsigned int face : model.faces) {
            vert_indices.push_back(face + start_vertices);
        }
    }
    // Create physics object
    SharedShapePtr shape;
    ShapeDisposalData *sdd = NULL;
    sdd = new ShapeDisposalData();
    btBvhTriangleMeshShape *new_shape;
    {
        int index_stride = 3 * sizeof(int);
        int vertex_stride = 3 * sizeof(GLfloat);
        std::vector<int> &faces = sdd->faces;
        faces.resize(vert_indices.size());
        for (unsigned i = 0; i < faces.size(); i++) {
            faces[i] = vert_indices[i];
        }
        sdd->index_vert_array = new btTriangleIndexVertexArray(vert_indices.size() / 3,
                                                               &vert_indices[0],
                                                               index_stride,
                                                               vertices.size() / 3,
                                                               (btScalar *)&vertices[0],
                                                               vertex_stride);
        new_shape = new btBvhTriangleMeshShape(sdd->index_vert_array, true);
        sdd->triangle_info_map = new btTriangleInfoMap();
        btGenerateInternalEdgeInfo(new_shape, sdd->triangle_info_map);
    }
    shape.reset(new_shape);
    BulletObject *obj = CreateRigidBody(shape, 0.0f, BW_NO_FLAGS);
    if (sdd) {
        obj->shape_disposal_data.reset(sdd);
    }
    obj->body->setCollisionFlags(obj->body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
    merged_obj = obj;
}
