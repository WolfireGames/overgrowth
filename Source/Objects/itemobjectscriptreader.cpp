//-----------------------------------------------------------------------------
//           Name: itemobjectscriptreader.cpp
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
#include "itemobjectscriptreader.h"

#include <Objects/itemobject.h>
#include <Scripting/angelscript/ascontext.h>

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

void ItemObjectScriptReader::Invalidate() {
    if (invalidate_callback) {
        invalidate_callback(this, callback_ptr_);
    }
    // obj = NULL;
}

void ItemObjectScriptReader::Detach() {
    if (obj) {
        obj->RemoveReader(this);
        if (constraint) {
            obj->RemoveConstraint(&constraint);
        }
    }
    obj = NULL;
}

void ItemObjectScriptReader::AttachToItemObject(ItemObject* _obj) {
    Detach();
    obj = _obj;
    obj->AddReader(this);
}

ItemObjectScriptReader::~ItemObjectScriptReader() {
    Detach();
}

bool ItemObjectScriptReader::valid() const {
    return obj != NULL;
}

ItemObjectScriptReader::ItemObjectScriptReader() : obj(NULL),
                                                   constraint(NULL),
                                                   just_created(true),
                                                   holding(false),
                                                   stuck(false),
                                                   char_id(-1),
                                                   attachment_type(_at_unspecified) {}

vec3 ItemObjectScriptReader::GetPhysicsPosition() {
    if (obj) {
        return obj->GetPhysicsPosition();
    }
    return vec3(0.0f);
}

void ItemObjectScriptReader::SetPhysicsTransform(mat4 transform) {
    if (obj) {
        obj->SetPhysicsTransform(transform);
    }
}

void ItemObjectScriptReader::ActivatePhysics() {
    if (obj) {
        obj->ActivatePhysics();
    }
}

void ItemObjectScriptReader::SetInterpInfo(int count, int period) {
    if (obj) {
        obj->SetInterpolation(period - count, period);
    }
}

float ItemObjectScriptReader::GetRangeExtender() {
    if (obj) {
        return obj->item_ref()->GetRangeExtender();
    } else {
        return 0.0f;
    }
}

ItemObject* ItemObjectScriptReader::GetAttached() const {
    return obj;
}

void ItemObjectScriptReader::SetPhysicsVel(const vec3& linear_vel, const vec3& angular_vel) {
    if (obj) {
        return obj->SetVelocities(linear_vel, angular_vel);
    }
}

mat4 ItemObjectScriptReader::GetPhysicsTransform() {
    return obj->GetPhysicsTransform();
}

void ItemObjectScriptReader::GetPhysicsVel(vec3& linear_vel, vec3& angular_vel) {
    return obj->GetPhysicsVel(linear_vel, angular_vel);
}

void ItemObjectScriptReader::SetInvalidateCallback(void (*func)(ItemObjectScriptReader*, void*), void* callback_ptr) {
    invalidate_callback = func;
    callback_ptr_ = callback_ptr;
}

float ItemObjectScriptReader::GetRangeMultiplier() {
    return obj->item_ref()->GetRangeMultiplier();
}

void ItemObjectScriptReader::AddConstraint(BulletObject* bullet_object) {
    RemoveConstraint();
    constraint = obj->AddConstraint(bullet_object);
}

void ItemObjectScriptReader::RemoveConstraint() {
    if (constraint) {
        obj->RemoveConstraint(&constraint);
    }
}
