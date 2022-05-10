//-----------------------------------------------------------------------------
//           Name: ascollisions.cpp
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
#include "ascollisions.h"

#include <Physics/bulletworld.h>
#include <Physics/bulletcollision.h>
#include <Physics/bulletcollision.h>

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <Scripting/angelscript/ascontext.h>
#include <Main/scenegraph.h>
#include <Objects/object.h>

/*
   Adapted from http://paulbourke.net/geometry/lineline3d/
   Calculate the line segment PaPb that is the shortest route between
   two lines P1P2 and P3P4. Calculate also the values of mua and mub where
      Pa = P1 + mua (P2 - P1)
      Pb = P3 + mub (P4 - P3)
   Return FALSE if no solution exists.
*/
vec2 LineLineIntersect(
    const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p4) {
    vec3 p13, p43, p21;
    float d1343, d4321, d1321, d4343, d2121;
    float numer, denom;
    const float EPS = 0.000001f;

    p13[0] = p1[0] - p3[0];
    p13[1] = p1[1] - p3[1];
    p13[2] = p1[2] - p3[2];
    p43[0] = p4[0] - p3[0];
    p43[1] = p4[1] - p3[1];
    p43[2] = p4[2] - p3[2];
    if (fabs(p43[0]) < EPS && fabs(p43[1]) < EPS && fabs(p43[2]) < EPS)
        return (vec2(0.0f));
    p21[0] = p2[0] - p1[0];
    p21[1] = p2[1] - p1[1];
    p21[2] = p2[2] - p1[2];
    if (fabs(p21[0]) < EPS && fabs(p21[1]) < EPS && fabs(p21[2]) < EPS)
        return (vec2(0.0f));

    d1343 = p13[0] * p43[0] + p13[1] * p43[1] + p13[2] * p43[2];
    d4321 = p43[0] * p21[0] + p43[1] * p21[1] + p43[2] * p21[2];
    d1321 = p13[0] * p21[0] + p13[1] * p21[1] + p13[2] * p21[2];
    d4343 = p43[0] * p43[0] + p43[1] * p43[1] + p43[2] * p43[2];
    d2121 = p21[0] * p21[0] + p21[1] * p21[1] + p21[2] * p21[2];

    denom = d2121 * d4343 - d4321 * d4321;
    if (fabs(denom) < EPS)
        return (vec2(0.0f));
    numer = d1343 * d4321 - d1321 * d4343;

    float mua = numer / denom;
    float mub = (d1343 + d4321 * mua) / d4343;

    return vec2(mua, mub);
}

vec3 ASLineLineIntersect(vec3 p1, vec3 p2, vec3 p3, vec3 p4) {
    vec2 vec = LineLineIntersect(p1, p2, p3, p4);
    return vec3(vec[0], vec[1], 0.0f);
}

void ASCollisions::ASGetSlidingSphereCollision(vec3 pos, float radius) {
    GetSlidingSphereCollision(pos, radius, as_col, SINGLE_SIDED);
}

void ASCollisions::ASGetSlidingSphereCollisionDoubleSided(vec3 pos, float radius) {
    GetSlidingSphereCollision(pos, radius, as_col, DOUBLE_SIDED);
}

void ASCollisions::GetSlidingSphereCollision(vec3 pos,
                                             float radius,
                                             SphereCollision& col,
                                             CollisionSides sides) {
    col.position = pos;
    col.radius = radius;

    ContactSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    if (sides == DOUBLE_SIDED) {
        cb.single_sided = false;
    }

    BulletWorld* bullet_world = scenegraph->bullet_world_;
    bullet_world->GetSphereCollisions(col.position, col.radius, cb);
    col.adjusted_position = bullet_world->ApplySphereSlide(col.position,
                                                           col.radius,
                                                           cb.collision_info);

    col.SetFromCallback(cb);
}

