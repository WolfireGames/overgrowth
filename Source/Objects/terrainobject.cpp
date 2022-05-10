//-----------------------------------------------------------------------------
//           Name: terrainobject.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The terrain object is an entity representing some terrain
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
#include "terrainobject.h"

#include <Graphics/camera.h>
#include <Graphics/graphics.h>
#include <Graphics/textures.h>
#include <Graphics/shaders.h>
#include <Graphics/sky.h>

#include <Physics/bulletworld.h>
#include <Physics/bulletobject.h>

#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>
#include <Internal/timer.h>

#include <Main/engine.h>
#include <Main/scenegraph.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Compat/fileio.h>
#include <Math/vec3math.h>
#include <Objects/lightvolume.h>
#include <Wrappers/glm.h>
#include <Utility/compiler_macros.h>

extern bool g_simple_shadows;
extern bool g_level_shadows;
extern char* global_shader_suffix;
extern bool g_no_decals;
extern Timer game_timer;
extern bool g_no_reflection_capture;

extern bool g_debug_runtime_disable_terrain_object_draw_depth_map;
extern bool g_debug_runtime_disable_terrain_object_draw_terrain;
extern bool g_debug_runtime_disable_terrain_object_pre_draw_camera;

static void DrawPatchModel(Model& model, Graphics* graphics, Shaders* shaders, int shader, bool use_tesselation) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "DrawPatchModel");
    if (!model.vbo_loaded) {
        PROFILER_ZONE(g_profiler_ctx, "createVBO");
        model.createVBO();
    }
    model.VBO_faces.Bind();
    int attrib_ids[4];
    for (int i = 0; i < 4; ++i) {
        const char* attrib_str;
        int num_el;
        VBOContainer* vbo;
        switch (i) {
            case 0:
                attrib_str = "vertex_attrib";
                num_el = 3;
                vbo = &model.VBO_vertices;
                break;
            case 1:
                attrib_str = "tangent_attrib";
                num_el = 3;
                vbo = &model.VBO_tangents;
                break;
            case 2:
                attrib_str = "tex_coord_attrib";
                num_el = 2;
                vbo = &model.VBO_tex_coords;
                break;
            case 3:
                attrib_str = "detail_tex_coord";
                num_el = 2;
                vbo = &model.VBO_tex_coords2;
                break;
            default:
                __builtin_unreachable();
                break;
        }
        CHECK_GL_ERROR();
        vbo->Bind();
        CHECK_GL_ERROR();
        attrib_ids[i] = shaders->returnShaderAttrib(attrib_str, shader);
        if (attrib_ids[i] != -1) {
            CHECK_GL_ERROR();
            graphics->EnableVertexAttribArray(attrib_ids[i]);
            CHECK_GL_ERROR();
            glVertexAttribPointer(attrib_ids[i], num_el, GL_FLOAT, false, num_el * sizeof(GLfloat), 0);
            CHECK_GL_ERROR();
        }
    }
    CHECK_GL_ERROR();
    graphics->DrawElements(use_tesselation ? GL_PATCHES : GL_TRIANGLES, (unsigned int)model.faces.size(), GL_UNSIGNED_INT, 0);
    CHECK_GL_ERROR();
    graphics->ResetVertexAttribArrays();
    CHECK_GL_ERROR();
    graphics->BindArrayVBO(0);
    graphics->BindElementVBO(0);
}

// Check for collision with a line
int TerrainObject::lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal) {
    if (bullet_object_) {
        bool hit = scenegraph_->bullet_world_->CheckRayCollisionObj(
                       start, end, *bullet_object_, point, normal) != -1;
        if (hit) {
            return 0;
        } else {
            return -1;
        }
    }
    return -1;
}

