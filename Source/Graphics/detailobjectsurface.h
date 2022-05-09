//-----------------------------------------------------------------------------
//           Name: detailobjectsurface.h
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
#include <Math/vec4.h>

#include <Asset/Asset/objectfile.h>
#include <Asset/Asset/image_sampler.h>

#include <Utility/flat_hash_map.hpp>
#include <Graphics/drawbatch.h>

#include <vector>
#include <set>
#include <map>

class Model;
class LightProbeCollection;
class SceneGraph;

// One instance of a detail object
struct DetailInstance {
    // mat4 transform;    // Transformation matrix for this detail object - note: now stored separately for better bulk memory transfer patterns
    vec2 tex_coord0;     // Base texture coordinates for color/light matching - note: reduced to one coord because the other was not being used in drawing
    float embed;         // How embedded it is in the ground
    float height_scale;  // How much the height has been scaled
};

struct BatchVertex {
    GLfloat px, py, pz;  // position (0-12 bytes)
    GLfloat nx, ny, nz;  // normal (12-24 bytes)
    GLfloat tx, ty, tz;  // tangent (24-36 bytes)
    GLfloat bx, by, bz;  // bitangent (36-48 bytes)
    GLfloat tu, tv;      // texcoords (48-56 bytes)
    float padding[2];    // (to reach 64 bytes)
};

// A patch containing many detail object instances
struct DOPatch {
    bool calculated;
    std::vector<int> tris;
    std::vector<vec3> verts;
    std::vector<vec3> normals;
    std::vector<vec2> tex_coords;
    std::vector<vec2> tex_coords2;
    std::vector<DetailInstance> detail_instances;
    std::vector<mat4> detail_instance_transforms;
    std::vector<float> detail_instance_origins_x;
    std::vector<float> detail_instance_origins_y;
    std::vector<float> detail_instance_origins_z;
    vec3 sphere_center;
    float sphere_radius;
    DOPatch();
};

// A comparable array of three ints
struct TriInt {
    int val[3];
    TriInt(int a = 0, int b = 0, int c = 0);
    bool operator<(const TriInt &other) const;
    bool operator==(const TriInt &other) const;
};

namespace std {
template <>
struct hash<TriInt> {
    size_t operator()(const TriInt &k) const {
        // Compute individual hash values for first, second and third
        // http://stackoverflow.com/a/1646913/126995
        size_t res = 17;
        res = res * 31 + hash<float>()((const float)k.val[0]);
        res = res * 31 + hash<float>()((const float)k.val[1]);
        res = res * 31 + hash<float>()((const float)k.val[2]);
        return res;
    }
};
}  // namespace std

struct CalculatedPatch {
    TriInt coords;
    mutable float last_used_time;
    bool operator<(const CalculatedPatch &other) const;
    bool operator==(const CalculatedPatch &other) const;
};

namespace std {
template <>
struct hash<CalculatedPatch> {
    size_t operator()(const CalculatedPatch &k) const {
        // Compute individual hash values for first, second and third
        // http://stackoverflow.com/a/1646913/126995
        size_t res = 17;
        res = res * 31 + hash<TriInt>()(k.coords);
        res = res * 31 + hash<float>()(k.last_used_time);
        return res;
    }
};
}  // namespace std

const unsigned batch_size = 40;
// A model that contains "batch_size" copies of a model to reduce draw calls
struct BatchModel {
    bool created;
    float height;                 // The height is important for controlling how embedded it is in the base surface
    VBOContainer vboc;            // The vbo containing the batch vertex, normal, and uv information
    VBOContainer vboc_el;         // The vbo containing the batch triangle indices
    unsigned instance_num_faces;  // The number of faces in each instance, so we can control how many instances we draw
    void Create(const Model &model);
    void Draw(const std::vector<DetailInstance> &di_vec, const std::vector<mat4> &di_vec_transforms);
    BatchModel();
    void StartDraw();
    void StopDraw();
};

class DetailObjectSurface {
    std::vector<DetailInstance> draw_detail_instances;
    std::vector<mat4> draw_detail_instance_transforms;
    BatchModel bm;
    ObjectFileRef ofr;
    const Model *base_model_ptr;
    int detail_model_id;
    std::vector<float> triangle_area;
    typedef ska::flat_hash_map<TriInt, DOPatch> PatchesMap;
    PatchesMap patches;
    int bounding_box[6];
    DrawBatch batch;
    TextureAssetRef base_color_ref;
    TextureAssetRef base_normal_ref;
    ImageSamplerRef weight_map;
    typedef ska::flat_hash_set<CalculatedPatch> CalculatedPatchesSet;
    CalculatedPatchesSet calculated_patches;
    float density;
    float normal_conform;
    float min_embed;
    float max_embed;
    float min_scale;
    float max_scale;
    float view_dist;
    float overbright;
    float jitter_degrees;
    CollisionType collision_type;
    bool double_sided;

   public:
    float tint_weight;

    enum DetailObjectShaderType { TERRAIN,
                                  ENVOBJECT };
    void AttachTo(const Model &model, mat4 transform);
    void SetBaseTextures(const TextureAssetRef &color_ref, const TextureAssetRef &normal_ref);
    void LoadDetailModel(const std::string &path);
    void PreDrawCamera(const mat4 &transform);
    // void GetTrisInSphere( const vec3 &center, float radius );
    void Cluster();
    void Draw(const mat4 &transform, DetailObjectShaderType shader_type, vec3 color_tint, const TextureRef &light_cube, LightProbeCollection *light_probe_collection, SceneGraph *scenegraph);
    TriInt GetTriInt(const vec3 &pos);
    void GetTrisInPatches(mat4 transform);
    void CalcPatchInstances(const TriInt &coords, DOPatch &patch, mat4 transform);
    void ClearPatch(DOPatch &patch);
    void SetDensity(float _density);
    void SetColorTint(const vec3 &tint);
    void SetNormalConform(float _normal_conform);
    void SetMinEmbed(float _min_embed);
    void SetMaxEmbed(float _max_embed);
    void SetMinScale(float _min_scale);
    void SetMaxScale(float _max_scale);
    void LoadWeightMap(const std::string &weight_path);
    void SetViewDist(float _view_dist);
    void SetJitterDegrees(float _jitter_degrees);
    void SetOverbright(float _overbright);
    void SetCollisionType(CollisionType _collision);
};