void ASCollisions::GetSlidingScaledSphereCollision(vec3 pos,
                                                   float radius,
                                                   vec3 scale,
                                                   SphereCollision& col) {
    col.position = pos;
    col.radius = radius;

    ContactSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;

    BulletWorld* bullet_world = scenegraph->bullet_world_;
    bullet_world->GetScaledSphereCollisions(col.position, col.radius, scale, cb);
    col.adjusted_position = bullet_world->ApplyScaledSphereSlide(col.position,
                                                                 col.radius,
                                                                 scale,
                                                                 cb.collision_info);

    col.SetFromCallback(cb);
}

void ASCollisions::ASGetScaledSphereCollision(vec3 pos,
                                              float radius,
                                              vec3 scale) {
    GetScaledSphereCollision(pos, radius, scale, as_col);
}

void ASCollisions::ASGetScaledSpherePlantCollision(vec3 pos,
                                                   float radius,
                                                   vec3 scale) {
    GetScaledSphereCollision(pos, radius, scale, as_col, true);
    as_col.position = pos;
    as_col.radius = radius;

    ContactInfoCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;

    BulletWorld* bw = scenegraph->plant_bullet_world_;
    bw->GetScaledSphereCollisions(as_col.position, as_col.radius, scale, cb);

    as_col.SetFromCallback(cb);
}

void ASCollisions::ASGetSlidingScaledSphereCollision(vec3 pos,
                                                     float radius,
                                                     vec3 scale) {
    GetSlidingScaledSphereCollision(pos, radius, scale, as_col);
}

void ASCollisions::GetScaledSphereCollision(vec3 pos,
                                            float radius,
                                            vec3 scale,
                                            SphereCollision& col,
                                            bool plant_world) {
    col.position = pos;
    col.radius = radius;

    ContactSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;

    BulletWorld* bullet_world =
        plant_world ? scenegraph->plant_bullet_world_ : scenegraph->bullet_world_;
    bullet_world->GetScaledSphereCollisions(col.position, col.radius, scale, cb);

    col.SetFromCallback(cb);
}

void ASCollisions::ASGetCylinderCollision(vec3 pos,
                                          float radius,
                                          float height) {
    GetCylinderCollision(pos, radius, height, as_col, SINGLE_SIDED);
}

void ASCollisions::ASGetCylinderCollisionDoubleSided(vec3 pos,
                                                     float radius,
                                                     float height) {
    GetCylinderCollision(pos, radius, height, as_col, DOUBLE_SIDED);
}

void ASCollisions::GetCylinderCollision(vec3 pos,
                                        float radius,
                                        float height,
                                        SphereCollision& col,
                                        CollisionSides sides) {
    col.position = pos;
    col.radius = radius;

    ContactSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    if (sides == DOUBLE_SIDED) {
        cb.single_sided = false;
    }

    BulletWorld* bullet_world = scenegraph->bullet_world_;
    bullet_world->GetCylinderCollisions(col.position, col.radius, height, cb);

    col.SetFromCallback(cb);
}

void ASCollisions::ASGetSweptSphereCollision(const vec3& pos,
                                             const vec3& pos2,
                                             float radius) {
    GetSweptSphereCollision(pos, pos2, radius, as_col);
}

void ASCollisions::ASGetSweptBoxCollision(vec3 pos,
                                          vec3 pos2,
                                          vec3 dimensions) {
    GetSweptBoxCollision(pos, pos2, dimensions, as_col);
}

void ASCollisions::GetSweptSphereCollision(const vec3& pos, const vec3& pos2, float radius, SphereCollision& col) {
    col.radius = radius;
    SweptSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    BulletWorld* bullet_world = scenegraph->bullet_world_;
    bullet_world->GetSweptSphereCollisions(pos, pos2, col.radius, cb);
    col.adjusted_position = bullet_world->ApplySphereSlide(pos2, col.radius, cb.collision_info);
    col.SetFromCallback(pos, pos2, cb);
}

void ASCollisions::GetSweptBoxCollision(vec3 pos,
                                        vec3 pos2,
                                        vec3 dimensions,
                                        SphereCollision& col) {
    SweptSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;

    BulletWorld* bullet_world = scenegraph->bullet_world_;
    bullet_world->GetSweptBoxCollisions(pos, pos2, dimensions, cb);

    col.SetFromCallback(pos, pos2, cb);
}

