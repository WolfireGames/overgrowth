//-----------------------------------------------------------------------------
//           Name: navmeshregionobject.cpp
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
#include "navmeshregionobject.h"

#include <Graphics/textures.h>
#include <Graphics/Billboard.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/shaders.h>
#include <Graphics/geometry.h>

#include <Editors/actors_editor.h>
#include <Editors/map_editor.h>

#include <Game/EntityDescription.h>
#include <Internal/memwrite.h>
#include <Main/scenegraph.h>
#include <Objects/envobject.h>

NavmeshRegionObject::NavmeshRegionObject() {
    permission_flags = CAN_TRANSLATE | CAN_SCALE | CAN_SELECT | CAN_COPY | CAN_DELETE;
    box_.dims = vec3(1.0f);
    box_color = vec4(0.1f, 0.1f, 1.0f, 1.0f);
}

NavmeshRegionObject::~NavmeshRegionObject() {
}

void NavmeshRegionObject::Draw() {
    // Check if we should be drawn.
    if (scenegraph_->map_editor->state_ == MapEditor::kInGame || Graphics::Instance()->media_mode() || ActiveCameras::Instance()->Get()->GetFlags() == Camera::kPreviewCamera) {
        return;
    }
}

void NavmeshRegionObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
}

bool NavmeshRegionObject::SetFromDesc(const EntityDescription& desc) {
    return Object::SetFromDesc(desc);
}

bool NavmeshRegionObject::Initialize() {
    return true;
}

vec3 NavmeshRegionObject::GetMinBounds() {
    return (GetTransform() * vec4(-0.5, -0.5, -0.5, 1.0f)).xyz();
}

vec3 NavmeshRegionObject::GetMaxBounds() {
    return (GetTransform() * vec4(0.5, 0.5, 0.5, 1.0f)).xyz();
}
