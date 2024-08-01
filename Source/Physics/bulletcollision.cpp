//-----------------------------------------------------------------------------
//           Name: bulletcollision.cpp
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
#include <Math/vec3.h>
#include <Math/vec3math.h>
#include <Math/vec4.h>

#include <Physics/bulletcollision.h>
#include <Physics/bulletobject.h>

#include <Objects/object.h>
#include <Objects/envobject.h>

#include <Graphics/pxdebugdraw.h>
#include <Math/enginemath.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

btScalar ContactSlideCallback::addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObjWrap0, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
    // LOG_ASSERT2(index0 >= 0, index0);
    LOG_ASSERT_ONCE(index1 >= 0);

    collision_info.contacts.resize(collision_info.contacts.size() + 1);
    btVector3 normal_world = cp.m_normalWorldOnB;
    btVector3 override_dir;
    if (single_sided) {
        // Modify normal to use single-sided collision with triangle meshes
        static const bool draw_norm = false;
        btVector3 btNormDir;
        if (colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE) {
            const btTriangleShape* tri_shape = static_cast<const btTriangleShape*>(colObj1Wrap->getCollisionShape());
            btVector3 tri_normal;
            tri_shape->calcNormal(tri_normal);
            btNormDir = quatRotate(colObj1Wrap->getWorldTransform().getRotation(), tri_normal);
            if (colObj1Wrap->m_collisionObject && colObj1Wrap->m_collisionObject->getUserPointer() && ((BulletObject*)colObj1Wrap->m_collisionObject->getUserPointer())->owner_object) {
                Object* obj = ((BulletObject*)colObj1Wrap->m_collisionObject->getUserPointer())->owner_object;
                collision_info.contacts.back().obj_id = obj->GetID();
                if (obj->GetType() == _env_object) {
                    EnvObject* eo = (EnvObject*)obj;
                    collision_info.contacts.back().tri = index1;
                    if ((unsigned)index1 < eo->normal_override_custom.size()) {
                        collision_info.contacts.back().custom_normal = eo->normal_override_custom[index1].xyz();
                    }
                }
            }
            if (draw_norm) {
                const btTransform& bt_transform = colObj1Wrap->getCollisionObject()->getWorldTransform();
                btVector3 bt_vert[3];
                for (int i = 0; i < 3; ++i) {
                    tri_shape->getVertex(i, bt_vert[i]);
                    bt_vert[i] = bt_transform * bt_vert[i];
                }
                btVector3 mid = (bt_vert[0] + bt_vert[1] + bt_vert[2]) / 3.0f;
                DebugDraw::Instance()->AddLine(ToVec3(mid), ToVec3(mid + btNormDir), vec4(1.0f), _fade);
            }
        }
        if (btNormDir.dot(cp.m_normalWorldOnB) < 0.0f) {
            return 1.0f;
        }
    }

    btVector3 point_world = cp.m_positionWorldOnB;

    normal_world.normalize();

    collision_info.contacts.back().normal = vec3(normal_world[0], normal_world[1], normal_world[2]);
    collision_info.contacts.back().point = vec3(point_world[0], point_world[1], point_world[2]);

    return 1.0f;
}

