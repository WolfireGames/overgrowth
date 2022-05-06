//-----------------------------------------------------------------------------
//           Name: particletype.h
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
#pragma once

#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>
#include <Asset/Asset/animationeffect.h>

#include <Graphics/textureref.h>

#include <map>
#include <vector>

class ParticleType : public Asset {
public:
    ParticleType( AssetManager* owner, uint32_t asset_id );

    static AssetType GetType() { return PARTICLE_TYPE_ASSET; }
    static const char* GetTypeName() { return "PARTICLE_TYPE_ASSET"; }
    static bool AssetWarning() { return true; }

    static const int kMaxNameLen = 512;
    char shader_name[kMaxNameLen];
    TextureAssetRef color_map;
    TextureAssetRef normal_map;
    AnimationEffectRef ae_ref;
    float size_range[2];
    float rotation_range[2];
    vec4 color_range[2];
    float inertia;
    float gravity;
    float wind;
    float size_decay_rate;
    float opacity_decay_rate;
    float opacity_ramp_time;
    bool quadratic_expansion;
    float qe_speed;
    bool quadratic_dispersion;
    float qd_mult;
    bool velocity_axis;
    bool speed_stretch;
    float speed_mult;
    bool min_squash;
    bool no_rotation;
    bool collision;
    bool collision_destroy;
    bool character_collide;
    bool character_add_blood;
    std::string collision_decal;
    float collision_decal_size_mult;
    std::string collision_event;

    int sub_error;
    int Load(const std::string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void ReportLoad() override;
    void Reload( );

    void clear();

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<ParticleType> ParticleTypeRef;
