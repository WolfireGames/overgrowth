//-----------------------------------------------------------------------------
//           Name: lightvolume.cpp
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
#include "lightvolume.h"

#include <Graphics/camera.h>
#include <Graphics/shaders.h>
#include <Graphics/graphics.h>
#include <Graphics/sky.h>

#include <Game/EntityDescription.h>
#include <Main/scenegraph.h>
#include <Editors/map_editor.h>
#include <Utility/assert.h>

bool LightVolumeObject::Initialize() {
    obj_file = "light_volume_object";
    return true;
}

void LightVolumeObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    dirty = true;
}

void LightVolumeObject::Dispose() {
    Object::Dispose();
}

LightVolumeObject::LightVolumeObject() : dirty(true) {
    box_.dims = vec3(2.0f);
}

void LightVolumeObject::Draw() {
}

LightVolumeObject::~LightVolumeObject() {
}

void LightVolumeObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
}

bool LightVolumeObject::SetFromDesc(const EntityDescription& desc) {
    return Object::SetFromDesc(desc);
}