void TerrainObject::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    switch (type) {
        case OBJECT_MSG::LIGHTING_CHANGED:
            terrain_.BakeTerrainTexture(terrain_.framebuffer, scenegraph_->sky->GetSpecularCubeMapTexture());
            break;
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void TerrainObject::PreDrawFrame(float curr_game_time) {
    CHECK_GL_ERROR();
    terrain_.GLInit(scenegraph_->sky);
    CHECK_GL_ERROR();
    if (scenegraph_->sky->live_updated) {
        CHECK_GL_ERROR();
        terrain_.BakeTerrainTexture(terrain_.framebuffer, scenegraph_->sky->GetSpecularCubeMapTexture());
        scenegraph_->sky->BakeSecondPass(&terrain_.baked_texture_ref);
        scenegraph_->sky->live_updated = false;
        CHECK_GL_ERROR();
    }
}

void TerrainObject::PreDrawCamera(float curr_game_time) {
    if (g_debug_runtime_disable_terrain_object_pre_draw_camera) {
        return;
    }

    static const mat4 identity;
    for (auto& detail_object_surface : terrain_.detail_object_surfaces) {
        detail_object_surface->PreDrawCamera(identity);
    }
}

// Draw terrain
void TerrainObject::Draw() {
    if (Graphics::Instance()->queued_screenshot && Graphics::Instance()->screenshot_mode == Graphics::kTransparentGameplay) {
        return;
    }
    DrawTerrain();

    if (!terrain_info_.minimal) {
        static const mat4 identity;
        for (auto& detail_object_surface : terrain_.detail_object_surfaces) {
            detail_object_surface->Draw(identity, DetailObjectSurface::TERRAIN, vec3(1.0f), scenegraph_->sky->GetSpecularCubeMapTexture(), &scenegraph_->light_probe_collection, scenegraph_);
        }
    }
}

bool CheckBoxAgainstPlanes(const vec3& start, const vec3& end,
                           const vec4* cull_planes, int num_cull_planes) {
    vec3 point;
    for (int p = 0; p < num_cull_planes; ++p) {
        int in_count = 8;
        for (int i = 0; i < 8; ++i) {
            if (i < 4) {
                point[0] = start[0];
            } else {
                point[0] = end[0];
            }
            if (i % 4 < 2) {
                point[1] = start[1];
            } else {
                point[1] = end[1];
            }
            if (i % 2 == 0) {
                point[2] = start[2];
            } else {
                point[2] = end[2];
            }
            // test this point against the planes
            if (cull_planes[p][0] * point[0] +
                    cull_planes[p][1] * point[1] +
                    cull_planes[p][2] * point[2] +
                    cull_planes[p][3] <
                0) {
                in_count--;
            }
        }

        // were all the points outside of plane p?
        if (in_count == 0) {
            return false;
        }
    }
    return true;
}

void TerrainObject::DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type) {
    if (g_debug_runtime_disable_terrain_object_draw_depth_map) {
        return;
    }

    if (terrain_info_.minimal || preview_mode)
        return;
    PROFILER_GPU_ZONE(g_profiler_ctx, "TerrainObject::DrawDepthMap");
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    // Textures* textures = Textures::Instance();
    // Camera* cam = ActiveCameras::Get();
    graphics->setGLState(gl_state_);
    mat4 t = GetTransform();  // this seems to always be identity
    vec4 test(1.0, 1.0, 1.0, 1.0);
    LOG_ASSERT(t * test == test);

    bool use_tesselation = false;  // GLEW_ARB_tessellation_shader;

    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    FormatString(shader_str[0], kShaderStrSize, "envobject #TERRAIN #DEPTH_ONLY");
    if (draw_type == kDrawAllShadowCascades) {
        FormatString(shader_str[1], kShaderStrSize, "%s #SHADOW_CASCADE", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (use_tesselation) {
        FormatString(shader_str[1], kShaderStrSize, "%s #FRACTAL_DISPLACE", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    int shader = shaders->returnProgram(shader_str[0], use_tesselation ? Shaders::kTesselation : Shaders::kNone);

    shaders->setProgram(shader);
    shaders->SetUniformMat4("projection_view_mat", proj_view_matrix);
    shaders->SetUniformFloat("time", game_timer.GetRenderTime());
    shaders->SetUniformVec3("cam_pos", ActiveCameras::Get()->GetPos());
    const vec3 translation = GetTranslation();

    int vert_attrib_id = shaders->returnShaderAttrib("vertex_attrib", shader);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    for (auto& patch : terrain_.terrain_patches) {
        const vec3 adjusted_min = patch.min_coords + translation;
        const vec3 adjusted_max = patch.max_coords + translation;
        if (CheckBoxAgainstPlanes(adjusted_min, adjusted_max, cull_planes, num_cull_planes)) {
            if (!patch.vbo_loaded) {
                patch.createVBO();
            }
            patch.VBO_faces.Bind();
            patch.VBO_vertices.Bind();
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3 * sizeof(GLfloat), 0);
            graphics->DrawElements(use_tesselation ? GL_PATCHES : GL_TRIANGLES, (unsigned int)patch.faces.size(), GL_UNSIGNED_INT, 0);
        }
    }
    for (auto& patch : terrain_.edge_terrain_patches) {
        const vec3 adjusted_min = patch.min_coords + translation;
        const vec3 adjusted_max = patch.max_coords + translation;
        if (CheckBoxAgainstPlanes(adjusted_min, adjusted_max, cull_planes, num_cull_planes)) {
            if (!patch.vbo_loaded) {
                patch.createVBO();
            }
            patch.VBO_faces.Bind();
            patch.VBO_vertices.Bind();
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3 * sizeof(GLfloat), 0);
            graphics->DrawElements(use_tesselation ? GL_PATCHES : GL_TRIANGLES, (unsigned int)patch.faces.size(), GL_UNSIGNED_INT, 0);
        }
    }
    graphics->ResetVertexAttribArrays();
}

bool TerrainObject::Initialize() {
    terrain_.level_name = scenegraph_->level_name_;
    PROFILER_ZONE(g_profiler_ctx, "CalcDetailTextures");
    terrain_.CalcDetailTextures();
    return true;
}

void TerrainObject::GetShaderNames(std::map<std::string, int>& preload_shaders) {
    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    FormatString(shader_str[0], kShaderStrSize, "envobject #TERRAIN #DETAILMAP4 %s", shader_extra.c_str());
    preload_shaders[shader_str[0]] = SceneGraph::kPreloadTypeAll;
    preload_shaders["envobject #TERRAIN #DEPTH_ONLY"] = 0;
    preload_shaders["envobject #TERRAIN #DEPTH_ONLY #SHADOW_CASCADE"] = 0;

    terrain_.GetShaderNames(preload_shaders);
}

TerrainObject::TerrainObject(const TerrainInfo& _terrain_info) : bullet_object_(NULL),
                                                                 preview_mode(false) {
    shader_extra = _terrain_info.shader_extra;
    terrain_info_ = _terrain_info;
    added_to_physics_scene_ = false;
    terrain_.SetColorTexture(terrain_info_.colormap.c_str());
    Textures::Instance()->setWrap(GL_REPEAT);
    terrain_.weight_perturb_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/weight_perturb.tga");
    terrain_.SetDetailTextures(terrain_info_.detail_map_info);
    terrain_.SetWeightTexture(terrain_info_.weightmap.c_str());
    if (_terrain_info.minimal)
        terrain_.LoadMinimal(terrain_info_.heightmap.c_str(), terrain_info_.model_override);
    else
        terrain_.Load(terrain_info_.heightmap.c_str(), terrain_info_.model_override);
    terrain_.SetDetailObjectLayers(terrain_info_.detail_object_info);
    collidable = true;
    transparent = false;
    gl_state_.depth_test = true;
    gl_state_.cull_face = true;
    gl_state_.depth_write = true;
    gl_state_.blend = false;
    edge_gl_state_.depth_test = true;
    edge_gl_state_.cull_face = true;
    edge_gl_state_.depth_write = true;
    edge_gl_state_.blend = true;
    permission_flags = 0;
    exclude_from_undo = true;
    exclude_from_save = true;
}

TerrainObject::~TerrainObject() {
    if (added_to_physics_scene_)
        scenegraph_->bullet_world_->RemoveStaticObject(&bullet_object_);
}

void TerrainObject::PreparePhysicsMesh() {
    static const bool kCachePhysicsShape = false;
    if (!added_to_physics_scene_) {
        btBvhTriangleMeshShape* cached_shape = NULL;
        const char* cache_path = "terrain_serialize.bullet";
        if (kCachePhysicsShape) {
            /*char abs_path[kPathSize];
            if(FindFilePath(cache_path, abs_path, kPathSize, kDataPaths|kModPaths|kWriteDir|kModWriteDirs) != -1) {
                btBulletWorldImporter import(0);//don't store info into the world
                PROFILER_ZONE(g_profiler_ctx, "Loading cache file");
                if (import.loadFile(abs_path)){
                    int numShape = import.getNumCollisionShapes();
                    if (numShape) {
                        cached_shape = (btBvhTriangleMeshShape*)import.getCollisionShapeByIndex(0);
                    }
                }
            }*/
        }
        if (cached_shape) {
            PROFILER_ZONE(g_profiler_ctx, "Adding cached shape to physics scene");
            SharedShapePtr shape;
            shape.reset(cached_shape);
            BulletObject* obj = scenegraph_->bullet_world_->CreateRigidBody(shape, 0.0f, BW_NO_FLAGS);
            obj->body->setCollisionFlags(obj->body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
            bullet_object_ = obj;
        } else {
            PROFILER_ZONE(g_profiler_ctx, "Creating new static mesh");
            bullet_object_ = scenegraph_->bullet_world_->CreateStaticMesh(&terrain_.GetModel(), -1, BW_NO_FLAGS);

            if (kCachePhysicsShape) {
                /*PROFILER_ZONE(g_profiler_ctx, "Serializing cached mesh to disk");
                int maxSerializeBufferSize = 1024*1024*10;
                btDefaultSerializer*    serializer = new btDefaultSerializer(maxSerializeBufferSize);

                serializer->startSerialization();
                bullet_object_->shape->serializeSingleShape(serializer);
                serializer->finishSerialization();

                char path[kPathSize];
                FormatString(path, kPathSize, "%s%s", GetWritePath(CoreGameModID).c_str(), cache_path);
                FILE* file = my_fopen(path, "wb");
                fwrite(serializer->getBufferPointer(),serializer->getCurrentBufferSize(),1, file);
                fclose(file);

                delete serializer;*/
            }
        }
        bullet_object_->owner_object = this;
        added_to_physics_scene_ = true;
    }
}

const Model* TerrainObject::GetModel() const {
    return &terrain_.GetModel();
}

void TerrainObject::HandleMaterialEvent(const std::string& the_event, const vec3& event_pos, int* tri) {
    terrain_.HandleMaterialEvent(the_event, event_pos, tri);
}

const MaterialEvent& TerrainObject::GetMaterialEvent(const std::string& the_event, const vec3& event_pos, int* tri) {
    MaterialRef material_ref = terrain_.GetMaterialAtPoint(event_pos, tri);
    return material_ref->GetEvent(the_event);
}

const MaterialEvent& TerrainObject::GetMaterialEvent(const std::string& the_event, const vec3& event_pos, const std::string& mod, int* tri) {
    MaterialRef material_ref = terrain_.GetMaterialAtPoint(event_pos, tri);
    return material_ref->GetEvent(the_event, mod);
}

const MaterialDecal& TerrainObject::GetMaterialDecal(const std::string& type, const vec3& pos, int* tri) {
    MaterialRef material_ref = terrain_.GetMaterialAtPoint(pos, tri);
    return material_ref->GetDecal(type);
}

const MaterialParticle& TerrainObject::GetMaterialParticle(const std::string& type, const vec3& pos, int* tri) {
    MaterialRef material_ref = terrain_.GetMaterialAtPoint(pos, tri);
    return material_ref->GetParticle(type);
}

void TerrainObject::GetDisplayName(char* buf, int buf_size) {
    if (GetName().empty()) {
        FormatString(buf, buf_size, "%d: Terrain: %s", GetID(), terrain_info_.heightmap.c_str());
    } else {
        FormatString(buf, buf_size, "%s: Terrain: %s", GetName().c_str(), terrain_info_.heightmap.c_str());
    }
}

vec3 TerrainObject::GetColorAtPoint(const vec3& pos, int* tri) {
    return terrain_.SampleColorMapAtPoint(pos, tri);
}

MaterialRef TerrainObject::GetMaterial(const vec3& pos, int* tri) {
    return terrain_.GetMaterialAtPoint(pos, tri);
}

void TerrainObject::DrawTerrain() {
    if (g_debug_runtime_disable_terrain_object_draw_terrain) {
        return;
    }

    Graphics* graphics = Graphics::Instance();
    graphics->setGLState(gl_state_);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();
    Camera* cam = ActiveCameras::Get();

    mat4 projection_view_mat = cam->GetProjMatrix() * cam->GetViewMatrix();

    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};

    // Tessellation is ignored when preloading shaders because the use-case is unclear.
    // Where is it set? Is it per-object? Is it set once?
    bool use_tesselation = false;  // GLEW_ARB_tessellation_shader;

    FormatString(shader_str[0], kShaderStrSize, "envobject #TERRAIN #DETAILMAP4 %s %s", shader_extra.c_str(), global_shader_suffix);
    if (preview_mode) {
        FormatString(shader_str[1], kShaderStrSize, "%s #HALFTONE_STIPPLE", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if (use_tesselation) {
        FormatString(shader_str[1], kShaderStrSize, "%s #FRACTAL_DISPLACE", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    int shader = shaders->returnProgram(shader_str[0], use_tesselation ? Shaders::kTesselation : Shaders::kNone);

    shaders->setProgram(shader);

    vec4 temp(0.0f, 1.0f, 2.0f, 3.0f);
    shaders->SetUniformVec4("detail_color_indices", temp);

    textures->bindTexture(terrain_.detail_texture_weights, 5);
    textures->bindTexture(terrain_.color_texture_ref, 0);
    textures->bindTexture(scenegraph_->sky->GetSpecularCubeMapTexture(), 2);
    if (terrain_.normal_map_ref.valid()) {
        textures->bindTexture(terrain_.normal_map_ref, 1);
    } else {
        textures->bindBlankNormalTexture(1);
    }
    if (g_simple_shadows || !g_level_shadows) {
        textures->bindTexture(graphics->static_shadow_depth_ref, 4);
    } else {
        textures->bindTexture(graphics->cascade_shadow_depth_ref, 4);
    }
    textures->bindTexture(terrain_.detail_texture_ref, 6);
    textures->bindTexture(terrain_.detail_normal_texture_ref, 7);
    textures->bindTexture(terrain_.weight_perturb_ref, 14);
    if (scenegraph_->light_probe_collection.light_volume_enabled && scenegraph_->light_probe_collection.ambient_3d_tex.valid()) {
        textures->bindTexture(scenegraph_->light_probe_collection.ambient_3d_tex, 16);
    }
    textures->bindTexture(graphics->screen_color_tex, 17);
    textures->bindTexture(graphics->screen_depth_tex, 18);

    shaders->SetUniformInt("weight_component", 0);
    shaders->SetUniformVec4("avg_color0", terrain_.detail_texture_color_srgb[0]);
    shaders->SetUniformVec4("avg_color1", terrain_.detail_texture_color_srgb[1]);
    shaders->SetUniformVec4("avg_color2", terrain_.detail_texture_color_srgb[2]);
    shaders->SetUniformVec4("avg_color3", terrain_.detail_texture_color_srgb[3]);
    shaders->SetUniformVec3("ws_light", scenegraph_->primary_light.pos);
    shaders->SetUniformVec4("primary_light_color", vec4(scenegraph_->primary_light.color, scenegraph_->primary_light.intensity));
    shaders->SetUniformFloat("time", game_timer.GetRenderTime());
    shaders->SetUniformVec3("cam_pos", cam->GetPos());
    shaders->SetUniformMat4("projection_view_mat", projection_view_mat);
    shaders->SetUniformFloat("haze_mult", scenegraph_->haze_mult);
    std::vector<mat4> shadow_matrix;
    shadow_matrix.resize(4);
    for (int i = 0; i < 4; ++i) {
        shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
    }
    if (g_simple_shadows || !g_level_shadows) {
        shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
    }

    shaders->SetUniformInt("reflection_capture_num", (int)scenegraph_->ref_cap_matrix.size());
    if (!scenegraph_->ref_cap_matrix.empty()) {
        assert(!scenegraph_->ref_cap_matrix_inverse.empty());
        shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph_->ref_cap_matrix);
        shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph_->ref_cap_matrix_inverse);
    }

    std::vector<mat4> light_volume_matrix;
    std::vector<mat4> light_volume_matrix_inverse;
    for (auto obj : scenegraph_->light_volume_objects_) {
        const mat4& mat = obj->GetTransform();
        light_volume_matrix.push_back(mat);
        light_volume_matrix_inverse.push_back(invert(mat));
    }
    shaders->SetUniformInt("light_volume_num", (int)light_volume_matrix.size());
    if (!light_volume_matrix.empty()) {
        assert(!light_volume_matrix_inverse.empty());
        shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
        shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
    }

    if (g_no_reflection_capture == false) {
        textures->bindTexture(scenegraph_->cubemaps, 19);
    }

    shaders->SetUniformInt("num_tetrahedra", scenegraph_->light_probe_collection.ShaderNumTetrahedra());
    shaders->SetUniformInt("num_light_probes", scenegraph_->light_probe_collection.ShaderNumLightProbes());
    shaders->SetUniformVec3("grid_bounds_min", scenegraph_->light_probe_collection.grid_lookup.bounds[0]);
    shaders->SetUniformVec3("grid_bounds_max", scenegraph_->light_probe_collection.grid_lookup.bounds[1]);
    shaders->SetUniformInt("subdivisions_x", scenegraph_->light_probe_collection.grid_lookup.subdivisions[0]);
    shaders->SetUniformInt("subdivisions_y", scenegraph_->light_probe_collection.grid_lookup.subdivisions[1]);
    shaders->SetUniformInt("subdivisions_z", scenegraph_->light_probe_collection.grid_lookup.subdivisions[2]);
    shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
    shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
    if (scenegraph_->light_probe_collection.light_probe_buffer_object_id != -1) {
        glBindBuffer(GL_TEXTURE_BUFFER, scenegraph_->light_probe_collection.light_probe_buffer_object_id);
    }

    scenegraph_->BindDecals(shader);
    scenegraph_->BindLights(shader);
    shaders->SetUniformMat4Array("shadow_matrix", shadow_matrix);
    const vec3 translation = GetTranslation();
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw opaque terrain patches");
        for (auto& patch : terrain_.terrain_patches) {
            const vec3 adjusted_min = patch.min_coords + translation;
            const vec3 adjusted_max = patch.max_coords + translation;
            if (ActiveCameras::Get()->checkBoxInFrustum(adjusted_min,
                                                        adjusted_max)) {
                DrawPatchModel(patch, graphics, shaders, shader, use_tesselation);
            }
        }
    }
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw edge terrain patches");
        graphics->setGLState(edge_gl_state_);
        for (auto& patch : terrain_.edge_terrain_patches) {
            const vec3 adjusted_min = patch.min_coords + translation;
            const vec3 adjusted_max = patch.max_coords + translation;
            if (ActiveCameras::Get()->checkBoxInFrustum(adjusted_min,
                                                        adjusted_max)) {
                DrawPatchModel(patch, graphics, shaders, shader, use_tesselation);
            }
        }
    }
}

const TerrainInfo& TerrainObject::terrain_info() const {
    return terrain_info_;
}

void TerrainObject::SetTerrainColorTexture(const char* path) {
    terrain_.SetColorTexture(path);
    terrain_info_.colormap = path;
}

void TerrainObject::SetTerrainWeightTexture(const char* path) {
    terrain_.SetWeightTexture(path);
    terrain_info_.weightmap = path;
}

void TerrainObject::SetTerrainDetailTextures(const std::vector<DetailMapInfo>& detail_map_info) {
    terrain_.SetDetailTextures(detail_map_info);
    terrain_.CalcDetailTextures();
    terrain_info_.detail_map_info = detail_map_info;
}
