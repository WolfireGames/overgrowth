//-----------------------------------------------------------------------------
//           Name: terrain.h
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

#include <Graphics/model.h>
#include <Graphics/textures.h>
#include <Graphics/heightmap.h>
#include <Graphics/textureref.h>
#include <Graphics/detailobjectsurface.h>

#include <Math/vec2.h>
#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Asset/Asset/material.h>
#include <Asset/Asset/averagecolorasset.h>

#include <Compat/filepath.h>
#include <Internal/levelxml.h>

class Sky;
class LoadingText;
class SceneGraph;

class Terrain {
   private:
    HeightmapImage heightmap_;

    const char* shader;

    bool minimal;
    int model_id;

    void CalculateHighResVertices(Model& terrain_high_detail_model);
    void CalculateHighResFaces(Model& terrain_high_detail_model);

    void CalcWorkingPoints(vec3 cam_pos);

    void CalculateSimplifiedTerrain();
    void CalculateMinimalTerrain();
    bool LoadCachedSimplifiedTerrain();

   public:
    std::list<Model> terrain_patches;
    std::list<Model> edge_terrain_patches;
    std::list<DetailObjectSurface*> detail_object_surfaces;
    std::set<AverageColorRef> average_colors;

    void GLInit(Sky* sky);
    TextureAssetRef normal_map_ref;
    TextureAssetRef color_texture_ref;
    TextureAssetRef weight_perturb_ref;

    TextureRef baked_texture_ref;
    TextureAssetRef baked_texture_asset;

    TextureRef detail_texture_ref;         // array texture
    TextureRef detail_normal_texture_ref;  // array texture
    std::vector<vec4> detail_texture_color;
    std::vector<vec4> detail_texture_color_srgb;
    TextureAssetRef detail_texture_weights;
    TextureAssetRef detail_texture_tint;

    ImageSamplerRef weight_bitmap;
    ImageSamplerRef color_bitmap;

    std::string color_path;

    std::vector<DetailMapInfo> detail_maps_info;
    std::vector<DetailObjectLayer> detail_object_layers;

    std::string level_name;
    std::string weight_map_path;

    int terrain_texture_size;
    GLuint framebuffer;

    void BakeTerrainTexture(GLuint framebuffer, const TextureRef& light_cube);
    Model& GetModel() const;
    void drawLayer(int which);
    void Load(const char* name, const std::string& model_override = NULL);
    void LoadMinimal(const char* name, const std::string& model_override = NULL);
    void GetShaderNames(std::map<std::string, int>& preload_shaders);
    int lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal = 0);
    ~Terrain();
    Terrain();

    const MaterialRef GetMaterialAtPoint(vec3 point, int* tri = NULL);
    void CalcDetailTextures();
    void CalculatePatches();
    void Dispose();
    void HandleMaterialEvent(const std::string& the_event, const vec3& event_pos, int* tri);
    vec4 SampleWeightMapAtPoint(vec3 point, int* tri = NULL);
    vec2 GetUVAtPoint(const vec3& point, int* tri = NULL) const;
    vec3 SampleColorMapAtPoint(vec3 point, int* tri);
    void SetDetailObjectLayers(const std::vector<DetailObjectLayer>& detail_object_layers);
    void SetDetailTextures(const std::vector<DetailMapInfo>& detail_maps_info);

    void SetColorTexture(const char* path);
    void SetWeightTexture(const char* path);
};