btScalar SweptSlideCallback::addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) {
    collision_info.contacts.resize(collision_info.contacts.size() + 1);
    const btCollisionShape* shape = convexResult.m_hitCollisionObject->getCollisionShape();
    int shape_type = shape->getShapeType();
    btVector3 override_dir;
    if (single_sided) {
        // Get single-sided triangle normal to eliminate backface collisions
        btVector3 btNormDir;
        if (shape_type == TRIANGLE_MESH_SHAPE_PROXYTYPE || shape_type == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE) {
            btTriangleMeshShape* tri_mesh_shape;
            switch (shape_type) {
                case TRIANGLE_MESH_SHAPE_PROXYTYPE:
                    tri_mesh_shape = (btTriangleMeshShape*)shape;
                    break;
                case SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE:
                    tri_mesh_shape = ((btScaledBvhTriangleMeshShape*)shape)->getChildShape();
                    break;
            }
            btStridingMeshInterface* mesh_interface = tri_mesh_shape->getMeshInterface();

            const unsigned char* vertexbase;
            int numverts;
            PHY_ScalarType type;
            int stride;
            const unsigned char* indexbase;
            int indexstride;
            int numfaces;
            PHY_ScalarType indicestype;
            mesh_interface->getLockedReadOnlyVertexIndexBase(&vertexbase, numverts, type, stride, &indexbase, indexstride, numfaces, indicestype, 0);

            const btTransform& bt_transform = convexResult.m_hitCollisionObject->getWorldTransform();
            const btVector3& scale = shape->getLocalScaling();

            int tri_index = convexResult.m_localShapeInfo->m_triangleIndex;
            LOG_ASSERT(tri_index >= 0);
            btVector3 bt_vert[3];
            for (int i = 0; i < 3; ++i) {
                int index = ((int*)indexbase)[tri_index * indexstride / sizeof(int) + i] * stride / sizeof(float);
                for (int j = 0; j < 3; ++j) {
                    bt_vert[i][j] = ((float*)vertexbase)[index + j];
                }
                bt_vert[i] *= scale;
                bt_vert[i] = bt_transform * bt_vert[i];
            }
            btNormDir = btCross(bt_vert[1] - bt_vert[0], bt_vert[2] - bt_vert[0]);
            mesh_interface->unLockReadOnlyVertexBase(0);
            if (convexResult.m_hitCollisionObject && convexResult.m_hitCollisionObject->getUserPointer() && ((BulletObject*)convexResult.m_hitCollisionObject->getUserPointer())->owner_object) {
                Object* obj = ((BulletObject*)convexResult.m_hitCollisionObject->getUserPointer())->owner_object;
                collision_info.contacts.back().obj_id = obj->GetID();
                if (obj->GetType() == _env_object) {
                    EnvObject* eo = (EnvObject*)obj;
                    collision_info.contacts.back().tri = tri_index;
                    if ((unsigned)tri_index < eo->normal_override_custom.size()) {
                        collision_info.contacts.back().custom_normal = eo->normal_override_custom[tri_index].xyz();
                    }
                }
            }
        }
        if (btNormDir.dot(end_pos - start_pos) > 0.0f) {
            return 1.0f;
        }
    }

    if (collision_info.contacts.size() == 0) {
        true_closest_hit_fraction = convexResult.m_hitFraction;
    } else {
        true_closest_hit_fraction = min(convexResult.m_hitFraction,
                                        true_closest_hit_fraction);
    }

    btVector3 normal_world;
    if (normalInWorldSpace) {
        normal_world = convexResult.m_hitNormalLocal;
    } else {
        const btVector3& local_normal = convexResult.m_hitNormalLocal;
        if (shape_type == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE) {
            normal_world = local_normal;
        } else {
            const btTransform& normal_transform =
                convexResult.m_hitCollisionObject->getWorldTransform();
            normal_world = quatRotate(normal_transform.getRotation(), local_normal);
        }
    }
    normal_world.normalize();

    if (normal_world.dot(end_pos - start_pos) > 0.0f) {
        return 1.0f;
    }

    btVector3 point_world = convexResult.m_hitPointLocal;
    collision_info.contacts.back().normal = vec3(normal_world[0], normal_world[1], normal_world[2]);
    collision_info.contacts.back().point = vec3(point_world[0], point_world[1], point_world[2]);

    // These are set to 1.0 in order to return all collisions, and
    // not just the first one.
    this->m_closestHitFraction = 1.0f;
    return 1.0f;
}

btScalar ContactInfoCallback::addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObjWrap0, int partId0, int index0, const btCollisionObjectWrapper* colObjWrap1, int partId1, int index1) {
    btVector3 point_world = cp.m_positionWorldOnB;

    ContactInfo ci;
    ci.object = (BulletObject*)colObjWrap1->getCollisionObject()->getUserPointer();
    if (!ci.object) {
        ci.object = (BulletObject*)colObjWrap0->getCollisionObject()->getUserPointer();
    }
    ci.point = point_world;
    ci.tri = index1;
    contact_info.push_back(ci);

    return 1.0f;
}

btScalar SimpleRayResultCallbackInfo::addSingleResult(
    btCollisionWorld::LocalRayResult& cp,
    bool normal_in_world_space) {
    RayContactInfo ci;
    ci.object = (BulletObject*)cp.m_collisionObject->getUserPointer();
    ci.hit_fraction = cp.m_hitFraction;
    contact_info.push_back(ci);
    return 1.0f;
}

btScalar SimpleRayTriResultCallback::addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
    m_closestHitFraction = rayResult.m_hitFraction;
    m_collisionObject = rayResult.m_collisionObject;
    m_hit_normal = vec3(rayResult.m_hitNormalLocal[0],
                        rayResult.m_hitNormalLocal[1],
                        rayResult.m_hitNormalLocal[2]);
    if (rayResult.m_localShapeInfo) {
        m_tri = rayResult.m_localShapeInfo->m_triangleIndex;
    }
    m_object = (BulletObject*)rayResult.m_collisionObject->getUserPointer();
    return rayResult.m_hitFraction;
}

btScalar SimpleRayResultCallback::addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
    m_closestHitFraction = rayResult.m_hitFraction;
    m_collisionObject = rayResult.m_collisionObject;
    m_hit_normal = vec3(rayResult.m_hitNormalLocal[0],
                        rayResult.m_hitNormalLocal[1],
                        rayResult.m_hitNormalLocal[2]);
    return rayResult.m_hitFraction;
}

const vec3& SimpleRayResultCallback::GetHitNormal() {
    return m_hit_normal;
}

btScalar TriListCallback::addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObjWrap0, int partId0, int index0, const btCollisionObjectWrapper* colObjWrap1, int partId1, int index1) {
    BulletObject* obj = (BulletObject*)colObjWrap1->getCollisionObject()->getUserPointer();
    if (obj) {
        tri_list[obj].push_back(index1);
    }
    return 1.0f;
}

vec3 vecFromBT(const btVector3& bt_vec) {
    return vec3(bt_vec[0], bt_vec[1], bt_vec[2]);
}

btScalar MeshCollisionCallback::addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
    TriPair pair;
    pair.first = index0;
    pair.second = index1;
    tri_pairs.insert(pair);
    return 1.0f;
}
