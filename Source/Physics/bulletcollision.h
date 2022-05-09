//-----------------------------------------------------------------------------
//           Name: bulletcollision.h
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

#include <btBulletDynamicsCommon.h>

#include <map>
#include <vector>
#include <set>

typedef btAlignedObjectArray<btVector3> btVec3Array;

struct SlideCollisionContact {
    vec3 point;
    vec3 normal;
    vec3 custom_normal;
    int obj_id;
    int tri;
};

struct SlideCollisionInfo {
    std::vector<SlideCollisionContact> contacts;
};

class BulletObject;

class ContactSlideCallback : public btCollisionWorld::ContactResultCallback {
   public:
    SlideCollisionInfo collision_info;
    bool single_sided;
    ContactSlideCallback() : single_sided(true) {}

    btScalar addSingleResult(btManifoldPoint& cp,
                             const btCollisionObjectWrapper* colObj0Wrap,
                             int partId0,
                             int index0,
                             const btCollisionObjectWrapper* colObj1Wrap,
                             int partId1,
                             int index1) override;
};

typedef std::map<BulletObject*, std::vector<int> > TriListResults;

class TriListCallback : public btCollisionWorld::ContactResultCallback {
   public:
    TriListResults& tri_list;
    TriListCallback(TriListResults& _tri_list) : tri_list(_tri_list) {}
    btScalar addSingleResult(btManifoldPoint& cp,
                             const btCollisionObjectWrapper* colObj0Wrap,
                             int partId0,
                             int index0,
                             const btCollisionObjectWrapper* colObj1Wrap,
                             int partId1,
                             int index1) override;
};

class MeshCollisionCallback : public btCollisionWorld::ContactResultCallback {
   public:
    typedef std::pair<int, int> TriPair;
    typedef std::set<TriPair> TriPairSet;
    TriPairSet tri_pairs;
    btScalar addSingleResult(btManifoldPoint& cp,
                             const btCollisionObjectWrapper* colObj0Wrap,
                             int partId0,
                             int index0,
                             const btCollisionObjectWrapper* colObj1Wrap,
                             int partId1,
                             int index1) override;
};

struct ContactInfo {
    BulletObject* object;
    btVector3 point;
    int tri;
};

class ContactInfoCallback : public btCollisionWorld::ContactResultCallback {
   public:
    btAlignedObjectArray<ContactInfo> contact_info;
    btScalar addSingleResult(btManifoldPoint& cp,
                             const btCollisionObjectWrapper* colObj0Wrap,
                             int partId0,
                             int index0,
                             const btCollisionObjectWrapper* colObj1Wrap,
                             int partId1,
                             int index1) override;
};

class RayContactInfoCallback : public btCollisionWorld::ContactResultCallback {
   public:
    btAlignedObjectArray<ContactInfo> contact_info;
    btScalar addSingleResult(btManifoldPoint& cp,
                             const btCollisionObjectWrapper* colObj0Wrap,
                             int partId0,
                             int index0,
                             const btCollisionObjectWrapper* colObj1Wrap,
                             int partId1,
                             int index1) override;
};

class SweptSlideCallback : public btCollisionWorld::ConvexResultCallback {
   public:
    SlideCollisionInfo collision_info;
    bool single_sided;
    float true_closest_hit_fraction;
    btVector3 start_pos, end_pos;

    SweptSlideCallback() : single_sided(true),
                           true_closest_hit_fraction(1.0f) {}

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,
                             bool normalInWorldSpace) override;
};

struct RayContactInfo {
    BulletObject* object;
    vec3 point;
    vec3 normal;
    float hit_fraction;
};

struct SimpleRayResultCallbackInfo : public btCollisionWorld::RayResultCallback {
    btAlignedObjectArray<RayContactInfo> contact_info;
    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
                             bool normalInWorldSpace) override;
};

struct SimpleRayResultCallback : public btCollisionWorld::RayResultCallback {
    vec3 m_hit_normal;
    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
                             bool normalInWorldSpace) override;

    const vec3& GetHitNormal();
};

struct SimpleRayTriResultCallback : public btCollisionWorld::RayResultCallback {
    vec3 m_hit_normal;
    int m_tri;
    vec3 m_hit_pos;
    BulletObject* m_object;
    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
                             bool normalInWorldSpace) override;

    const vec3& GetHitNormal();
};

inline vec3 ToVec3(const btVector3& vec) {
    return vec3(vec[0], vec[1], vec[2]);
}

inline btVector3 ToBtVector3(const vec3& vec) {
    return btVector3(vec[0], vec[1], vec[2]);
}
