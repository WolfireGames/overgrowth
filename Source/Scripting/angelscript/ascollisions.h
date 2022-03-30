//-----------------------------------------------------------------------------
//           Name: ascollisions.h
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

#include <vector>

class SceneGraph;
class ASContext;

struct CollisionPoint {
    vec3 position;
    vec3 normal;
    vec3 custom_normal;
    int id;
    int tri;
};

class SweptSlideCallback;
class ContactSlideCallback;
class ContactInfoCallback;
struct SimpleRayResultCallbackInfo;

struct SphereCollision {
    float radius;
    vec3 position;
    vec3 adjusted_position;
    std::vector<CollisionPoint> contacts;
    size_t NumContacts() {return contacts.size();}
    CollisionPoint GetContact(int which) {return contacts[which];}
    void SetFromCallback( const vec3 &pos, const vec3 &pos2, const SweptSlideCallback &cb);
    void SetFromCallback( const SweptSlideCallback &cb);
    void SetFromCallback( const ContactSlideCallback &cb);
    void SetFromCallback( const ContactInfoCallback &cb);
	void SetFromCallback( const SimpleRayResultCallbackInfo &cb);
};

class ASCollisions {
public:
    enum CollisionSides {
        SINGLE_SIDED,
        DOUBLE_SIDED
    };
    void GetSlidingSphereCollision(vec3 pos, float radius, SphereCollision& col, CollisionSides sides);
    vec3 GetSlidingCapsuleCollision( vec3 pos, vec3 pos2, float radius );
    void ASGetSlidingSphereCollision(vec3 pos, float radius);
    void ASGetSlidingSphereCollisionDoubleSided(vec3 pos, float radius);
    void ASGetSweptSphereCollision(const vec3& pos, const vec3& pos2, float radius);
    void ASGetSweptSphereCollisionCharacters(vec3 pos, vec3 pos2, float radius);
    void GetSweptSphereCollision(const vec3& pos, const vec3& pos2, float radius, SphereCollision& col);
    void GetSweptBoxCollision(vec3 pos, vec3 pos2, vec3 dimensions, SphereCollision& col);
    void ASGetSweptBoxCollision(vec3 pos, vec3 pos2, vec3 dimensions);
    void GetSweptCylinderCollision( vec3 pos, vec3 pos2, float radius, float height, SphereCollision& col, CollisionSides sides );
    void ASGetSweptCylinderCollisionDoubleSided(vec3 pos, vec3 pos2, float radius, float height);
    void ASGetSweptCylinderCollision(vec3 pos, vec3 pos2, float radius, float height);
    void GetCylinderCollision(vec3 pos, float radius, float height, SphereCollision& col, CollisionSides sides);
    void ASGetCylinderCollision(vec3 pos, float radius, float height);
    void ASGetCylinderCollisionDoubleSided(vec3 pos, float radius, float height);
    void GetScaledSphereCollision(vec3 pos, float radius, vec3 scale, SphereCollision& col, bool plant_world = false);
    void ASGetScaledSphereCollision(vec3 pos, float radius, vec3 scale);
    void ASGetScaledSpherePlantCollision(vec3 pos, float radius, vec3 scale);
    void GetSlidingScaledSphereCollision(vec3 pos, float radius, vec3 scale, SphereCollision& col);
    void ASGetSlidingScaledSphereCollision(vec3 pos, float radius, vec3 scale);
    void ASCheckRayCollisionCharacters( vec3 pos, vec3 pos2 );
    vec3 GetRayCollision( vec3 pos, vec3 pos2);
	void ASGetObjRayCollision( vec3 pos, vec3 pos2);
	void GetObjRayCollision( vec3 start, vec3 end, SphereCollision& col);
	void ASGetObjectsInSphere(vec3 pos, float radius);
	void GetObjectsInSphere(vec3 pos, float radius, SphereCollision& col);

    SceneGraph *scenegraph;
    SphereCollision as_col;
    void AttachToContext(ASContext *as_context);

    ASCollisions(SceneGraph* _scenegraph);
    ~ASCollisions();
};
