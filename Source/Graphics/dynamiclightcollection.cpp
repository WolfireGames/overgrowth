//-----------------------------------------------------------------------------
//           Name: dynamiclightcollection.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "dynamiclightcollection.hpp"

#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/sky.h>
#include <Graphics/textures.h>

#include <Physics/bulletworld.h>
#include <Internal/profiler.h>
#include <Game/hardcoded_assets.h>
#include <Math/vec3math.h>
#include <Wrappers/glm.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <cfloat>

DynamicLightCollection::DynamicLightCollection() {
    next_id = 0;
}

DynamicLightCollection::~DynamicLightCollection() {
}

int DynamicLightCollection::AddLight(const vec3& pos, const vec3& color, float radius) {
    dynamic_lights.resize(dynamic_lights.size() + 1);
    DynamicLight& light = dynamic_lights.back();
    light.pos = pos;
    light.id = next_id++;
    light.color = color;
    light.radius = radius;
    return light.id;
}

bool DynamicLightCollection::MoveLight(int id, const vec3& pos) {
    DynamicLight* light = GetLightFromID(id);
    if (light) {
        light->pos = pos;
        return true;
    } else {
        return false;
    }
}

bool DynamicLightCollection::DeleteLight(int id) {
    for (int i = 0, len = dynamic_lights.size(); i < len; ++i) {
        if (dynamic_lights[i].id == id) {
            dynamic_lights.erase(dynamic_lights.begin() + i);
            return true;
        }
    }
    return false;
}

void DynamicLightCollection::Init() {
}

void DynamicLightCollection::Dispose() {
}

DynamicLight* DynamicLightCollection::GetLightFromID(int id) {
    for (auto& dynamic_light : dynamic_lights) {
        if (dynamic_light.id == id) {
            return &dynamic_light;
        }
    }
    return NULL;
}

void DynamicLightCollection::Draw(BulletWorld& bw) {
}
