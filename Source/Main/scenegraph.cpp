//-----------------------------------------------------------------------------
//           Name: scenegraph.cpp
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
#include "scenegraph.h"

#include <Graphics/decaltextures.h>
#include <Graphics/flares.h>
#include <Graphics/graphics.h>
#include <Graphics/models.h>
#include <Graphics/sky.h>
#include <Graphics/shaders.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/particles.h>
#include <Graphics/geometry.h>

#include <Objects/envobject.h>
#include <Objects/cameraobject.h>
#include <Objects/terrainobject.h>
#include <Objects/decalobject.h>
#include <Objects/hotspot.h>
#include <Objects/group.h>
#include <Objects/itemobject.h>
#include <Objects/movementobject.h>
#include <Objects/navmeshhintobject.h>
#include <Objects/navmeshregionobject.h>
#include <Objects/navmeshconnectionobject.h>
#include <Objects/lightvolume.h>
#include <Objects/reflectioncaptureobject.h>

#include <Physics/bulletworld.h>
#include <Physics/bulletcollision.h>
#include <Physics/bulletobject.h>
#include <Physics/physics.h>

#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Internal/snprintf.h>
#include <Internal/common.h>
#include <Internal/profiler.h>
#include <Internal/config.h>

#include <Editors/map_editor.h>
#include <Editors/actors_editor.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Main/engine.h>
#include <Internal/stopwatch.h>
#include <Compat/fileio.h>
#include <AI/navmesh.h>
#include <Logging/logdata.h>
#include <UserInput/input.h>
#include <Sound/sound.h>
#include <Game/level.h>
#include <Utility/assert.h>
#include <GUI/gui.h>

#include <SDL.h>

#include <cassert>
#include <algorithm>
#include <cstring>

extern const bool kUseShadowCache;
static const bool _draw_collision_shapes = false;

extern bool g_draw_vr;
extern bool g_simple_water;
extern bool g_disable_fog;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_albedo_only;
extern bool g_no_reflection_capture;
extern bool g_no_detailmaps;
extern bool g_no_decals;
extern bool g_no_decal_elements;
extern bool g_character_decals_enabled;
extern bool g_attrib_envobj_intancing_support;
extern bool g_attrib_envobj_intancing_enabled;
extern bool g_ubo_batch_multiplier_force_1x;

const int kGlobalShaderSuffixLen = 1024;
char global_shader_suffix_storage[kGlobalShaderSuffixLen];
char* global_shader_suffix = &global_shader_suffix_storage[0];

extern bool g_debug_runtime_disable_scene_graph_draw;
extern bool g_debug_runtime_disable_scene_graph_draw_depth_map;
extern bool g_debug_runtime_disable_scene_graph_prepare_lights_and_decals;

static UniformRingBuffer uniform_ring_buffer;

using std::max;
using std::min;

// These MUST match shader EXACTLY, be careful when changing them

// This is ClusterInfo in the shader
struct ShaderClusterInfo {
    unsigned int grid_size[3];
    unsigned int num_decals;
    unsigned int num_lights;
    unsigned int light_cluster_data_offset;
    unsigned int light_data_offset;
    unsigned int cluster_width;
    float inv_proj_mat[16];
    float viewport[4];
    float z_near;
    float z_mult;
    float pad3;
    float pad4;
};

// This is in decal data texture buffer
// must be floats only and divisible by 4
struct ShaderDecal {
    float decal_scale[3];
    float decal_spawn_time;
    float decal_rotation[4];  // quaternion
    float decal_position[3];
    float decal_pad1;
    float decal_tint[4];
    float decal_uv[4];
    float decal_normal[4];
};

// This is PointLightData in the shader
// also must be floats only and divisible by 4
struct ShaderLight {
    float pos[3];
    float radius;
    float color[3];
    float padding;
};

struct DetailObjectSurfaceDrawCall {
    EnvObject* draw_owner;
    EnvObject** instance_array;
    int num_instances;
};

#define NUM_GRID_COMPONENTS 2

SceneGraph::SceneGraph()
    : particle_system(NULL),
      terrain_object_(NULL),
      num_update_objects(0),
      bullet_world_(NULL),
      abstract_bullet_world_(NULL),
      plant_bullet_world_(NULL),
      queued_level_reset_(false),
      nav_mesh_(NULL),
      haze_mult(0.0008f),
      fog_amount(1.0f),
      level_has_been_previously_saved_(false),
      cluster_size(128),
      num_z_clusters(16),
      destruction_sanity_insert_position(0),
      destruction_memory_insert_position(0),
      infreq_update_index(0),
      partial_object_loop_counter(0),
      reflection_data_loaded(false),
      hotspots_modified_(false) {
    memset(destruction_sanity, 0, destruction_sanity_size * sizeof(Object*));
    for (int& destruction_memory_id : destruction_memory_ids) {
        destruction_memory_id = -1;
    }
    memset(destruction_memory_strings, '\0', destruction_memory_size * destruction_memory_string_size);

    const unsigned int num_floats = 3 * 4 + 3 * 4;
    LOG_ASSERT_EQ(num_floats * sizeof(float), sizeof(ShaderDecal));
    decal_tbo.resize(kMaxDecals * num_floats, 0.0f);

    // this is mainly used to accumulate decal cluster info before copying to GPU
    // but also here during initialization to fill buffers with 0
    decal_cluster_buffer.resize(4096, 0);
    const char* empty = reinterpret_cast<const char*>(&decal_cluster_buffer[0]);

    decal_data_texture = Textures::Instance()->makeBufferTexture(kMaxDecals * sizeof(ShaderDecal), GL_RGBA32F);
    Textures::Instance()->SetTextureName(decal_data_texture, "Decal Data");
    // TODO: should use 3D texture instead
    decal_cluster_texture = Textures::Instance()->makeBufferTexture(decal_cluster_buffer.size() * 4, GL_R32UI, empty);
    Textures::Instance()->SetTextureName(decal_cluster_texture, "Decal Clusters");
}

SceneGraph::~SceneGraph() {
    LOG_ASSERT(bullet_world_ == NULL);
    LOG_ASSERT(abstract_bullet_world_ == NULL);
    LOG_ASSERT(plant_bullet_world_ == NULL);
    LOG_ASSERT(nav_mesh_ == NULL);
    LOG_ASSERT(particle_system == NULL);
    LOG_ASSERT(level == NULL);
    LOG_ASSERT(map_editor == NULL);
    LOG_ASSERT(sky == NULL);
    SDL_assert(bullet_world_ == NULL);
    SDL_assert(abstract_bullet_world_ == NULL);
    SDL_assert(plant_bullet_world_ == NULL);
    SDL_assert(nav_mesh_ == NULL);
    SDL_assert(particle_system == NULL);
    SDL_assert(level == NULL);
    SDL_assert(map_editor == NULL);
    SDL_assert(sky == NULL);
}

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

static void SetIdToObjectMapValue(std::vector<Object*>& object_from_id_map_, int id, Object* value) {
    if (object_from_id_map_.size() <= id) {
        object_from_id_map_.resize((id + 1) * 2);
    }
    object_from_id_map_[id] = value;
}

static Object* GetIdToObjectMapValue(std::vector<Object*>& object_from_id_map_, int id) {
    Object* result = NULL;
    if (id >= 0 && id < object_from_id_map_.size()) {
        result = object_from_id_map_[id];
    }
    return result;
}

void SceneGraph::AssignID(Object* obj) {
    int temp_id = obj->GetID();
    bool say_new_id = false;

    // The id 0 is reserved for the terrain.
    if (temp_id == 0 && obj->GetType() != _terrain_type) {
        temp_id = -1;
    }

    if (temp_id != -1) {
        Object* obj_with_id = GetObjectFromID(temp_id);
        if (temp_id < 0 || (obj_with_id != obj && obj_with_id != NULL)) {
            if (temp_id != -1) {
                LOGI << "Object has a taken id: " << *obj << " giving it a new id" << std::endl;
                LOGI << "It collided with: " << *obj_with_id << std::endl;
                say_new_id = true;
            }
            temp_id = -1;
        }
    }

    int id;
    if (temp_id != -1) {
        id = temp_id;
    } else {
        id = GetAndReserveID();
        if (say_new_id) {
            LOGI << "Gave object id " << id << std::endl;
        }
        obj->SetID(id);
    }

    SetIdToObjectMapValue(object_from_id_map_, id, obj);
}

bool SceneGraph::addObject(Object* new_object) {
    LinkObject(new_object);
    new_object->scenegraph_ = this;
    if (new_object->Initialize() == false) {
        LOGE << "Failed at initializing object on addObject in scenegraph, unlinking. Type: " << CStringFromEntityType(new_object->GetType()) << " name: " << new_object->GetName() << std::endl;
        UnlinkObject(new_object);
        return false;
    }
    new_object->GetShaderNames(preload_shaders);
    return true;
}

void SceneGraph::LinkObject(Object* new_object) {
    for (auto& i : destruction_sanity) {
        if (i == new_object) {
            i = NULL;
        }
    }

    for (int& destruction_memory_id : destruction_memory_ids) {
        if (destruction_memory_id == new_object->GetID()) {
            destruction_memory_id = 0;
        }
    }

    objects_.push_back(new_object);
    if (new_object->collidable) {
        collide_objects_.push_back(new_object);
    }
    if (new_object->GetType() != _light_probe_object || new_object->GetType() != _dynamic_light_object) {
        visible_objects_.push_back(new_object);
        if (new_object->GetType() == _env_object) {
            visible_static_meshes_.push_back((EnvObject*)new_object);
            ShadowCacheObjectLightBounds env_object_light_bounds = {0};
            visible_static_meshes_shadow_cache_bounds_.push_back(env_object_light_bounds);
            visible_static_mesh_indices_.push_back(visible_static_mesh_indices_.size());
            visible_objects_need_sort = true;
        }
    }
    switch (new_object->GetType()) {
        case _terrain_type: {
            terrain_objects_.push_back((TerrainObject*)new_object);
            ShadowCacheObjectLightBounds terrain_light_bounds = {0};
            terrain_objects_shadow_cache_bounds_.push_back(terrain_light_bounds);
            break;
        }
        case _movement_object:
            movement_objects_.push_back(new_object);
            break;
        case _item_object:
            item_objects_.push_back(new_object);
            break;
        case _decal_object:
            decal_objects_.push_back(new_object);
            break;
        case _hotspot_object:
            hotspots_.push_back((Hotspot*)(new_object));
            hotspots_modified_ = true;
            break;
        case _navmesh_hint_object:
            navmesh_hints_.push_back((NavmeshHintObject*)(new_object));
            break;
        case _navmesh_connection_object:
            navmesh_connections_.push_back((NavmeshConnectionObject*)(new_object));
            break;
        case _path_point_object:
            path_points_.push_back(new_object);
            break;
        case _light_volume_object:
            light_volume_objects_.push_back((LightVolumeObject*)new_object);
            break;
        default:
            break;
    }
    int id = new_object->GetID();
    if (id != -1) {
        Object* object_at_id = GetIdToObjectMapValue(object_from_id_map_, id);

        if (object_at_id != NULL) {
            if (new_object != object_at_id) {
                if (object_at_id != NULL && new_object != NULL) {
                    LOGD << "Overwriting already assigned object id: " << *(object_at_id)
                         << " with the different object: " << *new_object << std::endl;
                }
            } else {
                LOGD << "Rewriting already assigned object id: " << *(object_at_id) << std::endl;
            }
        }

        SetIdToObjectMapValue(object_from_id_map_, id, new_object);
    }
}

void SceneGraph::QueueLevelReset() {
    queued_level_reset_ = true;
}

Collision SceneGraph::lineCheck(const vec3& start, const vec3& end) {
    std::vector<Collision> collisions;
    LineCheckAll(start, end, &collisions);
    Collision* closest = NULL;
    float closest_distance;
    for (auto& c : collisions) {
        float dist = distance_squared(start, c.hit_where);
        if (!closest || dist < closest_distance) {
            closest = &c;
            closest_distance = dist;
        }
    }
    if (closest) {
        return *closest;
    } else {
        return Collision();
    }
}

Collision SceneGraph::lineCheckCollidable(const vec3& start, const vec3& end, Object* not_hit) {
    std::vector<Collision> collisions;
    LineCheckAll(start, end, &collisions);
    Collision* closest = NULL;
    float closest_distance;
    for (auto& c : collisions) {
        if (c.hit_what->collidable && c.hit_what != not_hit) {
            float dist = distance_squared(start, c.hit_where);
            if (!closest || dist < closest_distance) {
                closest = &c;
                closest_distance = dist;
            }
        }
    }
    if (closest) {
        return *closest;
    } else {
        return Collision();
    }
}

void SceneGraph::LineCheckAll(const vec3& start, const vec3& end, std::vector<Collision>* collisions) {
    PROFILER_ZONE(g_profiler_ctx, "SceneGraph::LineCheckAll");
    for (auto obj : objects_) {
        vec3 point, normal;
        int collision_tri = obj->lineCheck(start, end, &point, &normal);
        if (collision_tri != -1) {
            Collision c;
            c.hit = true;
            c.hit_normal = normal;
            c.hit_what = obj;
            c.hit_how = collision_tri;
            c.hit_where = point;
            collisions->push_back(c);
        }
    }
}

static std::vector<EnvObject*>* g_static_mesh_draw_sort_entries = NULL;

static bool StaticMeshDrawSortIndices(uint16_t a_index, uint16_t b_index) {
    const EnvObject* a = (*g_static_mesh_draw_sort_entries)[a_index];
    const EnvObject* b = (*g_static_mesh_draw_sort_entries)[b_index];
    if (a->transparent != b->transparent) {
        return a->transparent < b->transparent;
    } else if (a->ofr->shader_name != b->ofr->shader_name) {
        return a->ofr->shader_name < b->ofr->shader_name;
    } else if (a->ofr->path_ != b->ofr->path_) {
        return a->ofr->path_ < b->ofr->path_;
    } else {
        return a->winding_flip < b->winding_flip;
    }
}

static void DrawQuad(int shader_id) {
    Shaders* shaders = Shaders::Instance();
    Graphics* graphics = Graphics::Instance();
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    graphics->ResetVertexAttribArrays();
}

extern VBOContainer quad_vert_vbo;
extern VBOContainer quad_index_vbo;

