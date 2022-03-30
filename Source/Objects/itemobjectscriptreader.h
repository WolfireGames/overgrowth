//-----------------------------------------------------------------------------
//           Name: itemobjectscriptreader.h
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

#include <Game/attachment_type.h>
#include <Asset/Asset/attachmentasset.h>

#include <string>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

class ASContext;
class ItemObject;
class BulletObject;
class btTypedConstraint;

class ItemObjectScriptReader {
public:
    ItemObject* obj;
    btTypedConstraint* constraint;
    mat4 velocity;
    bool just_created;
    bool holding;
    bool stuck;
    int char_id;
    AttachmentType attachment_type;
    AttachmentRef attachment_ref;
    bool attachment_mirror;
    void* callback_ptr_;
    void (*invalidate_callback)(ItemObjectScriptReader*, void*);
    void SetInvalidateCallback(void(*func)(ItemObjectScriptReader*, void*), void* callback_ptr);
    ItemObjectScriptReader();
    virtual ~ItemObjectScriptReader();
    void AttachToItemObject(ItemObject* _obj);
    ItemObject* GetAttached() const;
    void Invalidate();
    bool valid() const;
    vec3 GetPhysicsPosition();
    void SetPhysicsTransform(mat4 transform);
    mat4 GetPhysicsTransform();
    void SetPhysicsVel(const vec3 &linear_vel, const vec3 &angular_vel);
    void GetPhysicsVel(vec3 &linear_vel, vec3 &angular_vel);
    void Detach();
    void ActivatePhysics();
    void SetInterpInfo( int count, int period );
    float GetRangeExtender();
    float GetRangeMultiplier();
    ItemObject* operator->() const {
        return obj;
    }
    void AddConstraint( BulletObject* bullet_object );
    void RemoveConstraint( );
};
