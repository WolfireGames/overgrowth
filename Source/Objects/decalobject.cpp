//-----------------------------------------------------------------------------
//           Name: decalobject.cpp
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
#include "decalobject.h"

#include <Objects/envobject.h>
#include <Objects/group.h>

#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>

#include <Internal/stopwatch.h>
#include <Internal/memwrite.h>
#include <Internal/profiler.h>
#include <Internal/common.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Editors/map_editor.h>
#include <UserInput/input.h>
#include <Logging/logdata.h>
#include <Internal/timer.h>

#include <tinyxml.h>

extern Timer game_timer;

std::map<std::string, RC_DecalTexture> DecalObject::textureCache;

extern bool g_debug_runtime_disable_decal_object_draw;
extern bool g_debug_runtime_disable_decal_object_pre_draw_frame;

EntityType DecalObject::GetType() const {
    return decal_file_ref->is_shadow ? _shadow_decal_object : _decal_object;
}

DecalObject::DecalObject() {
    collidable = false;
    box_.dims = vec3(1.0f);
    spawn_time_ = game_timer.game_time;
}

bool DecalObject::Initialize() {
    return true;
}

void DecalObject::Dispose() {
    texture.Set(NULL);
    const std::map<std::string, RC_DecalTexture>::iterator& texit = textureCache.find(obj_file);
    if (texit != textureCache.end()) {
        if (texit->second.GetReferenceCount() == 1) {  // Owned only by texture cache
            textureCache.erase(texit);
        }
    }
}

void DecalObject::GetDisplayName(char* buf, int buf_size) {
    if (GetName().empty()) {
        FormatString(buf, buf_size, "%d, Decal: %s", GetID(), obj_file.c_str());
    } else {
        FormatString(buf, buf_size, "%s, Decal: %s", GetName().c_str(), obj_file.c_str());
    }
}

void DecalObject::Draw() {
    if (g_debug_runtime_disable_decal_object_draw) {
        return;
    }

    // Add fading line in the direction of the decal projector
    DebugDraw::Instance()->AddLine(
        GetTransform() * vec3(0.0f, 0.5f, 0.0f),
        GetTransform() * vec3(0.0f, -0.5f, 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f),
        vec4(0.0f, 0.0f, 0.0f, 0.0f),
        _delete_on_draw);
    // Add cross on the side from which the decal is projected
    DebugDraw::Instance()->AddLine(
        GetTransform() * vec3(-0.5f, 0.5f, -0.5f),
        GetTransform() * vec3(0.5f, 0.5f, 0.5f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f),
        _delete_on_draw);
    DebugDraw::Instance()->AddLine(
        GetTransform() * vec3(-0.5f, 0.5f, 0.5f),
        GetTransform() * vec3(0.5f, 0.5f, -0.5f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f),
        _delete_on_draw);
}

void DecalObject::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    switch (type) {
        case OBJECT_MSG::SET_COLOR:
        case OBJECT_MSG::SET_OVERBRIGHT: {
            // vec3 old_color = color_tint_component_.temp_tint();
            color_tint_component_.ReceiveObjectMessageVAList(type, args);
            break;
        }
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void DecalObject::PreDrawFrame(float curr_game_time) {
    if (g_debug_runtime_disable_decal_object_pre_draw_frame) {
        return;
    }

    if (decal_file_ref->special_type == 6 && (curr_game_time - spawn_time_ > 2.0f)) {  // Water drops fade after 2 seconds
        scenegraph_->map_editor->RemoveObject(this, scenegraph_, true);
    }
}

void DecalObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, obj_file);
    color_tint_component_.AddToDescription(desc);
}

bool DecalObject::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_FILE_PATH: {
                    std::string type_file;
                    field.ReadString(&type_file);
                    if (type_file != obj_file) {
                        Load(type_file);
                    }
                    break;
                }
            }
        }
        color_tint_component_.SetFromDescription(desc);
    }
    return ret;
}

void DecalObject::Load(const std::string& type_file) {
    obj_file = type_file;

    // decal_file_ref = DecalFiles::Instance()->ReturnRef(type_file);
    decal_file_ref = Engine::Instance()->GetAssetManager()->LoadSync<DecalFile>(type_file);

    std::map<std::string, RC_DecalTexture>::iterator texit = textureCache.find(type_file);

    if (texit == textureCache.end()) {
        texture = DecalTextures::Instance()->allocateTexture(decal_file_ref->color_map, decal_file_ref->normal_map);

        textureCache[type_file] = texture;
    } else {
        LOGS << "Loading decal from texture cache rather than reallocating space for it" << std::endl;
        texture = textureCache[type_file];
    }
}
