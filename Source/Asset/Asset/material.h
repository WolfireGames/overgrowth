//-----------------------------------------------------------------------------
//           Name: material.h
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

#include <Asset/assetinfobase.h>
#include <Math/vec3.h>

#include <map>

struct MaterialEvent {
    float max_distance;
    std::string soundgroup;
    bool attached;
};

struct MaterialDecal {
    std::string color_path;
    std::string normal_path;
    std::string shader;
};

struct MaterialParticle {
    std::string particle_path;
};

class AssetManager;

class Material : public AssetInfo {
    std::map<std::string, std::map<std::string, MaterialEvent> > event_map;
    std::map<std::string, MaterialDecal> decal_map;
    std::map<std::string, MaterialParticle> particle_map;

    AssetRef<Material> base_ref;

    float hardness;
    float friction;
    float sharp_penetration;
    // ASContext *context;
   public:
    Material(AssetManager *owner, uint32_t asset_id);
    ~Material() override;

    int Load(const std::string &path, uint32_t load_flags);
    const char *GetLoadErrorString();
    const char *GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void ReportLoad() override;
    void HandleEvent(const std::string &the_event, const vec3 &pos);
    const MaterialEvent &GetEvent(const std::string &the_event);
    const MaterialEvent &GetEvent(const std::string &the_event, const std::string &mod);
    const MaterialDecal &GetDecal(const std::string &type);
    const MaterialParticle &GetParticle(const std::string &type);
    float GetHardness() const;
    float GetFriction() const;
    float GetSharpPenetration() const;
    void ReturnPaths(PathSet &path_set) override;

    static AssetType GetType() { return MATERIAL_ASSET; }
    static const char *GetTypeName() { return "MATERIAL_ASSET"; }
    static bool AssetWarning() { return true; }

    AssetLoaderBase *NewLoader() override;
};

typedef AssetRef<Material> MaterialRef;
