//-----------------------------------------------------------------------------
//           Name: sky.h
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
#include <Asset/Asset/texture.h>
#include <Internal/datemodified.h>

#include <map>

class SceneGraph;

class Sky{
private:
    TextureRef cube_map_texture_ref;
    TextureRef spec_cube_map_texture_ref;
    TextureRef original_spec_cube_map_texture_ref;

    const char* simple_tex_3d_flipped;
    const char* fog;
    const char* shader;

public:
    bool displaying_YCOCG_sky;
    bool lighting_changed;

    int64_t modified;
    bool cached;
    bool load_resources_queued;

    GLuint framebuffer;
    GLuint framebuffer2;
        
    TextureAssetRef sky_texture_ref;
    TextureAssetRef horizon_texture_ref;
    Model sky_dome_model;
    Model sky_land_model;
    Model sky_box_model;
    Model horizon_model;

    enum CubeMapFlags {
        DRAW_LAND = 1<<0,
        DRAW_HORIZON = 1<<1
    };
    
    void RenderCubeMap(int flags, const TextureRef* land_texture_ref);

public:
    Sky();
    void GetShaderNames(std::map<std::string, int>& shader_names);

    std::string dome_texture_name;
    std::string level_name;
    float sky_rotation;
    vec3 sky_tint; // Current displayed tint
    vec3 sky_base_tint; // Set by script parameters
    

    bool live_updated;

    void Draw(SceneGraph* scenegraph);
    void LightingChanged(bool terrain_exists);
    void Reload();
    void QueueLoadResources();
    void LoadResources();
    void Dispose();
    void BakeFirstPass(); // Just render the skydome to a cubemap
    void BakeSecondPass(const TextureRef *_land_texture_ref); // Render skydome, ground plane, and horizon band

    void ResetSpecularCubeMapTexture();
    void SetSpecularCubeMapTexture( TextureRef new_spec_cube_map_texture_ref );
    TextureRef GetSpecularCubeMapTexture();
};
