//-----------------------------------------------------------------------------
//           Name: objectfile.h
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

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>
#include <Asset/Asset/texture.h>

#include <Math/vec3.h>
#include <Graphics/palette.h>
#include <Game/detailobjectlayer.h>

#include <string>
#include <vector>

class AssetManager;

class ObjectFile : public AssetInfo {
   public:
    std::string model_name;
    std::string color_map;
    std::string normal_map;
    std::string translucency_map;
    std::string shader_name;
    std::string wind_map;
    std::string sharpness_map;
    std::string label;
    std::string material_path;
    std::string weight_map;
    std::string palette_map_path;
    std::string palette_label[max_palette_elements];
    std::vector<std::string> m_detail_color_maps;
    std::vector<std::string> m_detail_normal_maps;
    std::vector<std::string> m_detail_materials;
    std::vector<float> m_detail_map_scale;
    std::vector<DetailObjectLayer> m_detail_object_layers;
    std::vector<int> avg_color;
    std::vector<int> avg_color_srgb;
    bool transparent;
    bool double_sided;
    bool terrain_fixed;
    bool dynamic;
    bool no_collision;
    bool clamp_texture;
    bool bush_collision;
    float ground_offset;
    vec3 color_tint;
    ModID modsource_;

    TextureAssetRef color_map_texture;
    TextureAssetRef normal_map_texture;
    TextureAssetRef translucency_map_texture;

    ObjectFile(AssetManager* owner, uint32_t asset_id);

    int sub_error;
    int Load(const std::string& path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();

    void Reload();
    void ReportLoad() override;

    void ReturnPaths(PathSet& path_set) override;

    static AssetType GetType() { return OBJECT_FILE_ASSET; }
    static const char* GetTypeName() { return "OBJECT_FILE_ASSET"; }
    static bool AssetWarning() { return true; }

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<ObjectFile> ObjectFileRef;
