//-----------------------------------------------------------------------------
//           Name: dynamiclightobject.cpp
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
#include "dynamiclightobject.h"

#include <Graphics/pxdebugdraw.h>
#include <Graphics/graphics.h>
#include <Game/hardcoded_assets.h>
#include <Game/EntityDescription.h>

#include <Objects/dynamiclightobject.h>
#include <Main/scenegraph.h>
#include <Utility/assert.h>
#include <Internal/memwrite.h>
#include <Math/vec3math.h>
#include <Main/scenegraph.h>
#include <Editors/map_editor.h>

extern bool g_debug_runtime_disable_dynamic_light_object_draw;

float RadiusFromScale(vec3 scale) {
    return std::min(fabs(scale.x()), std::min(fabs(scale.y()), fabs(scale.z())));
}

void DynamicLightObject::Draw() {
    if (g_debug_runtime_disable_dynamic_light_object_draw) {
        return;
    }

    if (!Graphics::Instance()->media_mode() && light_id_ != -1 && scenegraph_ && scenegraph_->map_editor->state_ != MapEditor::kInGame) {
        const DynamicLight *light = scenegraph_->dynamic_light_collection.GetLightFromID(light_id_);
        if (light) {
            mat4 transform;
            if (selected_) {
                transform.SetScale(vec3(RadiusFromScale(scale_)));
                transform.SetTranslationPart(translation_);
                DebugDraw::Instance()->AddWireMesh(HardcodedPaths::paths[HardcodedPaths::kPointLight], transform, vec4(light->color, 0.025f), _delete_on_draw);
            }
            transform.SetScale(vec3(0.1f));
            transform.SetTranslationPart(translation_);
            DebugDraw::Instance()->AddWireMesh(HardcodedPaths::paths[HardcodedPaths::kPointLight], transform, vec4(light->color, 0.25f), _delete_on_draw);
        }
    }
}

bool DynamicLightObject::Initialize() {
    LOG_ASSERT(light_id_ == -1);
    light_id_ = scenegraph_->dynamic_light_collection.AddLight(GetTranslation(), color * (1.0f + overbright * 0.3f), RadiusFromScale(scale_));
    return true;
}

void DynamicLightObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    if (scenegraph_ && light_id_ != -1) {
        DynamicLight *light = scenegraph_->dynamic_light_collection.GetLightFromID(light_id_);
        LOG_ASSERT(light != NULL);
        if (light) {
            light->radius = RadiusFromScale(scale_);
            light->pos = translation_;
        }
    }
}

void DynamicLightObject::Dispose() {
    Object::Dispose();
    if (scenegraph_ && light_id_ != -1) {
        bool success = scenegraph_->dynamic_light_collection.DeleteLight(light_id_);
        LOG_ASSERT(success);
        light_id_ = -1;
    }
}

DynamicLightObject::DynamicLightObject() {
    box_.dims = vec3(1.0f);
    light_id_ = -1;
    color = vec3(1.0f, 1.0f, 1.0f);
    overbright = 0.0f;
}

DynamicLightObject::~DynamicLightObject() {
    LOG_ASSERT(light_id_ == -1);
}

void DynamicLightObject::GetDesc(EntityDescription &desc) const {
    /*
    LOG_ASSERT(light_id_ != -1);
    LOG_ASSERT(scenegraph_);
    const DynamicLight *light = scenegraph_->dynamic_light_collection.GetLightFromID(light_id_);
    LOG_ASSERT(light != NULL);
    */
    Object::GetDesc(desc);
    desc.AddVec3(EDF_COLOR, color);
    desc.AddFloat(EDF_OVERBRIGHT, overbright);
}

bool DynamicLightObject::SetFromDesc(const EntityDescription &desc) {
    LOG_ASSERT(light_id_ == -1);
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto &field : desc.fields) {
            switch (field.type) {
                case EDF_COLOR: {
                    memread(color.entries, sizeof(float), 3, field.data);
                    break;
                }
                case EDF_OVERBRIGHT: {
                    memread(&overbright, sizeof(float), 1, field.data);
                    break;
                }
            }
        }
    }
    return ret;
}

vec3 DynamicLightObject::GetTint() const {
    return color;
    /*
    LOG_ASSERT(light_id_ != -1);
    DynamicLight *light = scenegraph_->dynamic_light_collection.GetLightFromID(light_id_);
    LOG_ASSERT(light != NULL);
    return light->color;
    */
}

float DynamicLightObject::GetOverbright() const {
    return overbright;
}

void DynamicLightObject::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    DynamicLight *light;
    switch (type) {
        case OBJECT_MSG::SET_COLOR:
            if (light_id_ != -1) {
                color = *va_arg(args, vec3 *);
                light = scenegraph_->dynamic_light_collection.GetLightFromID(light_id_);
                if (light) {
                    light->color = color * (1.0f + overbright * 0.3f);
                }
            }
            break;
        case OBJECT_MSG::SET_OVERBRIGHT:
            if (light_id_ != -1) {
                overbright = *va_arg(args, float *);
                light = scenegraph_->dynamic_light_collection.GetLightFromID(light_id_);
                if (light) {
                    light->color = color * (1.0f + overbright * 0.3f);
                }
            }
            break;
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void DynamicLightObject::SetEnabled(bool val) {
    if (!enabled_ && val) {
        light_id_ = scenegraph_->dynamic_light_collection.AddLight(GetTranslation(), color * (1.0f + overbright * 0.3f), RadiusFromScale(scale_));
    } else if (enabled_ && !val) {
        scenegraph_->dynamic_light_collection.DeleteLight(light_id_);
        light_id_ = -1;
    }
    Object::SetEnabled(val);
}
