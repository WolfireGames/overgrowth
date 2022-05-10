//-----------------------------------------------------------------------------
//           Name: bloodsurface.h
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

#include <Math/vec2.h>
#include <Asset/Asset/texture.h>
#include <Graphics/modelsurfacewalker.h>

#include <opengl.h>
#include <list>
#include <map>

class Object;
class Model;
class SceneGraph;

static bool blood_inited = false;
static bool blood_uniforms_inited = false;

class BloodSurface {
   public:
    class TransformedVertexGetter {  // Used to get transformed geometry from items or characters
       public:
        virtual vec3 GetTransformedVertex(int val) = 0;
    };

    bool blood_tex_mipmap_dirty;
    TextureRef blood_tex;
    TextureRef blood_work_tex;
    float sleep_time;
    BloodSurface(TransformedVertexGetter* transformed_vertex_getter);
    ~BloodSurface();
    void Dispose();
    void PreDrawFrame(int width, int height);
    void AttachToModel(Model* attach_model);
    void Update(SceneGraph* scenegraph, float timestep, bool force_update = false);
    void CreateBloodDripNearestPointTransformed(const vec3& bone_pos, float blood_amount, bool can_drip);
    void CreateBloodDripNearestPointDirection(const vec3& bone_pos, const vec3& direction, float blood_amount, bool can_drip);
    void CleanBlood();
    void SetFire(float fire);
    void SetWet(float wet);
    void AddDecalToTriangles(const std::vector<int>& hit_list, vec3 pos, vec3 dir, float size, const TextureAssetRef& texture_ref);
    void AddBloodLine(const vec2& start, const vec2& end);
    void CreateDripInTri(int tri, const vec3& bary_coords, float blood_amount, float delay, bool can_drip, SurfaceWalker::Type type);
    void StartDrawingToBloodTexture();
    void EndDrawingToBloodTexture();
    void Ignite();
    void Extinguish();
    void GetShaderNames(std::map<std::string, int>& shaders);

   private:
    struct CutLine {
        vec2 start;
        vec2 end;
    };

    void InitShaders();
    void InitUniforms();

    TransformedVertexGetter* transformed_vertex_getter_;
    Model* model;
    int drip_update_delay;
    int dry_delay;
    int dry_steps;
    bool framebuffer_created;
    GLuint blood_framebuffer;
    GLuint blood_work_framebuffer;
    typedef std::list<SurfaceWalker> SurfaceWalkerList;
    SurfaceWalkerList surface_walkers;
    std::vector<CutLine> cut_lines;
    ModelSurfaceWalker model_surface_walker;

    int simple_2d_id;
    int simple_2d_stab_id;
    int character_accumulate_id;
    int ignite_id;
    int extinguish_id;

    const char* simple_2d;
    const char* simple_2d_stab_texture;
    const char* character_accumulate_dry;
    const char* character_accumulate_ignite;
    const char* character_accumulate_extinguish;
};