void ASCollisions::GetSweptCylinderCollision(vec3 pos,
                                             vec3 pos2,
                                             float radius,
                                             float height,
                                             SphereCollision& col,
                                             CollisionSides sides) {
    SweptSlideCallback cb;
    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
    if (sides == DOUBLE_SIDED) {
        cb.single_sided = false;
    }

    BulletWorld* bullet_world = scenegraph->bullet_world_;
    bullet_world->GetSweptCylinderCollisions(pos, pos2, radius, height, cb);

    col.SetFromCallback(pos, pos2, cb);
}

void ASCollisions::ASGetSweptCylinderCollision(vec3 pos,
                                               vec3 pos2,
                                               float radius,
                                               float height) {
    GetSweptCylinderCollision(pos, pos2, radius, height, as_col, SINGLE_SIDED);
}

void ASCollisions::ASGetSweptCylinderCollisionDoubleSided(vec3 pos,
                                                          vec3 pos2,
                                                          float radius,
                                                          float height) {
    GetSweptCylinderCollision(pos, pos2, radius, height, as_col, DOUBLE_SIDED);
}

void SphereCollision::SetFromCallback(const SweptSlideCallback& cb) {
    contacts.resize(cb.collision_info.contacts.size());
    for (unsigned i = 0; i < contacts.size(); i++) {
        contacts[i].position = cb.collision_info.contacts[i].point;
        contacts[i].normal = cb.collision_info.contacts[i].normal;
        contacts[i].custom_normal = cb.collision_info.contacts[i].custom_normal;
        contacts[i].id = cb.collision_info.contacts[i].obj_id;
        contacts[i].tri = cb.collision_info.contacts[i].tri;
    }
}

void SphereCollision::SetFromCallback(const ContactSlideCallback& cb) {
    contacts.resize(cb.collision_info.contacts.size());
    for (unsigned i = 0; i < contacts.size(); i++) {
        contacts[i].position = cb.collision_info.contacts[i].point;
        contacts[i].normal = cb.collision_info.contacts[i].normal;
        contacts[i].custom_normal = cb.collision_info.contacts[i].custom_normal;
        contacts[i].id = cb.collision_info.contacts[i].obj_id;
        contacts[i].tri = cb.collision_info.contacts[i].tri;
    }
}

void SphereCollision::SetFromCallback(const ContactInfoCallback& cb) {
    contacts.resize(cb.contact_info.size());
    for (unsigned i = 0; i < contacts.size(); i++) {
        contacts[i].position = ToVec3(cb.contact_info[i].point);
        contacts[i].normal = vec3(0.0f);
        contacts[i].id = -1;
        BulletObject* obj = cb.contact_info[i].object;
        if (obj && obj->owner_object) {
            contacts[i].id = obj->owner_object->GetID();
        }
    }
}

void SphereCollision::SetFromCallback(const SimpleRayResultCallbackInfo& cb) {
    contacts.resize(cb.contact_info.size());
    for (unsigned i = 0; i < contacts.size(); i++) {
        contacts[i].position = cb.contact_info[i].point;
        contacts[i].normal = vec3(0.0f);
        contacts[i].id = -1;
        BulletObject* obj = cb.contact_info[i].object;
        if (obj && obj->owner_object) {
            contacts[i].id = obj->owner_object->GetID();
        }
    }
}

void SphereCollision::SetFromCallback(
    const vec3& pos,
    const vec3& pos2,
    const SweptSlideCallback& cb) {
    SetFromCallback(cb);

    if (contacts.empty()) {
        position = pos2;
    } else {
        position = mix(pos, pos2, cb.true_closest_hit_fraction);
    }
}

static void ASGetPlantRayCollision(ASCollisions* as_collisions, vec3 start, vec3 end) {
    as_collisions->as_col.contacts.clear();

    BulletWorld* bullet_world = as_collisions->scenegraph->plant_bullet_world_;
    SimpleRayResultCallbackInfo cb;
    bullet_world->CheckRayCollisionInfo(start, end, cb, true);
    as_collisions->as_col.SetFromCallback(cb);
}