static void UpdateShaderSuffix(SceneGraph* scenegraph, Object::DrawType object_draw_type) {
    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    FormatString(shader_str[0], kShaderStrSize, "");
    if (object_draw_type == Object::kFullDraw) {
        if (g_simple_shadows || !g_level_shadows) {
            FormatString(shader_str[1], kShaderStrSize, "%s #SIMPLE_SHADOW", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (scenegraph->light_probe_collection.probe_lighting_enabled &&
            scenegraph->light_probe_collection.light_probe_buffer_object_id != -1 &&
            !scenegraph->light_probe_collection.light_probes.empty()) {
            FormatString(shader_str[1], kShaderStrSize, "%s #CAN_USE_LIGHT_PROBES", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_albedo_only) {
            FormatString(shader_str[1], kShaderStrSize, "%s #ALBEDO_ONLY", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_simple_water) {
            FormatString(shader_str[1], kShaderStrSize, "%s #SIMPLE_WATER", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_disable_fog) {
            FormatString(shader_str[1], kShaderStrSize, "%s #DISABLE_FOG", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_no_reflection_capture) {
            FormatString(shader_str[1], kShaderStrSize, "%s #NO_REFLECTION_CAPTURE", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_no_detailmaps) {
            FormatString(shader_str[1], kShaderStrSize, "%s #NO_DETAILMAPS", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_no_decals) {
            FormatString(shader_str[1], kShaderStrSize, "%s #NO_DECALS", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (config["ssao"].toNumber<bool>()) {
            FormatString(shader_str[1], kShaderStrSize, "%s #SSAO_TEST", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (config["volume_shadows"].toNumber<bool>()) {
            FormatString(shader_str[1], kShaderStrSize, "%s #VOLUME_SHADOWS", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
    }
    if (object_draw_type == Object::kFullDraw || object_draw_type == Object::kDrawDepthOnly || object_draw_type == Object::kDrawDepthNoAA || Object::kDrawAllShadowCascades) {
        static int ubo_batch_size_multiplier = 1;
        static GLint max_ubo_size = -1;
        if (max_ubo_size == -1) {
            glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_ubo_size);

            if (max_ubo_size >= 131072) {
                ubo_batch_size_multiplier = 8;
            } else if (max_ubo_size >= 65536) {
                ubo_batch_size_multiplier = 4;
            } else if (max_ubo_size >= 32768) {
                ubo_batch_size_multiplier = 2;
            }
        }

        if (!g_ubo_batch_multiplier_force_1x && ubo_batch_size_multiplier >= 8) {
            FormatString(shader_str[1], kShaderStrSize, "%s #UBO_BATCH_SIZE_8X", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        } else if (!g_ubo_batch_multiplier_force_1x && ubo_batch_size_multiplier >= 4) {
            FormatString(shader_str[1], kShaderStrSize, "%s #UBO_BATCH_SIZE_4X", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        } else if (!g_ubo_batch_multiplier_force_1x && ubo_batch_size_multiplier >= 2) {
            FormatString(shader_str[1], kShaderStrSize, "%s #UBO_BATCH_SIZE_2X", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }

        if (g_attrib_envobj_intancing_support && g_attrib_envobj_intancing_enabled) {
            FormatString(shader_str[1], kShaderStrSize, "%s #ATTRIB_ENVOBJ_INSTANCING", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
    }
    if (Graphics::Instance()->use_sample_alpha_to_coverage && object_draw_type != Object::kDrawDepthNoAA && object_draw_type != Object::kDrawAllShadowCascades) {
        FormatString(shader_str[1], kShaderStrSize, "%s #ALPHA_TO_COVERAGE", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (object_draw_type != Object::kFullDraw) {
        FormatString(shader_str[1], kShaderStrSize, "%s #DEPTH_ONLY #NO_INSTANCE_ID", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (object_draw_type == Object::kDrawAllShadowCascades) {
        FormatString(shader_str[1], kShaderStrSize, "%s #SHADOW_CASCADE", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (scenegraph->light_probe_collection.light_volume_enabled && scenegraph->light_probe_collection.ambient_3d_tex.valid()) {
        FormatString(shader_str[1], kShaderStrSize, "%s #CAN_USE_3D_TEX", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (!(Graphics::Instance()->config_.motion_blur_amount_ > 0.01f)) {
        FormatString(shader_str[1], kShaderStrSize, "%s #NO_VELOCITY_BUF", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (scenegraph->level->script_params().HasParam("Custom Shader") && config["custom_level_shaders"].toNumber<bool>()) {
        const std::string& custom_shader = scenegraph->level->script_params().GetStringVal("Custom Shader");
        if (!custom_shader.empty()) {
            FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], custom_shader.c_str());
            std::swap(shader_str[0], shader_str[1]);
        }
    }
    if (Graphics::Instance()->config_.simple_fog()) {
        FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], "#SIMPLE_FOG");
        std::swap(shader_str[0], shader_str[1]);
    }

    for (int length = strlen(shader_str[0]), i = 0;
         i < length && shader_str[0][0] == ' ';
         ++i, shader_str[0]++)
        ;

    FormatString(global_shader_suffix_storage, kGlobalShaderSuffixLen, "%s", shader_str[0]);
}

extern bool last_ofr_is_valid;

// Draw all objects
void SceneGraph::Draw(SceneGraph::SceneDrawType scene_draw_type) {
    if (g_debug_runtime_disable_scene_graph_draw) {
        return;
    }

    Graphics* graphics = Graphics::Instance();
    Camera* camera = ActiveCameras::Get();

    UpdateShaderSuffix(this, Object::kFullDraw);

    graphics->setDepthFunc(GL_LEQUAL);
    if (nav_mesh_ && (nav_mesh_renderer_.IsNavMeshVisible() || nav_mesh_renderer_.IsCollisionMeshVisible())) {
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        graphics->Clear(true);

        nav_mesh_renderer_.Draw();
    }

    if (graphics->queued_screenshot && graphics->screenshot_mode == Graphics::kTransparentGameplay) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        graphics->Clear(true);
    }

    // List static meshes
    PROFILER_ENTER(g_profiler_ctx, "Draw env object batches");
    if (visible_objects_need_sort) {
        PROFILER_ENTER(g_profiler_ctx, "Sort visible objects");
        g_static_mesh_draw_sort_entries = &visible_static_meshes_;
        std::sort(visible_static_mesh_indices_.begin(), visible_static_mesh_indices_.end(), StaticMeshDrawSortIndices);
        visible_objects_need_sort = false;
        PROFILER_LEAVE(g_profiler_ctx);
    }
    static std::vector<EnvObject*> static_meshes_to_draw;
    static_meshes_to_draw.clear();
    static_meshes_to_draw.reserve(visible_static_meshes_.size());
    PROFILER_ENTER(g_profiler_ctx, "List visible objects / frustum cull");
    if (!nav_mesh_renderer_.IsCollisionMeshVisible()) {
        for (unsigned short index : visible_static_mesh_indices_) {
            EnvObject* eo = visible_static_meshes_[index];
            if (!eo->transparent && eo->enabled_) {
                if (camera->checkSphereInFrustum(eo->sphere_center_, eo->sphere_radius_)) {
                    static_meshes_to_draw.push_back(eo);
                }
            }
        }
    }
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_ENTER(g_profiler_ctx, "Issue draw calls");
    mat4 proj_view_mat = camera->GetProjMatrix() * camera->GetViewMatrix();
    mat4 prev_proj_view_mat = camera->GetProjMatrix() * camera->prev_view_mat;
    vec3 cam_pos = camera->GetPos();
    std::vector<mat4> shadow_matrix;
    shadow_matrix.resize(4);
    for (int i = 0; i < 4; ++i) {
        shadow_matrix[i] = camera->biasMatrix * graphics->cascade_shadow_mat[i];
    }
    int batch_start = 0;

    if (!graphics->drawing_shadow && !(graphics->queued_screenshot && graphics->screenshot_mode == 1)) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw sky");
        sky->Draw(this);
    }

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw flares");
        flares.Draw(Flares::kDiffuse);
    }

    static std::vector<DetailObjectSurfaceDrawCall> detail_objects_surfaces_to_draw;
    detail_objects_surfaces_to_draw.clear();
    detail_objects_surfaces_to_draw.reserve(visible_static_meshes_.size());

    last_ofr_is_valid = false;
    for (int i = 1, len = static_meshes_to_draw.size(); i <= len; ++i) {
        if (i == len || (static_meshes_to_draw[i]->ofr->path_ != static_meshes_to_draw[i - 1]->ofr->path_ ||
                         static_meshes_to_draw[i]->winding_flip != static_meshes_to_draw[i - 1]->winding_flip)) {
            static_meshes_to_draw[i - 1]->DrawInstances(&static_meshes_to_draw[batch_start], i - batch_start, proj_view_mat, prev_proj_view_mat, &shadow_matrix, cam_pos, Object::kFullDraw);
            if (static_meshes_to_draw[i - 1]->HasDetailObjectSurfaces()) {
                detail_objects_surfaces_to_draw.push_back({static_meshes_to_draw[i - 1], &static_meshes_to_draw[batch_start], i - batch_start});
            }
            batch_start = i;
        }
    }
    EnvObject::AfterDrawInstances();
    for (auto current : detail_objects_surfaces_to_draw) {
        current.draw_owner->DrawDetailObjectInstances(current.instance_array, current.num_instances, Object::kFullDraw);
    }
    if (g_draw_collision) {
        batch_start = 0;
        last_ofr_is_valid = false;
        for (int i = 1, len = static_meshes_to_draw.size(); i <= len; ++i) {
            if (i == len || (static_meshes_to_draw[i]->ofr->path_ != static_meshes_to_draw[i - 1]->ofr->path_ ||
                             static_meshes_to_draw[i]->winding_flip != static_meshes_to_draw[i - 1]->winding_flip)) {
                if (static_meshes_to_draw[i - 1]->ofr->bush_collision) {
                    if (static_meshes_to_draw[i - 1]->plant_component_.get()) {
                        HullCache* cache = static_meshes_to_draw[i - 1]->plant_component_->GetHullCache();
                        if (cache) {
                            for (int instance = batch_start; instance < i; ++instance) {
                                EnvObject* eo = static_meshes_to_draw[instance];
                                for (unsigned face_index = 0; face_index < cache->faces.size(); face_index += 3) {
                                    for (int side = 0; side < 3; ++side) {
                                        DebugDraw::Instance()->AddLine(eo->GetTransform() * cache->verts[cache->faces[face_index + side]], eo->GetTransform() * cache->verts[cache->faces[face_index + (side + 1) % 3]], vec4(0.0f, 1.0f, 0.0f, 1.0f), _delete_on_draw);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    static_meshes_to_draw[i - 1]->DrawInstances(&static_meshes_to_draw[batch_start], i - batch_start, proj_view_mat, prev_proj_view_mat, &shadow_matrix, cam_pos, Object::kWireframe);
                    // Intentionally not drawing detail object surfaces
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                batch_start = i;
            }
        }
        EnvObject::AfterDrawInstances();
    }
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_LEAVE(g_profiler_ctx);

    {
        // Update whether character decals should be enabled/disabled
        bool character_decals_enabled = false;
        if (config["custom_level_shaders"].toNumber<bool>() && level->script_params().HasParam("Custom Shader")) {
            const std::string& custom_shader = level->script_params().GetStringVal("Custom Shader");
            if (!custom_shader.empty()) {
                character_decals_enabled = custom_shader.find("#CHARACTER_DECALS") != std::string::npos;
            }
        }
        g_character_decals_enabled = character_decals_enabled;
    }

    {
        // First non transparent, then transparent objects.
        PROFILER_ZONE(g_profiler_ctx, "Draw objects");
        for (auto& visible_object : visible_objects_) {
            Object& obj = *visible_object;
            const EntityType& obj_type = obj.GetType();
            switch (obj_type) {
                case _decal_object:
                case _env_object:
                case _hotspot_object:
                case _group:
                    continue;
                case _terrain_type:
                    break;
                default:
                    if (scene_draw_type == kStaticOnly) {
                        continue;
                    }
            }
            if (obj.enabled_ && !obj.transparent) {
                PROFILER_GPU_ZONE(g_profiler_ctx, "%s %d draw", CStringFromEntityType(obj.GetType()), obj.GetID());
                bool output_velocity_buffer = (graphics->config_.motion_blur_amount_ > 0.01f) && ((obj_type == _movement_object) || (obj_type == _item_object));
                if (output_velocity_buffer) {
                    GLenum buffers[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
                    glDrawBuffers(2, buffers);
                }
                // We want to hide the terrain if we're rendering the navmesh.
                if (obj_type == _terrain_type && nav_mesh_renderer_.IsCollisionMeshVisible()) {
                    continue;
                }
                obj.ReceiveObjectMessage(OBJECT_MSG::DRAW);
                if (output_velocity_buffer) {
                    GLenum buffers[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
                    glDrawBuffers(1, buffers);
                }
            }
        }

        bool draw_transparent = false;
        if (!nav_mesh_renderer_.IsCollisionMeshVisible()) {
            // Iterating backwards because transparent objects are last in the list
            for (std::vector<uint16_t>::reverse_iterator it = visible_static_mesh_indices_.rbegin(); it != visible_static_mesh_indices_.rend(); ++it) {
                uint16_t index = *it;
                EnvObject* eo = visible_static_meshes_[index];
                if (eo->enabled_) {
                    // Whatever the last enabled object in the list is will determine if there are any transparent objects
                    draw_transparent = eo->transparent;
                    break;
                }
            }
        }

        if (draw_transparent) {
            if (!g_simple_water) {
                PROFILER_ZONE(g_profiler_ctx, "Update depth/color textures");
                if (graphics->multisample_framebuffer_exists) {
                    PROFILER_ZONE(g_profiler_ctx, "Blit from MSAA buffers");
                    graphics->PushFramebuffer();
                    graphics->BlitColorBuffer();
                    graphics->BlitDepthBuffer();
                    graphics->PopFramebuffer();
                }

                {
                    PROFILER_ZONE(g_profiler_ctx, "Copy screen_color_tex to temp_screen_tex");
                    GLState gl_state;
                    gl_state.blend = false;
                    gl_state.cull_face = false;
                    gl_state.depth_test = false;
                    gl_state.depth_write = false;
                    graphics->setGLState(gl_state);

                    Shaders* shaders = Shaders::Instance();
                    Textures* textures = Textures::Instance();

                    graphics->PushViewport();
                    graphics->PushFramebuffer();

                    graphics->setViewport(0, 0, graphics->render_dims[0], graphics->render_dims[1]);
                    graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);

                    int shader_id = shaders->returnProgram(graphics->post_shader_name);
                    shaders->setProgram(shader_id);
                    shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
                    shaders->SetUniformInt("screen_height", graphics->render_dims[1]);

                    textures->bindTexture(graphics->screen_color_tex);
                    graphics->framebufferColorTexture2D(graphics->post_effects.temp_screen_tex);
                    quad_vert_vbo.Bind();
                    quad_index_vbo.Bind();
                    DrawQuad(shader_id);

                    graphics->PopFramebuffer();
                    graphics->PopViewport();
                }
            }

            {
                PROFILER_ZONE(g_profiler_ctx, "Draw transparent objects");
                for (auto& visible_static_meshe : visible_static_meshes_) {
                    EnvObject& obj = *visible_static_meshe;
                    if (obj.enabled_ && obj.transparent) {
                        PROFILER_ZONE(g_profiler_ctx, "%s %d draw", CStringFromEntityType(((Object*)&obj)->GetType()), obj.GetID());
                        // Avoid calling EnvObject::Draw repeatedly, so matrices etc can be shared instead of reacquired for every draw call
                        // TODO: last_ofr_is_valid is set to false in EnvObject::Draw - is it important?
                        obj.DrawInstances(&visible_static_meshe, 1, proj_view_mat, prev_proj_view_mat, &shadow_matrix, cam_pos, Object::kFullDraw);
                        EnvObject::AfterDrawInstances();
                        obj.DrawDetailObjectInstances(&visible_static_meshe, 1, Object::kFullDraw);
                    }
                }
            }
        }
    }

    {
        graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (graphics->use_sample_alpha_to_coverage) {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
        if (!Graphics::Instance()->media_mode() && map_editor->state_ != MapEditor::kInGame) {
            for (auto& decal_object : decal_objects_) {
                Object& obj = *decal_object;
                if (obj.enabled_ && obj.Selected() && obj.editor_visible) {
                    PROFILER_ZONE(g_profiler_ctx, "%s %d draw", CStringFromEntityType(obj.GetType()), obj.GetID());
                    obj.ReceiveObjectMessage(OBJECT_MSG::DRAW);
                }
            }
        }
    }
    if (_draw_collision_shapes) {
        if (bullet_world_)
            bullet_world_->Draw(sky->GetSpecularCubeMapTexture());
        // abstract_bullet_world->Draw();
        // plant_bullet_world->Draw();
    }

    // Draw hotspots
    if (map_editor->state_ == MapEditor::kInGame || map_editor->IsTypeEnabled(_hotspot_object)) {
        PROFILER_ZONE(g_profiler_ctx, "Draw Hotspots");
        for (auto& hotspot : hotspots_) {
            ((Hotspot*)hotspot)->Draw();
        }
    }
    {
        if (!bullet_world_) {
            FatalError("Error", "No bullet world");
        }
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw particles");
        particle_system->Draw(this);
        if (level->script_params().HasParam("GPU Particle Field") && config["particle_field"].toNumber<bool>()) {
            const std::string& gpu_particle_field = level->script_params().GetStringVal("GPU Particle Field");
            if (!gpu_particle_field.empty()) {
                DrawGPUParticleField(this, gpu_particle_field.c_str());
            }
        }
    }
}

Object* SceneGraph::GetLastSelected() {
    Object* res = NULL;
    for (auto& object : objects_) {
        if (res == NULL || object->Selected() > res->Selected()) {
            res = object;
        }
    }
    return res;
}

void SceneGraph::ReturnSelected(std::vector<Object*>* selected) {
    for (auto& object : objects_) {
        if (object->Selected()) {
            selected->push_back(object);
        }
    }
}

void SceneGraph::UnselectAll() {
    for (auto& object : objects_) {
        if (object->Selected()) {
            object->Select(false);
        }
    }
}

// Update all objects
void SceneGraph::Update(float timestep, float curr_game_time) {
    if (queued_level_reset_) {
        PROFILER_ZONE(g_profiler_ctx, "Resetting level");
        queued_level_reset_ = false;
        Level::CollisionPtrMap blank;
        level->HandleCollisions(blank, *this);
        SendMessageToAllObjects(OBJECT_MSG::RESET);
        level->Message("post_reset");
    }

    // uint64_t start_count = SDL_GetPerformanceCounter();
    {
        PROFILER_ZONE(g_profiler_ctx, "Bullet world update");
        bullet_world_->Update(timestep);
    }
    // uint64_t end_count = SDL_GetPerformanceCounter();
    plant_bullet_world_->Update(timestep);

    {
        PROFILER_ZONE(g_profiler_ctx, "Abstract world collisions");
        level->HandleCollisions(abstract_bullet_world_->GetCollisions(), *this);
    }

    {
        // Only updating specific subtypes of objects? -Max
        PROFILER_ZONE(g_profiler_ctx, "Object updates");
        for (int i = 0; i < num_update_objects; ++i) {
            Object* obj = update_objects_[i];
            PROFILER_ZONE(g_profiler_ctx, "%s %d update", CStringFromEntityType(obj->GetType()), obj->GetID());
            obj->ReceiveObjectMessage(OBJECT_MSG::UPDATE, timestep);
        }
        /*
        for(object_list::iterator it = objects_.begin(); it != objects_.end();) {
            Object* obj = *it;
            if(obj->active){
                PROFILER_ZONE(g_profiler_ctx,"%s %d update",CStringFromEntityType(obj->GetType()),obj->GetID());
                obj->Update();
            }
            ++it;
            if(obj->to_delete){
                UnlinkObject(obj);
                delete obj;
            }
        }*/
    }

    // Checking sanity, a couple of objects at a time
    int partial_loop_countdown = 10;
    size_t objects_size = objects_.size();
    while (objects_size > 0 && partial_loop_countdown > 0) {
        if (partial_object_loop_counter >= objects_size) {
            partial_object_loop_counter = 0;
        }

        int i = partial_object_loop_counter;

        ObjectSanityState oss = objects_[i]->GetSanity();

        if (oss.Ok()) {
            for (auto& k : sanity_list) {
                if (k.GetID() == oss.GetID()) {
                    k = ObjectSanityState();
                }
            }
        } else {
            int index = -1;
            for (unsigned k = 0; k < kMaxWarnings; k++) {
                if (sanity_list[k].GetID() == oss.GetID()) {
                    index = k;
                } else if (sanity_list[k].GetID() == -1) {
                    if (index == -1) {
                        index = k;
                    }
                }
            }

            if (index != -1) {
                sanity_list[index] = oss;
            }
        }

        partial_object_loop_counter++;
        partial_loop_countdown--;
    }

    if (nav_mesh_ && (nav_mesh_renderer_.IsNavMeshVisible() || nav_mesh_renderer_.IsCollisionMeshVisible())) {
        PROFILER_ZONE(g_profiler_ctx, "Navmesh update");
        nav_mesh_->Update();
    }

    // Update hotspots.

    // If a hotspot adds another hotspot, the iterator risks being invalidated,
    // so track which objects have been modified, and then find the correct index to keep updating at
    hotspots_modified_ = false;
    object_list updated_hotspots;
    updated_hotspots.reserve(hotspots_.size());
    for (object_list::iterator it = hotspots_.begin(); it != hotspots_.end(); ++it) {
        PROFILER_ZONE(g_profiler_ctx, "Hotspot update");
        updated_hotspots.push_back(*it);
        (*it)->ReceiveObjectMessage(OBJECT_MSG::UPDATE, timestep);

        if (hotspots_modified_) {
            for (int i = (int)updated_hotspots.size() - 1; i >= 0; --i) {
                it = std::find(hotspots_.begin(), hotspots_.end(), updated_hotspots[i]);
                if (it != hotspots_.end())
                    break;
            }

            if (it == hotspots_.end())
                it = hotspots_.begin();
            hotspots_modified_ = false;
        }
    }

    {
        PROFILER_ZONE(g_profiler_ctx, "Particle update");
        particle_system->Update(this, timestep, curr_game_time);
    }

    if (objects_.size() > 0) {
        for (int c = 0; c < 64; c++) {
            if (infreq_update_index >= objects_.size()) {
                infreq_update_index = 0;
            }
            objects_[infreq_update_index]->ReceiveObjectMessage(OBJECT_MSG::INFREQUENT_UPDATE);
            infreq_update_index++;
        }
    }
}

static bool RemoveObjFromList(Object* o, SceneGraph::object_list* list) {
    SceneGraph::object_list::iterator it = std::find(list->begin(), list->end(), o);
    if (it != list->end()) {
        list->erase(it);
        return true;
    }
    return false;
}

template <typename T>
static bool RemoveDerivedObjFromList(EntityType type, Object* o, std::vector<T*>* list) {
    if (o->GetType() == type) {
        typename std::vector<T*>::iterator it = std::find(list->begin(), list->end(), (T*)o);
        if (it != list->end()) {
            list->erase(it);
            return true;
        }
    }
    return false;
}

template <typename T, typename U>
static bool RemoveDerivedObjFromList(EntityType type, Object* o, std::vector<T*>* list, std::vector<U>* second_parallel_list) {
    if (o->GetType() == type) {
        typename std::vector<T*>::iterator it = std::find(list->begin(), list->end(), (T*)o);
        if (it != list->end()) {
            auto distance_from_begin = std::distance(list->begin(), it);
            list->erase(it);
            second_parallel_list->erase(second_parallel_list->begin() + distance_from_begin);
            return true;
        }
    }
    return false;
}

template <typename T, typename U, typename I>
static bool RemoveDerivedObjFromListAndIndex(EntityType type, Object* o, std::vector<T*>* list, std::vector<U>* second_parallel_list, std::vector<I>* index_list) {
    if (o->GetType() == type) {
        typename std::vector<T*>::iterator it = std::find(list->begin(), list->end(), (T*)o);
        if (it != list->end()) {
            auto distance_from_begin = std::distance(list->begin(), it);
            list->erase(it);
            second_parallel_list->erase(second_parallel_list->begin() + distance_from_begin);
            auto index_list_it = std::find(index_list->begin(), index_list->end(), (I)distance_from_begin);
            index_list->erase(index_list_it);
            for (int i = 0; i < index_list->size(); ++i) {  // Decrement all above the removed index
                (*index_list)[i] = (*index_list)[i] - ((*index_list)[i] > (I)distance_from_begin ? 1 : 0);
            }
            return true;
        }
    }
    return false;
}

void SceneGraph::UnlinkObject(Object* o) {
    int id = o->GetID();
    Object* object_at_id = GetIdToObjectMapValue(object_from_id_map_, id);

    LOGD << "Unlinking object: " << *o << std::endl;
    destruction_sanity_insert_position = (destruction_sanity_insert_position + 1) % destruction_sanity_size;
    destruction_sanity[destruction_sanity_insert_position] = o;

    destruction_memory_insert_position = (destruction_memory_insert_position + 1) % destruction_memory_size;

    destruction_memory_ids[destruction_memory_insert_position] = o->GetID();
    o->GetDisplayName(&destruction_memory_strings[destruction_memory_insert_position * destruction_memory_string_size], destruction_memory_string_size - 1);

    if (object_at_id != NULL) {
        if (o != object_at_id) {
            LOGW << "Object removal mismatch." << std::endl;
        }

        SetIdToObjectMapValue(object_from_id_map_, id, NULL);
    }

    if (o == terrain_object_) {
        terrain_object_ = NULL;
    }

    RemoveObjFromList(o, &visible_objects_);
    RemoveDerivedObjFromListAndIndex(_env_object, o, &visible_static_meshes_, &visible_static_meshes_shadow_cache_bounds_, &visible_static_mesh_indices_);
    RemoveDerivedObjFromList(_terrain_type, o, &terrain_objects_, &terrain_objects_shadow_cache_bounds_);
    RemoveObjFromList(o, &collide_objects_);
    RemoveObjFromList(o, &movement_objects_);
    RemoveObjFromList(o, &item_objects_);
    RemoveObjFromList(o, &decal_objects_);
    RemoveObjFromList(o, &objects_);
    if (RemoveObjFromList(o, &hotspots_))
        hotspots_modified_ = true;
    RemoveObjFromList(o, &navmesh_hints_);
    RemoveObjFromList(o, &navmesh_connections_);
    RemoveObjFromList(o, &path_points_);
    RemoveDerivedObjFromList(_light_volume_object, o, &light_volume_objects_);

    for (decal_deque::iterator it = dynamic_decals.begin(); it != dynamic_decals.end();) {
        DecalObject* obj = *it;
        if (obj == o) {
            it = dynamic_decals.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& i : sanity_list) {
        if (i.GetID() == id) {
            i = ObjectSanityState();
        }
    }
}

Object* SceneGraph::GetObjectFromID(int object_id) {
    Object* object_at_id = GetIdToObjectMapValue(object_from_id_map_, object_id);
    if (object_at_id != NULL) {
        return object_at_id;
    } else {
        LOGW << "Requested an object with id " << object_id << " but found none. Last info known of this id is: " << GetDestroyedObjectInfo(object_id) << std::endl;
    }
    return NULL;
}

bool SceneGraph::DoesObjectWithIdExist(int object_id) {
    Object* object_at_id = GetIdToObjectMapValue(object_from_id_map_, object_id);
    bool result = object_at_id != NULL;
    return result;
}

std::vector<Object*> SceneGraph::GetObjectsOfType(enum EntityType type) {
    std::vector<Object*> ret;

    std::vector<Object*>::iterator objit;

    for (objit = objects_.begin(); objit != objects_.end(); objit++) {
        if ((*objit)->GetType() == type) {
            ret.push_back(*objit);
        }
    }

    return ret;
}

void SceneGraph::CreateNavMesh() {
    if (map_editor->GetTerrainPreviewMode()) {
        DisplayMessage("Cannot create navmesh", "It is not possible to create navmesh while previewing terrain");
        return;
    }

    PROFILER_ZONE(g_profiler_ctx, "CreateNavMesh");
    if (nav_mesh_) {
        delete nav_mesh_;
    }
    nav_mesh_ = new NavMesh();

    std::vector<Object*> regions = GetObjectsOfType(_navmesh_region_object);

    if (regions.size() > 0) {
        if (regions.size() > 1) {
            LOGW << "THere is more than one region object, using the first one, please remove all others" << std::endl;
        }

        nav_mesh_->SetExplicitBounderies(
            ((NavmeshRegionObject*)regions[0])->GetMinBounds(),
            ((NavmeshRegionObject*)regions[0])->GetMaxBounds());
    }

    AddSceneToNavmesh();
    nav_mesh_->CalcNavMesh();
    Graphics::Instance()->nav_mesh_out_of_date = false;

    nav_mesh_renderer_.LoadNavMesh(nav_mesh_);
}

void SceneGraph::SaveNavMesh() {
    PROFILER_ZONE(g_profiler_ctx, "SaveNavMesh");
    if (!nav_mesh_) {
        LOGE << "Unable to save navmesh, none is generated." << std::endl;
        return;
    }
    if (level_path_.isValid() == false) {
        LOGE << "Unable to save navmesh, level has no path." << std::endl;
        return;
    }
    LOGI << "Saving NAVMESH." << std::endl;
    nav_mesh_->Save(level_name_, level_path_);
}

bool SceneGraph::LoadNavMesh() {
    delete nav_mesh_;
    nav_mesh_ = new NavMesh();
    if (!nav_mesh_->Load(level_name_, level_path_)) {
        delete nav_mesh_;
        nav_mesh_ = NULL;
        Graphics::Instance()->nav_mesh_out_of_date = true;
        Graphics::Instance()->nav_mesh_out_of_date_chunk = -1;
    }

    nav_mesh_renderer_.LoadNavMesh(nav_mesh_);

    return nav_mesh_ != NULL;
}

void SceneGraph::AddSceneToNavmesh() {
    nav_mesh_->SetNavMeshParameters(level->nav_mesh_parameters_);
    object_list::iterator iter = collide_objects_.begin();
    for (; iter != collide_objects_.end(); ++iter) {
        Object* obj = (*iter);
        if (obj->GetType() == _env_object && !((EnvObject*)obj)->ofr->no_collision && !((EnvObject*)obj)->no_navmesh) {
            // Check that this object doesn't have a movementobject parent (meaning it is dynamic)
            EnvObject* env_obj = (EnvObject*)obj;
            if (env_obj->attached_ == NULL) {
                int model_id = env_obj->GetCollisionModelID();
                if (model_id != -1) {
                    const Model* model = &Models::Instance()->GetModel(model_id);
                    nav_mesh_->AddMesh(model->vertices, model->faces, env_obj->GetTransform());
                }
            }
        }
        if (obj->GetType() == _terrain_type) {
            TerrainObject* terrain_obj = (TerrainObject*)obj;
            const Model* model = terrain_obj->GetModel();
            mat4 identity;
            nav_mesh_->AddMesh(model->vertices, model->faces, identity);
        }
    }

    {
        std::vector<float> verts;
        std::vector<unsigned> faces;

        GetUnitBoxVertArray(verts, faces);

        for (iter = navmesh_hints_.begin(); iter != navmesh_hints_.end(); ++iter) {
            nav_mesh_->AddMesh(verts, faces, (*iter)->GetTransform());
        }
    }

    std::set<std::pair<int, int> > offmesh_connections;

    for (iter = navmesh_connections_.begin(); iter != navmesh_connections_.end(); ++iter) {
        NavmeshConnectionObject* obj = static_cast<NavmeshConnectionObject*>(*iter);

        obj->ResetConnectionOffMeshReference();

        int first_id = obj->GetID();
        int second_id;

        std::vector<NavMeshConnectionData>::iterator idit = obj->connections.begin();
        for (; idit != obj->connections.end(); idit++) {
            second_id = idit->other_object_id;
            // We always add the highest value first to ensure the connections are unique. (two-way assumed)
            if (first_id > second_id) {
                offmesh_connections.insert(std::pair<int, int>(first_id, second_id));
            } else if (second_id > first_id) {
                offmesh_connections.insert(std::pair<int, int>(second_id, first_id));
            } else {
                LOGE << "Offmesh connection is self-referential. \"" << first_id << "\"" << std::endl;
            }
        }
    }

    std::set<std::pair<int, int> >::iterator pair_it = offmesh_connections.begin();

    nav_mesh_->getInputGeom().deleteAllOffMeshConnections();

    int id_counter = 1000;

    for (; pair_it != offmesh_connections.end(); pair_it++) {
        Object* first = GetObjectFromID(pair_it->first);
        Object* second = GetObjectFromID(pair_it->second);

        if (first && second) {
            if (first->GetType() == _navmesh_connection_object) {
                NavmeshConnectionObject* first_nco = static_cast<NavmeshConnectionObject*>(first);
                first_nco->UpdatePolyAreas();
            }

            if (second->GetType() == _navmesh_connection_object) {
                NavmeshConnectionObject* second_nco = static_cast<NavmeshConnectionObject*>(second);
                second_nco->UpdatePolyAreas();
            }

            SamplePolyAreas area = SAMPLE_POLYAREA_DISABLED;

            // TODO: We might want to assert that the poly areas are the same from both connections.
            if (first->GetType() == _navmesh_connection_object) {
                NavmeshConnectionObject* nmco = static_cast<NavmeshConnectionObject*>(first);
                std::vector<NavMeshConnectionData>::iterator other = nmco->GetConnectionTo(pair_it->second);
                if (other != nmco->connections.end()) {
                    other->offmesh_connection_id = id_counter;
                    area = other->poly_area;
                }
            }

            if (second->GetType() == _navmesh_connection_object) {
                NavmeshConnectionObject* nmco = static_cast<NavmeshConnectionObject*>(second);
                std::vector<NavMeshConnectionData>::iterator other = nmco->GetConnectionTo(pair_it->first);
                if (other != nmco->connections.end()) {
                    other->offmesh_connection_id = id_counter;
                    area = other->poly_area;
                }
            }

            LOGI << "Adding off mesh connection between " << first->GetTranslation() << " and " << second->GetTranslation() << " type " << area << std::endl;

            unsigned short jump_category = SAMPLE_POLYFLAGS_JUMP5;

            if (area == SAMPLE_POLYAREA_JUMP1) {
                jump_category = SAMPLE_POLYFLAGS_JUMP1;
            }

            if (area == SAMPLE_POLYAREA_JUMP2) {
                jump_category = SAMPLE_POLYFLAGS_JUMP2;
            }

            if (area == SAMPLE_POLYAREA_JUMP3) {
                jump_category = SAMPLE_POLYFLAGS_JUMP3;
            }

            if (area == SAMPLE_POLYAREA_JUMP4) {
                jump_category = SAMPLE_POLYFLAGS_JUMP4;
            }

            if (area == SAMPLE_POLYAREA_JUMP5) {
                jump_category = SAMPLE_POLYFLAGS_JUMP5;
            }

            if (area == SAMPLE_POLYAREA_DISABLED) {
                jump_category = SAMPLE_POLYFLAGS_DISABLED;
            }

            nav_mesh_->getInputGeom().addOffMeshConnection(
                first->GetTranslation(),
                second->GetTranslation(),
                1.0f,                  // rad
                DT_OFFMESH_CON_BIDIR,  // bidir
                area,                  // User defined area id
                jump_category,
                id_counter++);  // User defined flags
        } else {
            LOGW << "Invalid offmesh connection between " << pair_it->first << " and " << pair_it->second << std::endl;
        }
    }
}

NavMesh* SceneGraph::GetNavMesh() {
    return nav_mesh_;
}

Object* SceneGraph::GetClosestObject(const vec3& pos, Object* excluded_object, vec3* hit_pos, int* hit_tri) {
    ContactInfoCallback cb;
    float size = 0.1f;
    bool hit_something = false;
    while (!hit_something && size < 0.5f) {
        bullet_world_->GetSphereCollisions(pos, size, cb);
        size += 0.1f;
        for (int i = 0; i < cb.contact_info.size(); ++i) {
            BulletObject* bo = cb.contact_info[i].object;
            if (bo &&
                bo->owner_object &&
                bo->owner_object->GetType() != _movement_object &&
                bo->owner_object != excluded_object) {
                hit_something = true;
            }
        }
    }
    // DebugDraw::Instance()->AddWireSphere(event_pos, size,vec4(1.0f),_persistent);
    Object* closest_obj = NULL;
    float closest_dist;
    vec3 closest_point;
    int closest_tri;
    for (int i = 0; i < cb.contact_info.size(); ++i) {
        BulletObject* bo = cb.contact_info[i].object;
        if (bo &&
            bo->owner_object &&
            (bo->owner_object->GetType() == _movement_object ||
             bo->owner_object == excluded_object)) {
            continue;
        }
        const vec3& point = vec3(cb.contact_info[i].point[0],
                                 cb.contact_info[i].point[1],
                                 cb.contact_info[i].point[2]);
        if (closest_obj == NULL ||
            distance_squared(pos, point) < closest_dist) {
            closest_dist = distance_squared(pos, point);
            closest_obj = bo->owner_object;
            closest_point = point;
            closest_tri = cb.contact_info[i].tri;
        }
    }
    if (closest_obj != NULL) {
        if (hit_pos) {
            *hit_pos = closest_point;
        }
        if (hit_tri) {
            *hit_tri = closest_tri;
        }
    }
    return closest_obj;
}

const MaterialEvent* SceneGraph::GetMaterialEvent(const std::string& the_event, const vec3& event_pos) {
    int hit_tri;
    vec3 hit_pos;
    Object* closest_obj = GetClosestObject(event_pos, NULL, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return NULL;
    }
    return &closest_obj->GetMaterialEvent(the_event, event_pos, &hit_tri);
}

const MaterialEvent* SceneGraph::GetMaterialEvent(const std::string& the_event, const vec3& event_pos, Object* excluded_object) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(event_pos, excluded_object, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return NULL;
    }
    return &closest_obj->GetMaterialEvent(the_event, event_pos, &hit_tri);
}

const MaterialEvent* SceneGraph::GetMaterialEvent(const std::string& the_event, const vec3& event_pos, const std::string& mod) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(event_pos, NULL, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return NULL;
    }
    return &closest_obj->GetMaterialEvent(the_event, event_pos, mod, &hit_tri);
}

const MaterialEvent* SceneGraph::GetMaterialEvent(const std::string& the_event, const vec3& event_pos, const std::string& mod, Object* excluded_object) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(event_pos, excluded_object, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return NULL;
    }
    return &closest_obj->GetMaterialEvent(the_event, event_pos, mod, &hit_tri);
}

const MaterialDecal* SceneGraph::GetMaterialDecal(const std::string& type,
                                                  const vec3& pos) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(pos, NULL, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return NULL;
    }
    return &closest_obj->GetMaterialDecal(type, pos, &hit_tri);
}

const MaterialParticle* SceneGraph::GetMaterialParticle(const std::string& type, const vec3& pos) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(pos, NULL, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return NULL;
    }
    return &closest_obj->GetMaterialParticle(type, pos, &hit_tri);
}

vec3 SceneGraph::GetColorAtPoint(const vec3& pos) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(pos, NULL, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return vec3(0);
    }
    return closest_obj->GetColorAtPoint(pos, &hit_tri);
}

void SceneGraph::UpdatePhysicsTransforms() {
    for (auto& visible_static_meshe : visible_static_meshes_) {
        visible_static_meshe->UpdatePhysicsTransform();
    }
}

void SceneGraph::GetPlayerCharacterIDs(int* num_avatars, int avatar_ids[], int max_avatars) {
    *num_avatars = 0;
    for (auto& movement_object : movement_objects_) {
        MovementObject* mo = (MovementObject*)movement_object;
        if (mo->is_player) {
            if (*num_avatars < max_avatars) {
                avatar_ids[*num_avatars] = mo->GetID();
                ++*num_avatars;
            } else {
                LOG_ASSERT(false);
            }
        }
    }
}

void SceneGraph::GetNPCCharacterIDs(int* num_avatars, int avatar_ids[], int max_avatars) {
    *num_avatars = 0;
    for (auto& movement_object : movement_objects_) {
        MovementObject* mo = (MovementObject*)movement_object;
        if (!mo->is_player) {
            if (*num_avatars < max_avatars) {
                avatar_ids[*num_avatars] = mo->GetID();
                ++*num_avatars;
            } else {
                LOG_ASSERT(false);
            }
        }
    }
}

void SceneGraph::GetCharacterIDs(int* num_avatars, int avatar_ids[], int max_avatars) {
    *num_avatars = 0;
    for (auto& movement_object : movement_objects_) {
        MovementObject* mo = (MovementObject*)movement_object;
        if (*num_avatars < max_avatars) {
            avatar_ids[*num_avatars] = mo->GetID();
            ++*num_avatars;
        } else {
            LOG_ASSERT(false);
        }
    }
}

float SceneGraph::GetMaterialHardness(const vec3& pos, Object* excluded_object) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(pos, excluded_object, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return 1.0f;
    }
    return closest_obj->GetMaterial(pos, &hit_tri)->GetHardness();
}

float SceneGraph::GetMaterialFriction(const vec3& pos, Object* excluded_object) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(pos, excluded_object, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return 1.0f;
    }
    // DebugDraw::Instance()->AddWireSphere(hit_pos, 0.1f, vec4(1.0f), _fade);
    return closest_obj->GetMaterial(pos, &hit_tri)->GetFriction();
}

float SceneGraph::GetMaterialSharpPenetration(const vec3& pos, Object* excluded_object) {
    vec3 hit_pos;
    int hit_tri;
    Object* closest_obj = GetClosestObject(pos, excluded_object, &hit_pos, &hit_tri);
    if (!closest_obj) {
        return 0.0f;
    }
    return closest_obj->GetMaterial(pos, &hit_tri)->GetSharpPenetration();
}

void SceneGraph::GetSweptSphereCollisionCharacters(const vec3& pos, const vec3& pos2, float radius, SphereCollision& as_col) {
    vec3 end = pos2;
    for (auto& movement_object : movement_objects_) {
        MovementObject* mo = (MovementObject*)movement_object;
        mo->rigged_object()->skeleton().GetSweptSphereCollisionCharacter(pos, end, radius, as_col);
        end = as_col.position;
    }
}

int SceneGraph::CheckRayCollisionCharacters(const vec3& start, const vec3& end, vec3* point, vec3* normal, int* bone) {
    vec3 new_end = end;
    vec3 temp_point;
    vec3 temp_normal;
    int char_id = -1;
    for (auto& movement_object : movement_objects_) {
        MovementObject* mo = (MovementObject*)movement_object;
        const btCollisionObject* bone_col = mo->rigged_object()->skeleton().CheckRayCollision(start, new_end, &temp_point, &temp_normal);
        if (bone_col != NULL) {
            if (point) {
                *point = temp_point;
            }
            if (normal) {
                *normal = temp_normal;
            }
            if (bone) {
                BulletObject* bullet_obj = (BulletObject*)bone_col->getUserPointer();
                if (bullet_obj) {
                    PhysicsBone* pb = (PhysicsBone*)bullet_obj->owner_object;
                    if (pb) {
                        *bone = pb->bone;
                    }
                }
            }
            new_end = temp_point;
            char_id = mo->GetID();
        }
    }
    return char_id;
}

void SceneGraph::SendMessageToAllObjects(OBJECT_MSG::Type type) {
    // This is a simple loop instead of an iterator because sometimes this would crash as an iterator after
    // passing way past the end. Likely because objects_ is sometimes changed as a result of this call.
    // This solution means that most objects get called, some might not if they are first destroyed.
    // New objects will also be called assuming they are added last to the list.
    for (unsigned int i = 0; i < objects_.size(); i++) {
        Object* obj = objects_[i];
        if (obj) {
            obj->ReceiveObjectMessage(type);
        } else {
            LOGE << "One of the objects is NULL when trying to SendMessageToAllObjects." << std::endl;
        }
    }
}

void SceneGraph::SendScriptMessageToAllObjects(std::string& msg) {
    // For consistency, I'm going to also make this a simple loop in case the same iterator problems affect
    // This function as well.
    for (unsigned int i = 0; i < objects_.size(); i++) {
        Object* obj = objects_[i];
        if (obj) {
            obj->ReceiveObjectMessage(OBJECT_MSG::SCRIPT, &msg);
        } else {
            LOGE << "One of the objects is NULL when trying to SendScriptMessageToAllObjects." << std::endl;
        }
    }
}

std::vector<MovementObject*> SceneGraph::GetControlledMovementObjects() {
    std::vector<MovementObject*> controlled_objects;
    for (Object* obj : movement_objects_) {
        MovementObject* mo = static_cast<MovementObject*>(obj);

        if (mo->controlled) {
            controlled_objects.push_back(mo);
        }
    }
    return controlled_objects;
}

std::vector<MovementObject*> SceneGraph::GetControllableMovementObjects() {
    std::vector<MovementObject*> controllable_objects;
    for (Object* obj : movement_objects_) {
        MovementObject* mo = static_cast<MovementObject*>(obj);

        if (mo->is_player) {
            controllable_objects.push_back(mo);
        }
    }
    return controllable_objects;
}

bool SceneGraph::VerifySanity() {
    for (auto& sanity : sanity_list) {
        if (sanity.Valid() && sanity.Ok() == false) {
            return false;
        }
    }
    return true;
}

// Adapted from https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
float orient2d(const float* a, const float* b, const float* c) {
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
}

void SceneGraph::DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, SceneGraph::DepthType depth_type, SceneDrawType scene_draw_type) {
    if (g_debug_runtime_disable_scene_graph_draw_depth_map) {
        return;
    }

    Object::DrawType object_draw_type = Object::kUnknown;
    if (depth_type == SceneGraph::kDepthPrePass) {
        object_draw_type = Object::kDrawDepthOnly;
    } else if (depth_type == SceneGraph::kDepthShadow) {
        object_draw_type = Object::kDrawDepthNoAA;
    } else if (depth_type == SceneGraph::kDepthAllShadowCascades) {
        object_draw_type = Object::kDrawAllShadowCascades;
    }

    UpdateShaderSuffix(this, object_draw_type);

    {
        static object_list visible_objects_copy;
        visible_objects_copy = visible_objects_;
        PROFILER_ZONE(g_profiler_ctx, "Draw dynamic objects");
        if (scene_draw_type == kStaticAndDynamic) {
            for (auto& it : visible_objects_copy) {
                Object& obj = *it;
                if (obj.enabled_ && obj.GetType() != _decal_object && obj.GetType() != _env_object) {
                    obj.DrawDepthMap(proj_view_matrix, cull_planes, num_cull_planes, object_draw_type);
                }
            }
        } else {
            for (auto& it : visible_objects_copy) {
                Object& obj = *it;
                if (obj.enabled_ && obj.GetType() == _terrain_type) {
                    obj.DrawDepthMap(proj_view_matrix, cull_planes, num_cull_planes, object_draw_type);
                }
            }
        }
    }

    if (!kUseShadowCache) {
        if (visible_objects_need_sort) {
            PROFILER_ENTER(g_profiler_ctx, "Sort visible objects");
            g_static_mesh_draw_sort_entries = &visible_static_meshes_;
            std::sort(visible_static_mesh_indices_.begin(), visible_static_mesh_indices_.end(), StaticMeshDrawSortIndices);
            visible_objects_need_sort = false;
            PROFILER_LEAVE(g_profiler_ctx);
        }
        vec3 cam_pos = ActiveCameras::Get()->GetPos();
        // List static meshes
        PROFILER_ENTER(g_profiler_ctx, "Draw env object batches (depth only)");
        static std::vector<EnvObject*> static_meshes_to_draw;
        static_meshes_to_draw.clear();
        static_meshes_to_draw.reserve(visible_static_meshes_.size());
        PROFILER_ENTER(g_profiler_ctx, "List visible objects / frustum cull");
        for (unsigned short index : visible_static_mesh_indices_) {
            EnvObject* eo = visible_static_meshes_[index];
            if (!eo->transparent && eo->enabled_) {
                bool culled = false;
                for (int plane = 0; plane < num_cull_planes; ++plane) {
                    if (dot(eo->sphere_center_, cull_planes[plane].xyz()) +
                            cull_planes[plane][3] + eo->sphere_radius_ <=
                        0.0f) {
                        culled = true;
                        break;
                    }
                }
                if (!culled) {
                    static_meshes_to_draw.push_back(eo);
                }
            }
        }
        PROFILER_LEAVE(g_profiler_ctx);
        PROFILER_ENTER(g_profiler_ctx, "Issue draw calls");
        int batch_start = 0;
        last_ofr_is_valid = false;
        for (int i = 1, len = static_meshes_to_draw.size(); i <= len; ++i) {
            if (i == len || (static_meshes_to_draw[i]->ofr->path_ != static_meshes_to_draw[i - 1]->ofr->path_ ||
                             static_meshes_to_draw[i]->winding_flip != static_meshes_to_draw[i - 1]->winding_flip)) {
                static_meshes_to_draw[i - 1]->DrawInstances(&static_meshes_to_draw[batch_start], i - batch_start, proj_view_matrix, proj_view_matrix, NULL, cam_pos, object_draw_type);
                batch_start = i;
            }
        }
        EnvObject::AfterDrawInstances();
        if (object_draw_type != Object::kDrawDepthOnly) {
            // Batch and draw detail objects
            static std::vector<DetailObjectSurfaceDrawCall> detail_objects_surfaces_to_draw;
            detail_objects_surfaces_to_draw.clear();
            detail_objects_surfaces_to_draw.reserve(visible_static_meshes_.size());
            batch_start = 0;
            for (int i = 1, len = static_meshes_to_draw.size(); i <= len; ++i) {
                if (i == len || (static_meshes_to_draw[i]->ofr->path_ != static_meshes_to_draw[i - 1]->ofr->path_ ||
                                 static_meshes_to_draw[i]->winding_flip != static_meshes_to_draw[i - 1]->winding_flip)) {
                    if (static_meshes_to_draw[i - 1]->HasDetailObjectSurfaces()) {
                        detail_objects_surfaces_to_draw.push_back({static_meshes_to_draw[i - 1], &static_meshes_to_draw[batch_start], i - batch_start});
                    }
                    batch_start = i;
                }
            }
            for (auto current : detail_objects_surfaces_to_draw) {
                current.draw_owner->DrawDetailObjectInstances(current.instance_array, current.num_instances, object_draw_type);
            }
        }
        PROFILER_LEAVE(g_profiler_ctx);
        PROFILER_LEAVE(g_profiler_ctx);
    } else {
    }
    /*if(depth_type == SceneGraph::kDepthShadow){
        particle_system->DrawDepth(this, proj_view_matrix);
    }*/
}

int SceneGraph::GetAndReserveID() {
    int curr_id = 0;
    int last_id = -1;
    int free_id = -1;
    for (unsigned int i = 0; i < object_from_id_map_.size(); ++i) {
        if (object_from_id_map_[i] == NULL) {
            free_id = i;
            break;
        }
    }
    if (free_id == -1) {
        free_id = object_from_id_map_.size();
    }
    SetIdToObjectMapValue(object_from_id_map_, free_id, NULL);
    return free_id;
}

int SceneGraph::LinkUpdateObject(Object* obj) {
    if (num_update_objects < kMaxUpdateObjects) {
        int id = num_update_objects++;
        update_objects_[id] = obj;
        return id;
    } else {
        FatalError("Error", "Too many update objects");
        return -1;
    }
}

void SceneGraph::UnlinkUpdateObject(Object* obj, int entry) {
    SDL_assert(update_objects_[entry] == obj);
    if (entry != num_update_objects - 1) {
        update_objects_[entry] = update_objects_[num_update_objects - 1];
        update_objects_[entry]->update_list_entry = entry;
    }
    --num_update_objects;
}

void SceneGraph::Dispose() {
    for (auto& object : objects_) {
        object->Dispose();
    }
    for (auto& object : objects_) {
        delete object;
    }
    object_from_id_map_.clear();
    if (bullet_world_) {
        bullet_world_->Dispose();
        delete bullet_world_;
        bullet_world_ = NULL;
    }
    if (abstract_bullet_world_) {
        abstract_bullet_world_->Dispose();
        delete abstract_bullet_world_;
        abstract_bullet_world_ = NULL;
    }
    if (plant_bullet_world_) {
        plant_bullet_world_->Dispose();
        delete plant_bullet_world_;
        plant_bullet_world_ = NULL;
    }
    if (nav_mesh_) {
        delete nav_mesh_;
        nav_mesh_ = NULL;
        nav_mesh_renderer_.LoadNavMesh(nav_mesh_);
    }
    delete particle_system;
    particle_system = NULL;
    light_probe_collection.Dispose();
    dynamic_light_collection.Dispose();
    level->Dispose();
    delete level;
    level = NULL;
    delete map_editor;
    map_editor = NULL;
    sky->Dispose();
    delete sky;
    sky = NULL;
    flares.CleanupFlares();
    DecalTextures::Instance()->Clear();
}

void SceneGraph::SetNavMeshVisible(bool v) {
    nav_mesh_renderer_.SetNavMeshVisible(v);
}

void SceneGraph::SetCollisionNavMeshVisible(bool v) {
    nav_mesh_renderer_.SetCollisionMeshVisible(v);
}

bool SceneGraph::IsNavMeshVisible() {
    return nav_mesh_renderer_.IsNavMeshVisible();
}

bool SceneGraph::IsCollisionNavMeshVisible() {
    return nav_mesh_renderer_.IsCollisionMeshVisible();
}

bool SceneGraph::AddDynamicDecal(DecalObject* decal) {
    if (dynamic_decals.size() >= kMaxDynamicDecals) {
        DecalObject* obj = dynamic_decals.front();
        obj->SetParent(NULL);
        UnlinkObject(obj);
        obj->Dispose();
        delete (obj);
    }
    // make sure we don't clump too many decals into one place
    vec3 my_pos = decal->GetTranslation();

    // find nearest decal which is same type
    // c++11 auto would be really helpful here
    float nearest_dist_sq = FLT_MAX;

    Object* nearest_obj = NULL;
    decal_deque::iterator e = dynamic_decals.end();
    for (decal_deque::iterator it = dynamic_decals.begin(); it != e; ++it) {
        DecalObject* obj = *it;

        LOG_ASSERT(obj != NULL);
        if (obj->decal_file_ref->color_map != "Data/Textures/bloodsplat_c.tga" || obj->obj_file != decal->obj_file) {
            // not the same kind of decal, don't consider
            continue;
        }

        vec3 other_pos = obj->GetTranslation();
        float dist_sq = length_squared(my_pos - other_pos);
        if (dist_sq < nearest_dist_sq) {
            nearest_dist_sq = dist_sq;
            nearest_obj = obj;
        }
    }

    vec3 scale = decal->GetScale();
    float my_radius = min(scale.x(), min(scale.y(), scale.z()));
    float my_radius_sq = my_radius * my_radius;
    float other_radius_sq = 0.0f;
    if (nearest_obj) {
        scale = nearest_obj->GetScale();
        float other_radius = min(scale.x(), min(scale.y(), scale.z()));
        other_radius_sq = other_radius * other_radius;
    }
    if (nearest_dist_sq > my_radius_sq && nearest_dist_sq > other_radius_sq) {
        if (ActorsEditor_AddEntity(this, decal, NULL, false)) {
            dynamic_decals.push_back(decal);
        } else {
            LOGE << "Failed to add dynamic decal to scenegraph" << std::endl;
            return false;
        }
    } else {
        vec3 cur_scale = nearest_obj->GetScale();
        vec3 add_scale = decal->GetScale();
        float cur_volume = cur_scale[0] * cur_scale[1];
        float add_volume = add_scale[0] * add_scale[1];
        if (cur_scale[0] < cur_scale[1]) {
            float square_vol = cur_scale[1] * cur_scale[1];
            if (cur_volume + add_volume > square_vol) {
                cur_scale[0] = cur_scale[1];
                add_volume += cur_volume - square_vol;
            } else {
                cur_scale[0] *= (cur_volume + add_volume) / cur_volume;
                add_volume = 0.0f;
            }
        } else if (cur_scale[0] > cur_scale[1]) {
            float square_vol = cur_scale[0] * cur_scale[0];
            if (cur_volume + add_volume > square_vol) {
                cur_scale[1] = cur_scale[0];
                add_volume += cur_volume - square_vol;
            } else {
                cur_scale[1] *= (cur_volume + add_volume) / cur_volume;
                add_volume = 0.0f;
            }
        }
        float increase = (cur_volume + add_volume) / cur_volume;
        nearest_obj->SetScale(nearest_obj->GetScale() * increase);
        // on top of existing, don't add
        return false;
    }
    return true;
}

#define COUNT_BITS 24u
#define COUNT_MASK ((1u << (32u - COUNT_BITS)) - 1u)
#define INDEX_MASK ((1u << (COUNT_BITS)) - 1u)

uint32_t SetCount(uint32_t val, uint32_t count) {
    uint32_t count_shifted = count << COUNT_BITS;
    LOG_ASSERT(count_shifted >> COUNT_BITS == count);
    return val | count_shifted;
}

uint32_t SetIndex(uint32_t val, uint32_t index) {
    LOG_ASSERT(index == (index & INDEX_MASK));
    return val | index;
}

// - z_near + 1.0f = minimum of 1.0, log of that = 0.0f
// z_mult already contains num_z_clusters
float ZCLUSTERFUNC(float val, float z_near, float z_mult) {
    return log(-1.0f * (val)-z_near + 1.0f) * z_mult;
}

void SceneGraph::PrepareLightsAndDecals(vec2 active_screen_start, vec2 active_screen_end, vec2 screen_dims) {
    if (g_debug_runtime_disable_scene_graph_prepare_lights_and_decals) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "PrepareLightsAndDecals");
    // http://www.humus.name/index.php?page=Articles
    // "Practical Clustered Shading"
    // this is similar but NOT the same algorithm
    // 1. different z function
    // 2. doesn't use plane tests

    // 1. I couldn't get the paper's version to work
    //    somehow it doesn't cluster things correctly
    //    play with ZCLUSTERFUNC if you want to fix it (also in the shader)

    // 2. this code loops through all light/decals and converts them to screen space
    //    the paper build a set of planes aligned with screen x/y/z axes
    //    and tests primitives against those
    //    the naive way was simpler and we don't have plane test primitives yet

    // you need plane-box and plane-sphere tests
    // with return value of Front, Behind, Intersect
    // Math/enginemath, Math/overgroth_geometry and Internal/collisiondetection
    // contain some things but not the things we need

    Graphics* graphics = Graphics::Instance();
    const unsigned int width = (unsigned int)screen_dims[0];
    const unsigned int height = (unsigned int)screen_dims[1];

    Camera* camera = ActiveCameras::Get();
    LOG_ASSERT(camera);
    mat4 proj_mat = camera->GetProjMatrix();
    mat4 inv_proj_mat = invert(proj_mat);
    mat4 view_mat = camera->GetViewMatrix();
    mat4 proj_view_mat = proj_mat * view_mat;
    float z_near = camera->GetNearPlane();
    float z_far = camera->GetFarPlane();
    mat4 cluster_mat;

    {
        const vec3 screen_start(active_screen_start, 0.0f);
        const vec3 screen_end(active_screen_end, 1.0f);
        const vec3 screen_mult = screen_end - screen_start;

        mat4 scale;
        mat4 translate;
        // scale .xy from [-1.0, 1.0] to [0.0, 1.0]
        scale.SetUniformScale(0.5f);
        translate.SetTranslation(vec3(0.5f));
        cluster_mat = translate * scale * proj_mat;

        // account for currently active view
        scale.SetScale(screen_mult);
        translate.SetTranslation(screen_start);
        cluster_mat = translate * scale * cluster_mat;

        // scale to [(0,0), (width, height)]
        scale.SetScale(vec3((float)width, (float)height, 1.0f));
        cluster_mat = scale * cluster_mat;

        // scale to cluster space
        scale.SetScale(vec3(1.0f / cluster_size, 1.0f / cluster_size, 1.0f));
        cluster_mat = scale * cluster_mat;
    }

    const unsigned int grid_width = (width + cluster_size - 1) / cluster_size;
    const unsigned int grid_height = (height + cluster_size - 1) / cluster_size;

    const unsigned int numclusters = grid_width * grid_height * num_z_clusters;

    const vec2 grid_size = vec2(float(grid_width), float(grid_height));
    const vec3 cluster_min_bound(active_screen_start * grid_size, -z_far);
    const vec3 cluster_max_bound(active_screen_end * grid_size, 0.0f);

    // z values will be divided by this
    // the value inside log is maximum we should get in view space
    // after adding 1.0 so the log works
    // by dividing with num_z_clusters the final value ends up
    // multiplied by it so we get [0,num_z_clusters]
    float z_div = log(z_far - z_near + 1.0f) / num_z_clusters;
    float z_mult = 1.0f / z_div;

    ShaderClusterInfo clusterInfo;
    clusterInfo.num_decals = 0;
    clusterInfo.num_lights = 0;
    clusterInfo.light_cluster_data_offset = 0;
    clusterInfo.light_data_offset = 0;
    clusterInfo.cluster_width = cluster_size;

    clusterInfo.grid_size[0] = grid_width;
    clusterInfo.grid_size[1] = grid_height;
    clusterInfo.grid_size[2] = num_z_clusters;

    memcpy(clusterInfo.inv_proj_mat, &inv_proj_mat, 16 * 4);
    for (unsigned int i = 0; i < 4; i++) {
        clusterInfo.viewport[i] = (float)graphics->viewport_dim[i];
    }

    clusterInfo.z_near = z_near;
    clusterInfo.z_mult = z_mult;
    clusterInfo.pad3 = 0.0f;
    clusterInfo.pad4 = 0.0f;

    // resize buffers if necessary
    if (cluster_decal_counts.size() != numclusters) {
        cluster_decal_counts.resize(numclusters);
        cluster_light_counts.resize(numclusters);
        cluster_list_heads.resize(numclusters);
        decal_grid_lookup.resize(numclusters);
        light_grid_lookup.resize(numclusters);
    }

    LOG_ASSERT_EQ(decal_grid_lookup.size(), cluster_decal_counts.size());
    LOG_ASSERT_EQ(light_grid_lookup.size(), cluster_decal_counts.size());
    LOG_ASSERT_EQ(cluster_light_counts.size(), cluster_decal_counts.size());

    // number of decals alive (visible on screen) in current frame
    unsigned int num_alive_decals = 0;

    {
        PROFILER_ZONE(g_profiler_ctx, "Cluster decals");

        int num_decals = decal_objects_.size();
        LOG_ASSERT_LTEQ(num_decals, (int)kMaxDecals);

        if (decal_tbo.size() < num_decals * sizeof(ShaderDecal) / sizeof(float)) {
            decal_tbo.resize(num_decals * sizeof(ShaderDecal) / sizeof(float));
        }

        // LOGI << "num_decals = " << num_decals << std::endl;

        for (unsigned int i = 0; i < numclusters; i++) {
            cluster_decal_counts[i] = 0;
        }

        ShaderDecal decal;

        bool decal_normals = config["decal_normals"].toNumber<bool>();

        memset(&cluster_list_heads[0], 0, sizeof(uint32_t) * cluster_list_heads.size());
        cluster_list_contents.clear();
        cluster_list_contents.push_back(ClusterData());

        // assign decals into clusters
        for (int i = 0; i < num_decals; ++i) {
            DecalObject* dec = static_cast<DecalObject*>(decal_objects_[i]);
            if (dec->enabled_ == false) {
                continue;
            }

            decal.decal_tint[3] = dec->decal_file_ref->special_type + 0.5f;

            mat4 mv_mat = view_mat * dec->GetTransform();

            // calculate decal bounding box in view space
            vec3 decal_view_min(FLT_MAX, FLT_MAX, FLT_MAX);
            vec3 decal_view_max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            Box decal_box = dec->box_;
            for (int point = 0; point < Box::NUM_POINTS; point++) {
                // strange thing: view space z decreases farther away
                vec3 view_point = (mv_mat * vec4(decal_box.GetPoint(point), 1.0f)).xyz();
                decal_view_min = components_min(decal_view_min, view_point);
                decal_view_max = components_max(decal_view_max, view_point);
            }

            // LOGI << "decal " << i << std::endl;

            // drop it if it's entirely behind the camera
            if (decal_view_min.z() > -z_near) {
                continue;
            }
            /*
            LOGI << "decal_view_min: " << decal_view_min << std::endl;
            LOGI << "decal_view_max: " << decal_view_max << std::endl;
            */

            // cap at near plane
            decal_view_max.z() = min(-z_near, decal_view_max.z());

            Box view_box;
            view_box.center = (decal_view_min + decal_view_max) * 0.5f;
            view_box.dims = decal_view_max - decal_view_min;

            // calculate projection space bounding box
            vec2 decal_proj_min(FLT_MAX, FLT_MAX);
            vec2 decal_proj_max(-FLT_MAX, -FLT_MAX);

            // TODO: we probably don't need to do the whole box, just the
            // four points on the near face
            for (int point = 0; point < Box::NUM_POINTS; point++) {
                vec4 proj_point = cluster_mat * vec4(view_box.GetPoint(point), 1.0f);
                // we know it's in front of the screen since we capped it at near plane
                LOG_ASSERT_GTEQ(proj_point.w(), 0.0f);
                vec2 proj_point2 = proj_point.xy() * (1.0f / proj_point.w());

                decal_proj_min = components_min2(decal_proj_min, proj_point2);
                decal_proj_max = components_max2(decal_proj_max, proj_point2);
            }

            /*
            LOGI << "decal_proj_min: " << decal_proj_min << std::endl;
            LOGI << "decal_proj_max: " << decal_proj_max << std::endl;
            */

            // we use projection-space .xy and view-space .z for clustering
            vec3 decal_cluster_min(decal_proj_min, decal_view_min.z());
            vec3 decal_cluster_max(decal_proj_max, decal_view_max.z());

            // drop the decal if it's entirely off-screen
            if (decal_cluster_min.x() > cluster_max_bound.x() || decal_cluster_max.x() < cluster_min_bound.x() || decal_cluster_min.y() > cluster_max_bound.y() || decal_cluster_max.y() < cluster_min_bound.y()) {
                continue;
            }

            decal_cluster_min = components_max(decal_cluster_min, cluster_min_bound);
            decal_cluster_max = components_max(decal_cluster_max, cluster_min_bound);
            decal_cluster_min = components_min(decal_cluster_min, cluster_max_bound);
            decal_cluster_max = components_min(decal_cluster_max, cluster_max_bound);

            /*
            LOGI << "decal_cluster_min: " << decal_cluster_min << std::endl;
            LOGI << "decal_cluster_max: " << decal_cluster_max << std::endl << std::endl;
            LOGI << "view_point " << point << ": " << view_point << std::endl;
            LOGI << "proj_point " << point << ": " << proj_point << std::endl;
            LOGI << "screen_point " << point << ": " << screen_point << std::endl;
            */

            if (g_no_decal_elements) {
                continue;
            }

            unsigned int decal_index = num_alive_decals;
            num_alive_decals++;

            vec3 combined_tint = dec->color_tint_component_.tint_ * (1.0f + dec->color_tint_component_.overbright_ * 0.3f);
            memcpy(&decal.decal_tint, &combined_tint, 4 * 3);

            vec3 scale = dec->GetScale();
            decal.decal_scale[0] = scale.x();
            decal.decal_scale[1] = scale.y();
            decal.decal_scale[2] = scale.z();
            decal.decal_spawn_time = dec->spawn_time_;
            quaternion rotation = dec->GetRotation();
            memcpy(&decal.decal_rotation[0], &rotation.entries[0], 4 * sizeof(float));
            vec3 pos = dec->GetTranslation();
            decal.decal_position[0] = pos.x();
            decal.decal_position[1] = pos.y();
            decal.decal_position[2] = pos.z();
            decal.decal_pad1 = 0.0f;

            decal.decal_uv[0] = dec->texture->color_texture_ref.uv_start.entries[0];
            decal.decal_uv[1] = dec->texture->color_texture_ref.uv_start.entries[1];
            decal.decal_uv[2] = dec->texture->color_texture_ref.uv_size.entries[0];
            decal.decal_uv[3] = dec->texture->color_texture_ref.uv_size.entries[1];

            if (decal_normals) {
                decal.decal_normal[0] = dec->texture->normal_texture_ref.uv_start.entries[0];
                decal.decal_normal[1] = dec->texture->normal_texture_ref.uv_start.entries[1];
                decal.decal_normal[2] = dec->texture->normal_texture_ref.uv_size.entries[0];
                decal.decal_normal[3] = dec->texture->normal_texture_ref.uv_size.entries[1];
            }

            memcpy(&decal_tbo[sizeof(ShaderDecal) / sizeof(float) * decal_index], &decal, sizeof(ShaderDecal));

            unsigned int x_min = (unsigned int)decal_cluster_min.x();
            unsigned int x_max = (unsigned int)ceil(decal_cluster_max.x());

            unsigned int y_min = (unsigned int)decal_cluster_min.y();
            unsigned int y_max = (unsigned int)ceil(decal_cluster_max.y());

            // since z is "reversed" and min is actually the farthest value
            // we swap min and max
            unsigned int z_min = (unsigned int)(ZCLUSTERFUNC(decal_cluster_max.z(), z_near, z_mult));
            unsigned int z_max = (unsigned int)ceil(ZCLUSTERFUNC(decal_cluster_min.z(), z_near, z_mult));

            z_min = max(0u, z_min);
            z_max = min(z_max, num_z_clusters - 1);

            LOG_ASSERT_LTEQ(x_min, x_max);
            LOG_ASSERT_LTEQ(y_min, y_max);
            LOG_ASSERT_LTEQ(z_min, z_max);

            // these can be equal to the max since they're used in a C-style loop
            LOG_ASSERT_LTEQ(x_max, grid_width);
            LOG_ASSERT_LTEQ(y_max, grid_height);
            LOG_ASSERT_LTEQ(z_max, num_z_clusters);

            /*
            LOGI << "decal " << i << std::endl;
            LOGI << " min:  " << decal_cluster_min << std::endl;
            LOGI << " max:  " << decal_cluster_max << std::endl;
            LOGI << "  z:" << z_min << " - " << z_max << std::endl;
            LOGI << std::endl;
            */

            for (unsigned int x = x_min; x < x_max; x++) {
                for (unsigned int y = y_min; y < y_max; y++) {
                    for (unsigned int z = z_min; z < z_max; z++) {
                        // TODO: we might get better results with something like morton order
                        // it might increase cache locality in the shader
                        unsigned int decal_cluster_index = (y * grid_width + x) * num_z_clusters + z;
                        LOG_ASSERT_LT(decal_cluster_index, numclusters);

                        unsigned int count = cluster_decal_counts[decal_cluster_index];
                        cluster_decal_counts[decal_cluster_index]++;

                        // write new to 1d buffer and update 3d buffer
                        ClusterData new_data;
                        new_data.item = decal_index;
                        new_data.next = cluster_list_heads[decal_cluster_index];
                        cluster_list_heads[decal_cluster_index] = cluster_list_contents.size();
                        cluster_list_contents.push_back(new_data);
                    }
                }
            }
        }

        // fill lookup buffer
        // TODO: try to reuse buffer contents
        cluster_decals.clear();
        for (int unsigned i = 0; i < numclusters; i++) {
            unsigned int cluster_num_decals = 0;
            decal_grid_lookup[i] = SetIndex(0, cluster_decals.size());

            uint32_t index = cluster_list_heads[i];
            while (index != 0) {
                const ClusterData& data = cluster_list_contents[index];
                cluster_decals.push_back(data.item);
                cluster_num_decals++;
                index = data.next;
            }

            decal_grid_lookup[i] = SetCount(decal_grid_lookup[i], cluster_num_decals);
        }

        // LOGI << "decal buffer size: " << cluster_decals.size() << std::endl;
    }

    std::vector<DynamicLight>& dynamic_lights = dynamic_light_collection.dynamic_lights;
    std::vector<float> light_data_buffer;
    unsigned int num_alive_lights = 0;

    unsigned int num_lights = dynamic_lights.size();
    LOG_ASSERT_LT(num_lights, 65536);

    mat4 cluster_view_mat = cluster_mat * view_mat;

    LOG_ASSERT_EQ((sizeof(ShaderLight) % sizeof(float)), 0);
    const unsigned int light_params = sizeof(ShaderLight) / sizeof(float);
    light_data_buffer.resize(num_lights * light_params);
    ShaderLight l;
    l.padding = 0.0f;

    for (unsigned int i = 0; i < numclusters; i++) {
        cluster_light_counts[i] = 0;
    }

    memset(&cluster_list_heads[0], 0, sizeof(uint32_t) * cluster_list_heads.size());
    cluster_list_contents.clear();
    cluster_list_contents.push_back(ClusterData());

    for (unsigned int light_index = 0; light_index < num_lights; light_index++) {
        const DynamicLight& dl = dynamic_lights[light_index];
        const vec3 world_pos = dl.pos;
        vec3 view_pos = view_mat * world_pos;

        // TODO: don't use bounding box, there's a sphere algorithm in the paper

        // FIXME: not correct, doesn't account for projection
        vec3 view_min = view_pos - vec3(dl.radius * 0.5f);
        vec3 view_max = view_pos + vec3(dl.radius * 0.5f);

        // drop it if it's entirely behind the camera
        if (view_min.z() > -z_near) {
            continue;
        }

        // cap at near plane
        view_max.z() = min(-z_near, view_max.z());

        Box view_box;
        view_box.center = (view_min + view_max) * 0.5f;
        view_box.dims = view_max - view_min;

        // calculate projection space bounding box
        vec2 proj_min(FLT_MAX, FLT_MAX);
        vec2 proj_max(-FLT_MAX, -FLT_MAX);

        // TODO: we probably don't need to do the whole box, just the
        // four points on the near face
        for (int point = 0; point < Box::NUM_POINTS; point++) {
            vec4 proj_point = cluster_mat * vec4(view_box.GetPoint(point), 1.0f);
            // we know it's in front of the screen since we capped it at near plane
            LOG_ASSERT_GTEQ(proj_point.w(), 0.0f);
            vec2 proj_point2 = proj_point.xy() * (1.0f / proj_point.w());

            proj_min = components_min2(proj_min, proj_point2);
            proj_max = components_max2(proj_max, proj_point2);
        }

        /*
        LOGI << "proj_min: " << proj_min << std::endl;
        LOGI << "proj_max: " << proj_max << std::endl;
        */

        // we use projection-space .xy and view-space .z for clustering
        vec3 cluster_min(proj_min, view_min.z());
        vec3 cluster_max(proj_max, view_max.z());

        // drop the light if it's entirely off-screen
        if (cluster_min.x() > cluster_max_bound.x() || cluster_max.x() < cluster_min_bound.x() || cluster_min.y() > cluster_max_bound.y() || cluster_max.y() < cluster_min_bound.y()) {
            continue;
        }

        cluster_min = components_max(cluster_min, cluster_min_bound);
        cluster_max = components_max(cluster_max, cluster_min_bound);
        cluster_min = components_min(cluster_min, cluster_max_bound);
        cluster_max = components_min(cluster_max, cluster_max_bound);

        if (g_no_decal_elements) {
            continue;
        }
        /*
        LOGI << "cluster_min: " << cluster_min << std::endl;
        LOGI << "cluster_max: " << cluster_max << std::endl << std::endl;
        LOGI << "view_point " << point << ": " << view_point << std::endl;
        LOGI << "proj_point " << point << ": " << proj_point << std::endl;
        LOGI << "screen_point " << point << ": " << screen_point << std::endl;
        */

        unsigned int light_buf_index = num_alive_lights;
        num_alive_lights++;

        memcpy(l.pos, dl.pos.entries, 3 * sizeof(float));
        // the bounding box and helper mesh have a size of 0.5 instead of 1.0
        // so we divide the radius by 2 to make it look right
        l.radius = dl.radius / 2.0f;
        memcpy(l.color, dl.color.entries, 3 * sizeof(float));
        memcpy(&light_data_buffer[light_buf_index * light_params], &l, sizeof(ShaderLight));

        unsigned int x_min = (unsigned int)cluster_min.x();
        unsigned int x_max = (unsigned int)ceil(cluster_max.x());

        unsigned int y_min = (unsigned int)cluster_min.y();
        unsigned int y_max = (unsigned int)ceil(cluster_max.y());

        // since z is "reversed" and min is actually the farthest value
        // we swap min and max
        unsigned int z_min = (unsigned int)(ZCLUSTERFUNC(cluster_max.z(), z_near, z_mult));
        unsigned int z_max = (unsigned int)ceil(ZCLUSTERFUNC(cluster_min.z(), z_near, z_mult));

        z_min = max(0u, z_min);
        z_max = min(z_max, num_z_clusters - 1);

        LOG_ASSERT_LTEQ(x_min, x_max);
        LOG_ASSERT_LTEQ(y_min, y_max);
        LOG_ASSERT_LTEQ(z_min, z_max);

        // these can be equal to the max since they're used in a C-style loop
        LOG_ASSERT_LTEQ(x_max, grid_width);
        LOG_ASSERT_LTEQ(y_max, grid_height);
        LOG_ASSERT_LTEQ(z_max, num_z_clusters);

        /*
        LOGI << "decal " << i << std::endl;
        LOGI << " min:  " << cluster_min << std::endl;
        LOGI << " max:  " << cluster_max << std::endl;
        LOGI << "  z:" << z_min << " - " << z_max << std::endl;
        LOGI << std::endl;
        */

        for (unsigned int x = x_min; x < x_max; x++) {
            for (unsigned int y = y_min; y < y_max; y++) {
                for (unsigned int z = z_min; z < z_max; z++) {
                    unsigned int cluster_index = (y * grid_width + x) * num_z_clusters + z;
                    LOG_ASSERT_LT(cluster_index, numclusters);

                    unsigned int count = cluster_light_counts[cluster_index];
                    cluster_light_counts[cluster_index]++;

                    // write new to 1d buffer and update 3d buffer
                    ClusterData new_data;
                    new_data.item = light_buf_index;
                    new_data.next = cluster_list_heads[cluster_index];
                    cluster_list_heads[cluster_index] = cluster_list_contents.size();
                    cluster_list_contents.push_back(new_data);
                }
            }
        }
    }

    // LOGI << "num_alive_lights: " << num_alive_lights << std::endl;

    // fill the lookup buffer with data
    // TODO: try to reuse buffer contents
    cluster_lights.clear();
    for (int unsigned j = 0; j < numclusters; j++) {
        unsigned int cluster_num_lights = 0;
        light_grid_lookup[j] = SetIndex(0, cluster_lights.size());

        uint32_t index = cluster_list_heads[j];
        while (index != 0) {
            const ClusterData& data = cluster_list_contents[index];
            cluster_lights.push_back(data.item);
            cluster_num_lights++;
            index = data.next;
        }

        light_grid_lookup[j] = SetCount(light_grid_lookup[j], cluster_num_lights);
    }

    LOG_ASSERT_EQ(sizeof(ShaderDecal), (12 + 4 + 4 + 4) * 4);
    LOG_ASSERT_EQ(sizeof(ShaderClusterInfo), (8 * 4) * 4);

    {
        PROFILER_ENTER(g_profiler_ctx, "Setup batch data");

        clusterInfo.num_decals = num_alive_decals;
        clusterInfo.num_lights = num_alive_lights;

        unsigned int decal_grid_lookup_offset = 0;
        unsigned int decal_grid_lookup_size = numclusters * NUM_GRID_COMPONENTS;
        unsigned int cluster_decals_offset = decal_grid_lookup_size;
        unsigned int cluster_decals_size = cluster_decals.size();
        unsigned int light_cluster_data_offset = cluster_decals_offset + cluster_decals_size;
        unsigned int light_cluster_data_size = cluster_lights.size();

        const bool kBandwidthDisplay = false;
        if (kBandwidthDisplay) {
            const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "decal_grid_lookup_size: %u", decal_grid_lookup_size);
            Engine::Instance()->gui.AddDebugText("decal_grid_lookup_size", buf, 0.5f);
            FormatString(buf, kBufSize, "cluster_decals_size: %u", cluster_decals_size);
            Engine::Instance()->gui.AddDebugText("cluster_decals_size", buf, 0.5f);
            FormatString(buf, kBufSize, "light_cluster_data_size: %u", light_cluster_data_size);
            Engine::Instance()->gui.AddDebugText("light_cluster_data_size", buf, 0.5f);
        }

        unsigned int buf_size = decal_grid_lookup_size + cluster_decals.size() + cluster_lights.size();
        if (buf_size > decal_cluster_buffer.size()) {
            // resize the buffer
            decal_cluster_buffer.resize(buf_size, 0);
        }

        // first copy them into decal_cluster_buffer ...
        if (decal_grid_lookup_size > 0) {
            for (unsigned int i = 0; i < numclusters; i++) {
                unsigned int offs = decal_grid_lookup_offset + (NUM_GRID_COMPONENTS * i);
                memcpy(&decal_cluster_buffer[offs + 0], &decal_grid_lookup[i], 4);
                memcpy(&decal_cluster_buffer[offs + 1], &light_grid_lookup[i], 4);
            }
        }

        if (!cluster_decals.empty()) {
            memcpy(&decal_cluster_buffer[cluster_decals_offset], &cluster_decals[0], cluster_decals_size * 4);
        }

        if (!cluster_lights.empty()) {
            memcpy(&decal_cluster_buffer[light_cluster_data_offset], &cluster_lights[0], light_cluster_data_size * 4);
        }

        // light data and decal data in the same buffer, decal data first
        // light_data_offset is in floats because we're copying to float buffer...
        unsigned int light_data_offset = num_alive_decals * sizeof(ShaderDecal) / sizeof(float);
        unsigned int light_data_size = num_alive_lights * sizeof(ShaderLight) / sizeof(float);

        if (light_data_size > 0) {
            // make sure it fits
            if (decal_tbo.size() < light_data_offset + light_data_size) {
                decal_tbo.resize(light_data_offset + light_data_size, 0.0f);
            }

            // TODO: we could avoid this memcpy by packing the data in decal_tbo buffer
            // when looping over lights
            memcpy(&decal_tbo[light_data_offset], &light_data_buffer[0], light_data_size * 4);
        }

        PROFILER_LEAVE(g_profiler_ctx);

        PROFILER_ZONE(g_profiler_ctx, "Upload buffer data");
        // ...and then send it to the GPU
        if (kBandwidthDisplay) {
            const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "buf_size*4: %u", buf_size * 4);
            Engine::Instance()->gui.AddDebugText("buf_size", buf, 0.5f);
            FormatString(buf, kBufSize, "decal_tbo.size(): %u", decal_tbo.size());
            Engine::Instance()->gui.AddDebugText("decal_tbo.size()", buf, 0.5f);
            FormatString(buf, kBufSize, "sizeof(ShaderClusterInfo): %u", sizeof(ShaderClusterInfo));
            Engine::Instance()->gui.AddDebugText("sizeof(ShaderClusterInfo)", buf, 0.5f);
        }

        PrecisionStopwatch stop_watch;
        stop_watch.Start();
        Textures::Instance()->updateBufferTexture(decal_cluster_texture, buf_size * 4, &decal_cluster_buffer[0]);
        if (kBandwidthDisplay) {
            uint64_t time = stop_watch.StopAndReportNanoseconds();
            const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "decal_cluster_texture: %u us", time / 1000);
            Engine::Instance()->gui.AddDebugText("decal_cluster_texture", buf, 0.5f);
        }
        Textures::Instance()->updateBufferTexture(decal_data_texture, decal_tbo.size() * sizeof(float), &decal_tbo[0]);
        if (kBandwidthDisplay) {
            uint64_t time = stop_watch.StopAndReportNanoseconds();
            const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "decal_data_texture: %u us", time / 1000);
            Engine::Instance()->gui.AddDebugText("decal_data_texture", buf, 0.5f);
        }

        clusterInfo.light_cluster_data_offset = light_cluster_data_offset;
        // ... but here we divide by 4 since the shader samples from a vec4 buffer
        LOG_ASSERT_EQ((sizeof(ShaderDecal) % (sizeof(float) * 4)), 0);
        clusterInfo.light_data_offset = light_data_offset / 4;

        stop_watch.Start();
        {
            PROFILER_ZONE(g_profiler_ctx, "Cluster info buffer");
            if (uniform_ring_buffer.gl_id == -1) {
                uniform_ring_buffer.Create(128 * 1024);
            }
            uniform_ring_buffer.Fill(sizeof(ShaderClusterInfo), &clusterInfo);
        }
        if (kBandwidthDisplay) {
            uint64_t time = stop_watch.StopAndReportNanoseconds();
            const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "clusterInfo: %u us", time / 1000000);
            Engine::Instance()->gui.AddDebugText("clusterInfo", buf, 0.5f);
        }
        if (kBandwidthDisplay) {
            static GLint max_tbo_size = -1;
            if (max_tbo_size == -1) {
                glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_tbo_size);
            }
            static GLint max_ubo_size = -1;
            if (max_ubo_size == -1) {
                glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_ubo_size);
            }
            const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "GL_MAX_TEXTURE_BUFFER_SIZE: %d", max_tbo_size);
            Engine::Instance()->gui.AddDebugText("GL_MAX_TEXTURE_BUFFER_SIZE", buf, 0.5f);
            FormatString(buf, kBufSize, "GL_MAX_UNIFORM_BLOCK_SIZE: %d", max_ubo_size);
            Engine::Instance()->gui.AddDebugText("GL_MAX_UNIFORM_BLOCK_SIZE", buf, 0.5f);
        }
    }
}

void SceneGraph::BindDecals(int the_shader) {
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();

    int cluster_block_index = shaders->GetUBOBindIndex(the_shader, "ClusterInfo");

    if (cluster_block_index == (int)GL_INVALID_INDEX) {
        // no decals in this shader, skip the whole thing
        return;
    }

    // TODO: pass in decal info so they can be drawn in one pass
    // Pass in structure of all decals that overlap any instances of this type
    // Decal transform
    // Decal texture

    // Pass in number of intersecting decals for each instance
    // For each instance of envobject
    //     Pass in indices of each decal
    TextureRef decal_color_texture_ref;

    // Disabling normal texture as we've reached the texture limit.
    TextureRef decal_normal_texture_ref;

    decal_color_texture_ref = DecalTextures::Instance()->coloratlas->atlas_texture;
    if (decal_color_texture_ref.valid()) {
        textures->bindTexture(decal_color_texture_ref, TEX_DECAL_COLOR);
        shaders->SetUniformInt(shaders->GetTexUniform(TEX_DECAL_COLOR), TEX_DECAL_COLOR);
    }

    if (config["decal_normals"].toNumber<bool>()) {
        decal_normal_texture_ref = DecalTextures::Instance()->normalatlas->atlas_texture;
        if (decal_normal_texture_ref.valid()) {
            textures->bindTexture(decal_normal_texture_ref, TEX_DECAL_NORMAL);
            shaders->SetUniformInt(shaders->GetTexUniform(TEX_DECAL_NORMAL), TEX_DECAL_NORMAL);
        }
    }
}

void SceneGraph::BindLights(int the_shader) {
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();

    int cluster_block_index = shaders->GetUBOBindIndex(the_shader, "ClusterInfo");
    if (cluster_block_index == (int)GL_INVALID_INDEX) {
        // no decals in this shader, skip the whole thing
        return;
    }

    // Set up cluster parameters UBO
    {
        PROFILER_ENTER(g_profiler_ctx, "Setup decal parameters UBO");

        GLint block_size = shaders->returnShaderBlockSize(cluster_block_index, the_shader);

        LOG_ASSERT_LT(block_size, 16384);
        LOG_ASSERT_EQ(block_size, sizeof(ShaderClusterInfo));

        if (uniform_ring_buffer.gl_id != -1) {
            glBindBufferBase(GL_UNIFORM_BUFFER, UBO_CLUSTER_DATA, uniform_ring_buffer.gl_id);
            glBindBufferRange(GL_UNIFORM_BUFFER, UBO_CLUSTER_DATA, uniform_ring_buffer.gl_id, uniform_ring_buffer.offset, uniform_ring_buffer.next_offset - uniform_ring_buffer.offset);
        }
        glUniformBlockBinding(shaders->programs[the_shader].gl_program, cluster_block_index, UBO_CLUSTER_DATA);
        PROFILER_LEAVE(g_profiler_ctx);
    }

    shaders->SetUniformInt(shaders->GetTexUniform(TEX_LIGHT_DECAL_DATA_BUFFER), TEX_LIGHT_DECAL_DATA_BUFFER);
    shaders->SetUniformInt(shaders->GetTexUniform(TEX_CLUSTER_BUFFER), TEX_CLUSTER_BUFFER);

    textures->bindTexture(decal_data_texture, TEX_LIGHT_DECAL_DATA_BUFFER);
    textures->bindTexture(decal_cluster_texture, TEX_CLUSTER_BUFFER);
}

static int HexToVal(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    } else {
        return -1;
    }
}

vec3 ColorFromString(const char* str) {
    const char* char_ptr = str;
    int color_id = 0;
    int val = 0;
    vec3 color;
    char char_val = ' ';
    while (char_val != '\0') {
        char_val = *char_ptr;
        if (char_val >= '0' && char_val <= '9') {
            val *= 10;
            val += char_val - '0';
        } else if (char_val == ',' || char_val == '\0') {
            color[color_id] = val / 255.0f;
            ++color_id;
            val = 0;
        }
        ++char_ptr;
    }
    return color;
}

void SceneGraph::ApplyScriptParams(SceneGraph* scenegraph, const ScriptParamMap& spm) {
    scenegraph->level->SetScriptParams(spm);
    {
        ScriptParamMap::const_iterator iter = spm.find("Sky Rotation");
        if (iter != spm.end()) {
            const ScriptParam& sp = iter->second;
            float new_sky_rotation = sp.GetFloat();
            Sky* sky = scenegraph->sky;
            if (new_sky_rotation != sky->sky_rotation) {
                sky->sky_rotation = sp.GetFloat();
                sky->LightingChanged(scenegraph->terrain_object_ != NULL);
            }
        }
    }
    {
        ScriptParamMap::const_iterator iter = spm.find("Saturation");
        if (iter == spm.end()) {
            scenegraph->level->script_params().ASAddFloat("Saturation", 1.0f);
        }
    }
    {
        ScriptParamMap::const_iterator iter = spm.find("Sky Tint");
        ScriptParamMap::const_iterator iter2 = spm.find("Sky Brightness");
        if (iter != spm.end() && iter2 != spm.end()) {
            const ScriptParam& sp = iter->second;
            const std::string& new_sky_tint = sp.GetString();
            /*if(new_sky_tint.size() == 7 && new_sky_tint[0] == '#'){
                vec3 color;
                color[0] = (HexToVal(new_sky_tint[1])*16.0f + HexToVal(new_sky_tint[2])) / 255.0f;
                color[1] = (HexToVal(new_sky_tint[3])*16.0f + HexToVal(new_sky_tint[4])) / 255.0f;
                color[2] = (HexToVal(new_sky_tint[5])*16.0f + HexToVal(new_sky_tint[6])) / 255.0f;
                Sky* sky = scenegraph->sky;
                if(color != sky->sky_tint){
                    sky->sky_tint = color;
                    sky->LightingChanged(scenegraph->terrain_object_ != NULL);
                }
            }*/
            vec3 color = ColorFromString(new_sky_tint.c_str());
            color *= iter2->second.GetFloat();
            Sky* sky = scenegraph->sky;
            if (color != sky->sky_tint) {
                sky->sky_tint = color;
                sky->sky_base_tint = color;
                sky->LightingChanged(scenegraph->terrain_object_ != NULL);
            }
        }
    }
    {
        ScriptParamMap::const_iterator iter = spm.find("HDR White point");
        if (iter != spm.end()) {
            const ScriptParam& sp = iter->second;
            Graphics::Instance()->hdr_white_point = sp.GetFloat();
        }
    }
    {
        ScriptParamMap::const_iterator iter = spm.find("HDR Black point");
        if (iter != spm.end()) {
            const ScriptParam& sp = iter->second;
            Graphics::Instance()->hdr_black_point = sp.GetFloat();
        }
    }
    {
        ScriptParamMap::const_iterator iter = spm.find("HDR Bloom multiplier");
        if (iter != spm.end()) {
            const ScriptParam& sp = iter->second;
            Graphics::Instance()->hdr_bloom_mult = sp.GetFloat();
        }
    }
    {
        ScriptParamMap::const_iterator iter = spm.find("Fog amount");
        if (iter != spm.end()) {
            const ScriptParam& sp = iter->second;
            scenegraph->fog_amount = sp.GetFloat();
            scenegraph->haze_mult = 8.0f * (float)pow(10, scenegraph->fog_amount - 5.0f);
        }
    }
}

void SceneGraph::PrintCurrentObjects() {
    object_list::iterator objit = objects_.begin();

    LOGI << "Listing current objects in scene" << std::endl;
    for (; objit != objects_.end(); objit++) {
        LOGI << **objit << std::endl;
    }
}

int SceneGraph::CountObjectsWithName(const char* name) {
    int count = 0;
    object_list::iterator objit = objects_.begin();

    LOGI << "Listing current objects in scene" << std::endl;
    for (; objit != objects_.end(); objit++) {
        if (strmtch((*objit)->name, name)) {
            count++;
        }
    }
    return count;
}

bool SceneGraph::IsObjectSane(Object* obj) {
    if (obj == NULL) return false;

    for (auto& i : destruction_sanity) {
        if (i == obj) {
            return false;
        }
    }
    return true;
}

const char* SceneGraph::GetDestroyedObjectInfo(int object_id) {
    if (object_id >= 0) {
        for (unsigned i = 0; i < destruction_memory_size; i++) {
            if (destruction_memory_ids[i] == object_id) {
                return &destruction_memory_strings[i * destruction_memory_string_size];
            }
        }
    }
    return "";
}

void SceneGraph::DumpState() {
    std::ofstream output;
    char dump_path[kPathSize];
    GetScenGraphDumpPath(dump_path, kPathSize);

    my_ofstream_open(output, dump_path);

    if (output.is_open()) {
        LOGI << "Dumping all objects in scenegraph into " << dump_path << std::endl;
        for (auto& object : objects_) {
            if (output.good()) {
                output << "[" << object->GetID() << "]: \"" << CStringFromEntityType(object->GetType()) << "\" " << object->GetName() << std::endl;
            }
        }
    } else {
        LOGE << "Failed at opening " << dump_path << " for dumping. " << std::endl;
    }
}

void SceneGraph::GetParticleShaderNames(std::map<std::string, int>& preload_shaders) {
    Path p = FindFilePath("Data/shader_preload.xml", kDataPaths, true);
    if (!p.isValid()) {
        LOGW << "Couldn't load " << p.GetOriginalPath() << std::endl;
        return;
    }

    if (!CheckFileAccess(p.GetFullPath())) {
        LOGW << "Couldn't access " << p.GetFullPath() << std::endl;
        return;
    }

    TiXmlDocument doc;
    if (!doc.LoadFile(p.GetFullPath())) {
        LOGE << "Couldn't load " << p.GetFullPath() << ": " << doc.ErrorDesc() << " on row " << doc.ErrorRow() << std::endl;
        return;
    }

    TiXmlElement* root = doc.RootElement();
    if (root && strcmp(root->Value(), "Shaders") == 0) {
        root = root->FirstChildElement();
    } else {
        LOGE << p.GetFullPath() << " root element null or not \"Shaders\"" << std::endl;
        return;
    }

    for (const TiXmlElement* field = root; field; field = field->NextSiblingElement()) {
        const char* field_value = field->Value();

        if (strcmp(field_value, "Shader") == 0) {
            const char* attribute = field->Attribute("name");
            const char* suffix = field->Attribute("suffix");
            const char* optional = field->Attribute("optional");

            int draw_type = 0;
            if (suffix) {
                draw_type = (strcmp(suffix, "true") == 0) ? kFullDraw : 0;
            }
            if (optional) {
                if (strcmp(optional, "none") == 0) {
                    draw_type |= kOptionalNone;
                } else if (strcmp(optional, "geometry") == 0) {
                    draw_type |= kOptionalGeometry;
                } else if (strcmp(optional, "tessellation") == 0) {
                    draw_type |= kOptionalTessellation;
                }
            }

            if (attribute) {
                preload_shaders[attribute] = draw_type;
            } else {
                LOGE << "No name attribute found when parsing " << p.GetFullPath() << std::endl;
            }
        }
    }
}

void SceneGraph::PreloadForDrawType(std::map<std::string, int>& preload_shaders, PreloadType type) {
    Shaders* shaders = Shaders::Instance();
    Object::DrawType draw_type;
    switch (type) {
        case kDrawDepthOnly:
            draw_type = Object::kDrawDepthOnly;
            break;
        case kDrawAllShadowCascades:
            draw_type = Object::kDrawAllShadowCascades;
            break;
        case kDrawDepthNoAA:
            draw_type = Object::kDrawDepthNoAA;
            break;
        case kFullDraw:
            draw_type = Object::kFullDraw;
            break;
        case kWireframe:
            draw_type = Object::kWireframe;
            break;
        case kDecal:
            draw_type = Object::kDecal;
            break;
        default:
            return;
    }

    UpdateShaderSuffix(this, draw_type);
    for (auto& preload_shader : preload_shaders) {
        if (preload_shader.second & type) {
            const int kShaderStrSize = 1024;
            char buf[kShaderStrSize];
            FormatString(buf, kShaderStrSize, "%s %s", preload_shader.first.c_str(), global_shader_suffix);
            Shaders::OptionalShaders optional = Shaders::kNone;
            if (preload_shader.second & kOptionalGeometry)
                optional = Shaders::kGeometry;
            else if (preload_shader.second & kOptionalTessellation)
                optional = Shaders::kTesselation;
            shaders->createProgram(shaders->returnProgram(buf, optional));
        }
    }
}

void SceneGraph::PreloadShaders() {
    LOGI << "Preloading shaders..." << std::endl;
    PROFILER_ZONE(g_profiler_ctx, "Preload shaders");
    // There is no easy way to call DetailObjectSurface::GetShaderNames, so do it here for now
    const char* detail_object_names[6] =
        {
            "envobject #DETAIL_OBJECT", "envobject #DETAIL_OBJECT #PLANT", "envobject #DETAIL_OBJECT #PLANT #LESS_PLANT_MOVEMENT", "envobject #DETAIL_OBJECT #TERRAIN", "envobject #DETAIL_OBJECT #TERRAIN #PLANT", "envobject #DETAIL_OBJECT #TERRAIN #PLANT #LESS_PLANT_MOVEMENT"};

    for (auto& detail_object_name : detail_object_names) {
        const int kShaderStrSize = 1024;
        char buf[2][kShaderStrSize];
        char* shader_str[2] = {buf[0], buf[1]};

        FormatString(shader_str[0], kShaderStrSize, "%s", detail_object_name);
        if (!Graphics::Instance()->config_.detail_object_decals()) {
            FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], "#NO_DECALS");
            std::swap(shader_str[0], shader_str[1]);
        }
        if (!Graphics::Instance()->config_.detail_object_shadows()) {
            FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], "#NO_DETAIL_OBJECT_SHADOWS");
            std::swap(shader_str[0], shader_str[1]);
        }
        preload_shaders[shader_str[0]] = kFullDraw | kDrawDepthOnly;
    }

    GetParticleShaderNames(preload_shaders);
    Engine::Instance()->GetShaderNames(preload_shaders);
    Graphics::Instance()->GetShaderNames(preload_shaders);
    sky->GetShaderNames(preload_shaders);
    flares.GetShaderNames(preload_shaders);
    const PreloadType preload_types[6] = {
        kDrawDepthOnly, kDrawAllShadowCascades, kDrawDepthNoAA, kFullDraw, kWireframe, kDecal};
    for (auto preload_type : preload_types)
        PreloadForDrawType(preload_shaders, preload_type);

    // Load everything that isn't any of the above types
    Shaders* shaders = Shaders::Instance();
    for (auto& preload_shader : preload_shaders) {
        if ((preload_shader.second & kPreloadTypeAll) == 0) {
            Shaders::OptionalShaders optional = Shaders::kNone;
            if (preload_shader.second & kOptionalGeometry)
                optional = Shaders::kGeometry;
            else if (preload_shader.second & kOptionalTessellation)
                optional = Shaders::kTesselation;
            shaders->createProgram(shaders->returnProgram(preload_shader.first, optional));
        }
    }
}

void SceneGraph::LoadReflectionCaptureCubemaps() {
    PROFILER_ZONE(g_profiler_ctx, "Load reflection capture cubemaps");
    for (auto obj : objects_) {
        if (obj->GetType() == _reflection_capture_object) {
            char save_path[kPathSize];
            FormatString(save_path, kPathSize, "%s_refl_cap_%d.hdrcube", level_path_.GetOriginalPath(), obj->GetID());
            char abs_path[kPathSize];
            if (FindFilePath(save_path, abs_path, kPathSize, kAnyPath, false) != -1) {
                ReflectionCaptureObject* rco = (ReflectionCaptureObject*)obj;
                rco->cube_map_ref = Textures::LoadCubeMapMipmapsHDR(abs_path);  // TODO: if no_reflection_capture is true, only the global capture cube should be loaded.
                if (rco->cube_map_ref.valid()) {
                    rco->dirty = false;
                    if (rco->GetScriptParams()->ASGetInt("Global") == 1) {
                        sky->SetSpecularCubeMapTexture(rco->cube_map_ref);
                    }
                } else {
                    rco->dirty = true;
                }
            }
        }
    }
    reflection_data_loaded = true;
}

// TODO: make a version that only clears the dynamic cubemaps.
void SceneGraph::UnloadReflectionCaptureCubemaps() {
    for (auto obj : objects_) {
        if (obj->GetType() == _reflection_capture_object) {
            ReflectionCaptureObject* rco = (ReflectionCaptureObject*)obj;
            rco->cube_map_ref.clear();
            rco->dirty = true;
        }
    }

    sky->ResetSpecularCubeMapTexture();
    reflection_data_loaded = false;
}
