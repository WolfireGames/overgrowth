//-----------------------------------------------------------------------------
//           Name: navmeshhintobject.cpp
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
#include "navmeshhintobject.h"

#include <Graphics/textures.h>
#include <Graphics/Billboard.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/shaders.h>
#include <Graphics/camera.h>
#include <Graphics/geometry.h>

#include <Editors/actors_editor.h>
#include <Editors/map_editor.h>

#include <Game/EntityDescription.h>
#include <Internal/memwrite.h>
#include <Main/scenegraph.h>
#include <Objects/envobject.h>
#include <Logging/logdata.h>

extern bool g_debug_runtime_disable_navmesh_hint_object_draw;

NavmeshHintObject::NavmeshHintObject() {
    box_.dims = vec3(1.0f);
    box_color = vec4(0.4f, 0.4f, 0.9f, 1.0f);

    vec3 corners[] =
        {
            // Upper four corners
            vec3(0.5f, 0.5f, 0.5f),
            vec3(0.5f, 0.5f, -0.5f),
            vec3(-0.5f, 0.5f, -0.5f),
            vec3(-0.5f, 0.5f, 0.5f),

            // Lower four corners
            vec3(0.5f, -0.5f, 0.5f),
            vec3(0.5f, -0.5f, -0.5f),
            vec3(-0.5f, -0.5f, -0.5f),
            vec3(-0.5f, -0.5f, 0.5f),
        };

    for (int i = 0; i < 8; i++) {
        for (int k = i; k < 8; k++) {
            if (i != k) {
                if (
                    corners[i][0] == corners[k][0] ||
                    corners[i][1] == corners[k][1] ||
                    corners[i][2] == corners[k][2]) {
                    cross_marking.push_back(corners[i]);
                    cross_marking.push_back(corners[k]);
                }
            }
        }
    }
}

NavmeshHintObject::~NavmeshHintObject() {
}

void NavmeshHintObject::Draw() {
    if (g_debug_runtime_disable_navmesh_hint_object_draw) {
        return;
    }

    // Check if we should be drawn.
    if (scenegraph_->map_editor->state_ == MapEditor::kInGame || Graphics::Instance()->media_mode() || ActiveCameras::Instance()->Get()->GetFlags() == Camera::kPreviewCamera) {
    } else if (this->editor_visible) {
        if (!unit_box_vbo->valid()) {
            unit_box_vbo->Fill(
                kVBOStatic | kVBOFloat,
                cross_marking.size() * sizeof(vec3),
                &cross_marking[0]);
        }
        vec4 draw_color(box_color);
        draw_color[3] *= selected_ ? 1.0f : 0.2f;
        DebugDraw::Instance()->AddLineObject(unit_box_vbo, GetTransform(), draw_color, _delete_on_draw);
    }
}

void NavmeshHintObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
}

bool NavmeshHintObject::SetFromDesc(const EntityDescription& desc) {
    return Object::SetFromDesc(desc);
}

bool NavmeshHintObject::Initialize() {
    return true;
}
