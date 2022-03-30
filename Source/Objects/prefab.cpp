//-----------------------------------------------------------------------------
//           Name: prefab.cpp
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
#include "prefab.h"

#include <Internal/memwrite.h>

Prefab::Prefab() :
    Group(),
    prefab_locked(true),
    original_scale_(0.0f)
{}

bool Prefab::Initialize() {
    return true;
}

void Prefab::GetDisplayName(char* buf, int buf_size) {
    if( GetName().empty() ) {
        FormatString(buf, buf_size, "%d, Prefab with %d children", GetID(), children.size());
    } else { 
        FormatString(buf, buf_size, "%s, Prefab with %d children", GetName().c_str(), children.size());
    }
}

bool Prefab::SetFromDesc( const EntityDescription &desc ) {
	bool ret = Group::SetFromDesc(desc);
    if( ret ) {
        const EntityDescriptionField* l_prefab_locked = desc.GetField(EDF_PREFAB_LOCKED);
        if(l_prefab_locked){
            l_prefab_locked->ReadBool(&prefab_locked);
        }

        const EntityDescriptionField* l_prefab_path = desc.GetField(EDF_PREFAB_PATH);
        if(l_prefab_path){
            l_prefab_path->ReadPath(&prefab_path);
        }

        const EntityDescriptionField* l_original_scale = desc.GetField(EDF_ORIGINAL_SCALE);
        if(l_original_scale){
            memread(original_scale_.entries, sizeof(float), 3, l_original_scale->data);
        }

        InitRelMats();
    }
    return ret;
}

void Prefab::GetDesc(EntityDescription &desc) const {
    Group::GetDesc(desc);
    desc.AddBool(EDF_PREFAB_LOCKED, prefab_locked);
    desc.AddPath(EDF_PREFAB_PATH, prefab_path);
    desc.AddVec3(EDF_ORIGINAL_SCALE, original_scale_);
}

void Prefab::InfrequentUpdate() {
    Group::InfrequentUpdate();
    if( prefab_locked ) {
        box_color = vec4(22/255.0f, 183/255.0f, 204/255.0f, 1.0f);
    } else {
        box_color = vec4(53/255.0f, 253/255.0f, 104/255.0f, 1.0f);
    }
}

void Prefab::InitShape() {
    Group::InitShape();
    original_scale_ = scale_;
}

bool Prefab::LockedChildren() {
    return prefab_locked || Group::LockedChildren();
}

ObjectSanityState Prefab::GetSanity() {
    ObjectSanityState sanity = Group::GetSanity();
    sanity.type = _prefab;
    return sanity;
}