void ASCollisions::AttachToContext(ASContext* as_context) {
    as_context->RegisterEnum("CollisionSides");
    as_context->RegisterEnumValue("CollisionSides", "DOUBLE_SIDED", DOUBLE_SIDED);
    as_context->RegisterEnumValue("CollisionSides", "SINGLE_SIDED", SINGLE_SIDED);
    as_context->DocsCloseBrace();

    as_context->RegisterObjectType("CollisionPoint", sizeof(CollisionPoint), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
    as_context->RegisterObjectProperty("CollisionPoint", "vec3 position", asOFFSET(CollisionPoint, position));
    as_context->RegisterObjectProperty("CollisionPoint", "vec3 normal", asOFFSET(CollisionPoint, normal));
    as_context->RegisterObjectProperty("CollisionPoint", "vec3 custom_normal", asOFFSET(CollisionPoint, custom_normal));
    as_context->RegisterObjectProperty("CollisionPoint", "int id", asOFFSET(CollisionPoint, id));
    as_context->RegisterObjectProperty("CollisionPoint", "int tri", asOFFSET(CollisionPoint, tri));
    as_context->DocsCloseBrace();

    as_context->RegisterObjectType("SphereCollision", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectProperty("SphereCollision", "vec3 position", asOFFSET(SphereCollision, position));
    as_context->RegisterObjectProperty("SphereCollision", "vec3 adjusted_position", asOFFSET(SphereCollision, adjusted_position), "Position after collision response is applied");
    as_context->RegisterObjectProperty("SphereCollision", "float radius", asOFFSET(SphereCollision, radius));
    as_context->RegisterObjectMethod("SphereCollision", "int NumContacts()", asMETHOD(SphereCollision, NumContacts), asCALL_THISCALL);
    as_context->RegisterObjectMethod("SphereCollision", "CollisionPoint GetContact(int)", asMETHOD(SphereCollision, GetContact), asCALL_THISCALL);
    as_context->DocsCloseBrace();

    as_context->RegisterObjectType("ASCollisions", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectMethod("ASCollisions", "vec3 GetSlidingCapsuleCollision(vec3 start, vec3 end, float radius)", asMETHOD(ASCollisions, GetSlidingCapsuleCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSlidingSphereCollision(vec3 pos, float radius)", asMETHOD(ASCollisions, ASGetSlidingSphereCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSlidingSphereCollisionDoubleSided(vec3 pos, float radius)", asMETHOD(ASCollisions, ASGetSlidingSphereCollisionDoubleSided), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetScaledSphereCollision(vec3 pos, float radius, vec3 scale)", asMETHOD(ASCollisions, ASGetScaledSphereCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetScaledSpherePlantCollision(vec3 pos, float radius, vec3 scale)", asMETHOD(ASCollisions, ASGetScaledSpherePlantCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSlidingScaledSphereCollision(vec3 pos, float radius, vec3 scale)", asMETHOD(ASCollisions, ASGetSlidingScaledSphereCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetCylinderCollision(vec3 pos, float radius, float height)", asMETHOD(ASCollisions, ASGetCylinderCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetCylinderCollisionDoubleSided(vec3 pos, float radius, float height)", asMETHOD(ASCollisions, ASGetCylinderCollisionDoubleSided), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSweptSphereCollision(const vec3 &in start, const vec3 &in end, float radius)", asMETHOD(ASCollisions, ASGetSweptSphereCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSweptSphereCollisionCharacters(vec3 start, vec3 end, float radius)", asMETHOD(ASCollisions, ASGetSweptSphereCollisionCharacters), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSweptCylinderCollision(vec3 start, vec3 end, float radius, float height)", asMETHOD(ASCollisions, ASGetSweptCylinderCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSweptCylinderCollisionDoubleSided(vec3 start, vec3 end, float radius, float height)", asMETHOD(ASCollisions, ASGetSweptCylinderCollisionDoubleSided), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetSweptBoxCollision(vec3 start, vec3 end, vec3 dimensions)", asMETHOD(ASCollisions, ASGetSweptBoxCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void CheckRayCollisionCharacters(vec3 start, vec3 end)", asMETHOD(ASCollisions, ASCheckRayCollisionCharacters), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "vec3 GetRayCollision(vec3 start, vec3 end)", asMETHOD(ASCollisions, GetRayCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetObjRayCollision(vec3 start, vec3 end)", asMETHOD(ASCollisions, ASGetObjRayCollision), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ASCollisions", "void GetPlantRayCollision(vec3 start, vec3 end)", asFUNCTION(ASGetPlantRayCollision), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ASCollisions", "void GetObjectsInSphere(vec3 pos, float radius)", asMETHOD(ASCollisions, ASGetObjectsInSphere), asCALL_THISCALL);
    as_context->DocsCloseBrace();
    as_context->RegisterGlobalProperty("ASCollisions col", this, "Used to access collision functions");
    as_context->RegisterGlobalProperty("SphereCollision sphere_col", &as_col, "Stores results of collision functions");

    as_context->RegisterGlobalFunction("vec3 LineLineIntersect(vec3 start_a, vec3 end_a, vec3 start_b, vec3 end_b)", asFUNCTION(ASLineLineIntersect), asCALL_CDECL, "Get closest point between two line segments");
}

ASCollisions::ASCollisions(SceneGraph* _scenegraph) : scenegraph(_scenegraph) {}

vec3 ASCollisions::GetSlidingCapsuleCollision(vec3 pos, vec3 pos2, float radius) {
    return scenegraph->bullet_world_->CheckCapsuleCollisionSlide(pos, pos2, radius);
}

vec3 ASCollisions::GetRayCollision(vec3 pos, vec3 pos2) {
    vec3 point;
    scenegraph->bullet_world_->CheckRayCollision(pos, pos2, &point);
    return point;
}

void ASCollisions::ASGetObjRayCollision(vec3 start, vec3 end) {
    as_col.contacts.clear();
    GetObjRayCollision(start, end, as_col);
}

void ASCollisions::GetObjRayCollision(vec3 pos, vec3 pos2, SphereCollision& col) {
    BulletWorld* bullet_world = scenegraph->bullet_world_;
    SimpleRayResultCallbackInfo cb;
    // SweptSlideCallback cb;
    bullet_world->CheckRayCollisionInfo(pos, pos2, cb, true);
    // bullet_world->GetSweptCylinderCollisions(pos, pos2, 1.0f, 1.0f, cb);

    col.SetFromCallback(cb);
}

void ASCollisions::ASGetObjectsInSphere(vec3 pos, float radius) {
    as_col.contacts.clear();
    GetObjectsInSphere(pos, radius, as_col);
}

void ASCollisions::GetObjectsInSphere(vec3 pos, float radius, SphereCollision& col) {
    ContactInfoCallback cb;
    // scenegraph->abstract_bullet_world_->GetSphereCollisions(pos, radius, cb);
    scenegraph->bullet_world_->GetSphereCollisions(pos, radius, cb);
    SimpleRayResultCallbackInfo scb;

    for (int i = 0; i < cb.contact_info.size(); ++i) {
        // RayContactInfo &ci = scb.contact_info[i];
    }
    col.SetFromCallback(cb);
}

void ASCollisions::ASGetSweptSphereCollisionCharacters(vec3 pos, vec3 pos2, float radius) {
    return scenegraph->GetSweptSphereCollisionCharacters(pos, pos2, radius, as_col);
}

void ASCollisions::ASCheckRayCollisionCharacters(vec3 pos, vec3 pos2) {
    as_col.contacts.clear();
    as_col.position = pos2;
    vec3 point, normal;
    int bone = -1;
    int hit = scenegraph->CheckRayCollisionCharacters(pos, pos2, &point, &normal, &bone);
    if (hit != -1) {
        as_col.position = point;
        as_col.contacts.resize(1);
        as_col.contacts.back().position = point;
        as_col.contacts.back().normal = normal;
        as_col.contacts.back().id = hit;
        as_col.contacts.back().tri = bone;
    } else {
        as_col.contacts.resize(0);
    }
}

ASCollisions::~ASCollisions() {}
