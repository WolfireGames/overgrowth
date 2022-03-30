//-----------------------------------------------------------------------------
//           Name: flares.h
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

#include <Graphics/glstate.h>
#include <Graphics/textureref.h>
#include <Graphics/vbocontainer.h>

#include <Math/vec3.h>

#include <Asset/Asset/texture.h>

#include <opengl.h>

#include <vector>
#include <map>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

const float _sharp_glare_threshold = 1.5f;
const float _soft_glare_threshold = 2.5f;
const float _no_glare_threshold = 4.0f;

struct OccQuery{
    bool started;
    bool created;
    GLuint id;

    OccQuery();
};

struct Flare {
    std::vector<OccQuery> query;
    vec3 position;
    vec3 old_position;
    float brightness;
    float diffuse;
    bool distant;
    float visible;
    float slow_visible;
    vec3 color;
    float tex_opac[3];
    float size_mult;
    float visible_mult;
};

class SceneGraph;

class Flares{
public:
    Flares();
    ~Flares();

    enum FlareType {
        kDiffuse,
        kSharp
    };

    SceneGraph *scenegraph;
        
    Flare* MakeFlare(vec3 _position, float _brightness, bool _distant);
    void CleanupFlares();
    void Draw(FlareType flare_type);
    void Update(float timestep);
    void DeleteFlare(Flare* flare);
    std::vector<Flare*> flares;

    void GetShaderNames(std::map<std::string, int>& preload_shaders);

private:
    VBOContainer vert_vbo;
    VBOContainer index_vbo;
    float animation;
    float old_angle;
    float new_angle;
    GLState gl_state;
    int shader_id;
    const char* shader;
    TextureAssetRef flare_texture_matte;
    TextureAssetRef flare_texture_streaks;
    TextureAssetRef flare_texture_blur1;
    TextureAssetRef flare_texture_blur2;
    TextureAssetRef flare_texture_color;
    void DrawFlare(int which);
    void OcclusionQuery(int which);
    void DrawOcclusion(int which);
public:


};

