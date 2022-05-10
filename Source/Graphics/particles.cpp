//-----------------------------------------------------------------------------
//           Name: particles.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This class handles particle animation and rendering
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
#include "particles.h"

#include <Graphics/graphics.h>
#include <Graphics/camera.h>
#include <Graphics/textures.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/shaders.h>

#include <Internal/common.h>
#include <Internal/timer.h>
#include <Internal/profiler.h>

#include <Objects/movementobject.h>
#include <Objects/decalobject.h>
#include <Objects/lightvolume.h>

#include <Physics/physics.h>
#include <Physics/bulletworld.h>

#include <Math/enginemath.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Online/online_datastructures.h>
#include <Main/scenegraph.h>
#include <XML/xml_helper.h>
#include <Sound/sound.h>
#include <Asset/Asset/material.h>
#include <Graphics/sky.h>
#include <Editors/actors_editor.h>
#include <Main/engine.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <tinyxml.h>

#include <cassert>

extern std::string script_dir_path;
extern Timer game_timer;
extern bool g_no_reflection_capture;
extern bool g_debug_runtime_disable_gpu_particle_field_draw;
extern bool g_debug_runtime_disable_particle_draw;
extern bool g_debug_runtime_disable_particle_system_draw;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

bool CParticleCompare(Particle* p1, Particle* p2) {
    if (p1->particle_type == p2->particle_type) {
        return p1->cam_dist > p2->cam_dist;
    } else {
        return p1->particle_type->asset_id < p2->particle_type->asset_id;
    }
}

extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_no_decals;
extern bool g_particle_field_simple;
extern char* global_shader_suffix;

TextureAssetRef smoke_texture;

void DrawGPUParticleField(SceneGraph* scenegraph, const char* type) {
    if (g_debug_runtime_disable_gpu_particle_field_draw) {
        return;
    }

    static bool initialized = false;

    if (initialized == false) {
        smoke_texture = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/smoke.tga");
        initialized = true;
    }

    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();
    Textures* textures = Textures::Instance();
    mat4 proj_view_matrix = cam->GetProjMatrix() * cam->GetViewMatrix();
    CHECK_GL_ERROR();

    GLState gl_state;
    gl_state.blend = true;
    gl_state.depth_test = true;
    gl_state.depth_write = false;
    gl_state.cull_face = false;
    graphics->setGLState(gl_state);
    graphics->setAdditiveBlend(false);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    int shader_id;
    {
        // Get shader id with flags
        char shader_name[ParticleType::kMaxNameLen];
        if (g_particle_field_simple) {
            FormatString(shader_name, ParticleType::kMaxNameLen, "envobject #GPU_PARTICLE_FIELD #GPU_PARTICLE_FIELD_SIMPLE %s %s", type, global_shader_suffix);
        } else {
            FormatString(shader_name, ParticleType::kMaxNameLen, "envobject #GPU_PARTICLE_FIELD %s %s", type, global_shader_suffix);
        }
        shader_id = shaders->returnProgram(shader_name);
    }
    shaders->setProgram(shader_id);
    shaders->SetUniformMat4("mvp", proj_view_matrix);
    shaders->SetUniformMat4("projection_matrix", cam->GetProjMatrix());
    shaders->SetUniformMat4("view_matrix", cam->GetViewMatrix());
    shaders->SetUniformFloat("time", game_timer.GetRenderTime());
    if (Engine::Instance()->paused) {
        shaders->SetUniformFloat("time_scale", 0.05f);
    } else {
        shaders->SetUniformFloat("time_scale", game_timer.time_scale);
    }

    shaders->SetUniformFloat("haze_mult", scenegraph->haze_mult);

    textures->bindTexture(smoke_texture->GetTextureRef());

    {  // Bind shadow matrices to shader
        std::vector<mat4> shadow_matrix;
        shadow_matrix.resize(4);
        for (int i = 0; i < 4; ++i) {
            shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
        }
        std::vector<mat4> temp_shadow_matrix = shadow_matrix;
        if (g_simple_shadows || !g_level_shadows) {
            temp_shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
        }
        shaders->SetUniformMat4Array("shadow_matrix", temp_shadow_matrix);
    }

    // Bind shadow texture
    if (g_simple_shadows || !g_level_shadows) {
        textures->bindTexture(graphics->static_shadow_depth_ref, TEX_SHADOW);
    } else {
        textures->bindTexture(graphics->cascade_shadow_depth_ref, TEX_SHADOW);
    }
    CHECK_GL_ERROR();
    shaders->SetUniformVec3("ws_light", scenegraph->primary_light.pos);
    shaders->SetUniformVec4("primary_light_color", vec4(scenegraph->primary_light.color, scenegraph->primary_light.intensity));
    shaders->SetUniformVec3("cam_pos", cam->GetPos());
    shaders->SetUniformVec3("cam_dir", cam->GetFacing());
    scenegraph->BindLights(shader_id);
    vec2 viewport_dims;
    for (int i = 0; i < 2; ++i) {
        viewport_dims[i] = (float)graphics->viewport_dim[i + 2];
    }
    shaders->SetUniformVec2("viewport_dims", viewport_dims);
    CHECK_GL_ERROR();

    textures->bindTexture(scenegraph->sky->GetSpecularCubeMapTexture(), 2);
    textures->bindTexture(scenegraph->sky->GetSpecularCubeMapTexture(), 3);

    graphics->SetBlendFunc(
        shaders->GetProgramBlendSrc(shader_id),
        shaders->GetProgramBlendDst(shader_id));

    textures->bindTexture(graphics->screen_depth_tex, 5);
    scenegraph->BindLights(shader_id);

    shaders->SetUniformInt("reflection_capture_num", scenegraph->ref_cap_matrix.size());
    if (!scenegraph->ref_cap_matrix.empty()) {
        assert(!scenegraph->ref_cap_matrix_inverse.empty());
        shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph->ref_cap_matrix);
        shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph->ref_cap_matrix_inverse);
    }

    std::vector<mat4> light_volume_matrix;
    std::vector<mat4> light_volume_matrix_inverse;
    for (auto obj : scenegraph->light_volume_objects_) {
        const mat4& mat = obj->GetTransform();
        light_volume_matrix.push_back(mat);
        light_volume_matrix_inverse.push_back(invert(mat));
    }
    shaders->SetUniformInt("light_volume_num", light_volume_matrix.size());
    if (!light_volume_matrix.empty()) {
        assert(!light_volume_matrix_inverse.empty());
        shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
        shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
    }
    if (g_no_reflection_capture == false) {
        textures->bindTexture(scenegraph->cubemaps, 19);
    }
    shaders->SetUniformInt("num_tetrahedra", scenegraph->light_probe_collection.ShaderNumTetrahedra());
    shaders->SetUniformInt("num_light_probes", scenegraph->light_probe_collection.ShaderNumLightProbes());
    shaders->SetUniformVec3("grid_bounds_min", scenegraph->light_probe_collection.grid_lookup.bounds[0]);
    shaders->SetUniformVec3("grid_bounds_max", scenegraph->light_probe_collection.grid_lookup.bounds[1]);
    shaders->SetUniformInt("subdivisions_x", scenegraph->light_probe_collection.grid_lookup.subdivisions[0]);
    shaders->SetUniformInt("subdivisions_y", scenegraph->light_probe_collection.grid_lookup.subdivisions[1]);
    shaders->SetUniformInt("subdivisions_z", scenegraph->light_probe_collection.grid_lookup.subdivisions[2]);
    shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
    shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
    if (scenegraph->light_probe_collection.light_probe_buffer_object_id != -1) {
        glBindBuffer(GL_TEXTURE_BUFFER, scenegraph->light_probe_collection.light_probe_buffer_object_id);
    }

    if (scenegraph->light_probe_collection.light_volume_enabled && scenegraph->light_probe_collection.ambient_3d_tex.valid()) {
        textures->bindTexture(scenegraph->light_probe_collection.ambient_3d_tex, 16);
    }

    CHECK_GL_ERROR();
    PROFILER_LEAVE(g_profiler_ctx);  // Prepare instanced particle draw

    static VBOContainer data_vbo;
    if (!data_vbo.valid()) {
        static const GLfloat data[] = {
            1, 1, 1, 1, 0,
            0, 1, -1, 1, 0,
            0, 0, -1, -1, 0,
            1, 0, 1, -1, 0};
        static GLfloat data_vec[20 * 1000];
        int val = 0;
        for (int i = 0; i < 1000; ++i) {
            for (float j : data) {
                data_vec[val++] = j;
            }
        }
        data_vbo.Fill(kVBOStatic | kVBOFloat, sizeof(data_vec), (void*)data_vec);
    }
    static VBOContainer index_vbo;
    if (!index_vbo.valid()) {
        static const GLuint index[] = {0, 1, 2, 0, 3, 2};
        static GLuint index_vec[6 * 1000];
        int val = 0;
        int val2 = 0;
        for (int i = 0; i < 1000; ++i) {
            for (unsigned int j : index) {
                index_vec[val2++] = j + val;
            }
            val += 4;
        }
        index_vbo.Fill(kVBOStatic | kVBOElement, sizeof(index_vec), (void*)index_vec);
    }
    CHECK_GL_ERROR();
    int vert_attrib_id = shaders->returnShaderAttrib("vertex_attrib", shader_id);
    int tex_coord_attrib_id = shaders->returnShaderAttrib("tex_coord_attrib", shader_id);
    data_vbo.Bind();
    CHECK_GL_ERROR();
    if (vert_attrib_id != -1) {
        graphics->EnableVertexAttribArray(vert_attrib_id);
        glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 5 * sizeof(GLfloat), (const void*)(data_vbo.offset() + 2 * sizeof(GLfloat)));
    }
    CHECK_GL_ERROR();
    if (tex_coord_attrib_id != -1) {
        graphics->EnableVertexAttribArray(tex_coord_attrib_id);
        glVertexAttribPointer(tex_coord_attrib_id, 2, GL_FLOAT, false, 5 * sizeof(GLfloat), (const void*)data_vbo.offset());
    }
    CHECK_GL_ERROR();
    index_vbo.Bind();
    CHECK_GL_ERROR();
    {
        PROFILER_ZONE(g_profiler_ctx, "glDrawElementsInstanced");
        graphics->DrawElementsInstanced(GL_TRIANGLES, 6000, GL_UNSIGNED_INT, 0, 1000);
    }
    graphics->ResetVertexAttribArrays();
}

// Draw a particle
void Particle::Draw(SceneGraph* scenegraph, DrawType draw_type, const mat4& proj_view_matrix) {
    if (g_debug_runtime_disable_particle_draw) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "Particle::Draw");
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();
    Textures* textures = Textures::Instance();
    CHECK_GL_ERROR();

    int shader_id;
    {
        // Get shader id with flags
        char shader_name[ParticleType::kMaxNameLen];

        FormatString(shader_name, ParticleType::kMaxNameLen, "%s %s", particle_type->shader_name, global_shader_suffix);

        shader_id = shaders->returnProgram(shader_name);
        // LOGI << shader_name << std::endl;
    }

    shaders->setProgram(shader_id);
    shaders->SetUniformMat4("mvp", proj_view_matrix);
    shaders->SetUniformVec4("color_tint", interp_color);
    shaders->SetUniformFloat("time", game_timer.GetRenderTime());

    shaders->SetUniformFloat("haze_mult", scenegraph->haze_mult);

    if (particle_type->color_map.valid()) {
        textures->bindTexture(particle_type->color_map->GetTextureRef());
    } else if (ae_reader.valid()) {
        textures->bindTexture(ae_reader.GetTextureAssetRef());
    }

    if (draw_type == COLOR) {
        {  // Bind shadow matrices to shader
            std::vector<mat4> shadow_matrix;
            shadow_matrix.resize(4);
            for (int i = 0; i < 4; ++i) {
                shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
            }
            std::vector<mat4> temp_shadow_matrix = shadow_matrix;
            if (g_simple_shadows || !g_level_shadows) {
                temp_shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
            }
            shaders->SetUniformMat4Array("shadow_matrix", temp_shadow_matrix);
        }

        // Bind shadow texture
        if (g_simple_shadows || !g_level_shadows) {
            textures->bindTexture(graphics->static_shadow_depth_ref, TEX_SHADOW);
        } else {
            textures->bindTexture(graphics->cascade_shadow_depth_ref, TEX_SHADOW);
        }
        CHECK_GL_ERROR();
        shaders->SetUniformVec3("ws_light", scenegraph->primary_light.pos);
        shaders->SetUniformVec4("primary_light_color", vec4(scenegraph->primary_light.color, scenegraph->primary_light.intensity));
        shaders->SetUniformVec3("cam_pos", cam->GetPos());
        scenegraph->BindLights(shader_id);
        vec2 viewport_dims;
        for (int i = 0; i < 2; ++i) {
            viewport_dims[i] = (float)graphics->viewport_dim[i + 2];
        }
        shaders->SetUniformVec2("viewport_dims", viewport_dims);
        CHECK_GL_ERROR();

        if (particle_type->normal_map.valid()) {
            textures->bindTexture(particle_type->normal_map->GetTextureRef(), 1);
        }
        textures->bindTexture(scenegraph->sky->GetSpecularCubeMapTexture(), 2);
        textures->bindTexture(scenegraph->sky->GetSpecularCubeMapTexture(), 3);

        graphics->SetBlendFunc(
            shaders->GetProgramBlendSrc(shader_id),
            shaders->GetProgramBlendDst(shader_id));

        textures->bindTexture(graphics->screen_depth_tex, 5);
        shaders->SetUniformFloat("size", interp_size);
        scenegraph->BindLights(shader_id);

        shaders->SetUniformInt("reflection_capture_num", scenegraph->ref_cap_matrix.size());
        if (!scenegraph->ref_cap_matrix.empty()) {
            assert(!scenegraph->ref_cap_matrix_inverse.empty());
            shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph->ref_cap_matrix);
            shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph->ref_cap_matrix_inverse);
        }

        std::vector<mat4> light_volume_matrix;
        std::vector<mat4> light_volume_matrix_inverse;
        for (auto obj : scenegraph->objects_) {
            const mat4& mat = obj->GetTransform();
            light_volume_matrix.push_back(mat);
            light_volume_matrix_inverse.push_back(invert(mat));
        }
        shaders->SetUniformInt("light_volume_num", light_volume_matrix.size());
        if (!light_volume_matrix.empty()) {
            assert(!light_volume_matrix_inverse.empty());
            shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
            shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
        }

        if (g_no_reflection_capture == false) {
            textures->bindTexture(scenegraph->cubemaps, 19);
        }
        shaders->SetUniformInt("num_tetrahedra", scenegraph->light_probe_collection.ShaderNumTetrahedra());
        shaders->SetUniformInt("num_light_probes", scenegraph->light_probe_collection.ShaderNumLightProbes());
        shaders->SetUniformVec3("grid_bounds_min", scenegraph->light_probe_collection.grid_lookup.bounds[0]);
        shaders->SetUniformVec3("grid_bounds_max", scenegraph->light_probe_collection.grid_lookup.bounds[1]);
        shaders->SetUniformInt("subdivisions_x", scenegraph->light_probe_collection.grid_lookup.subdivisions[0]);
        shaders->SetUniformInt("subdivisions_y", scenegraph->light_probe_collection.grid_lookup.subdivisions[1]);
        shaders->SetUniformInt("subdivisions_z", scenegraph->light_probe_collection.grid_lookup.subdivisions[2]);
        shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
        shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
        if (scenegraph->light_probe_collection.light_probe_buffer_object_id != -1) {
            glBindBuffer(GL_TEXTURE_BUFFER, scenegraph->light_probe_collection.light_probe_buffer_object_id);
        }

        if (scenegraph->light_probe_collection.light_volume_enabled && scenegraph->light_probe_collection.ambient_3d_tex.valid()) {
            textures->bindTexture(scenegraph->light_probe_collection.ambient_3d_tex, 16);
        }
    }

    CHECK_GL_ERROR();

    if (connected.empty()) {
        vec3 up, right;
        float temp_size;
        vec3 temp_pos;
        vec3 particle_facing;
        if (draw_type == COLOR) {
            vec3 look;
            look = cam->GetPos() - interp_position;
            // Don't draw if cam is not facing towards particle
            if (dot(look, cam->GetFacing()) > 0.0f) {
                return;
            }
            float cam_distance = length(look);
            if (cam_distance > 0.0f) {
                look /= cam_distance;
            }

            bool velocity_axis = particle_type->velocity_axis;
            if (velocity_axis || particle_type->speed_stretch) {
                particle_facing = look;
            } else {
                particle_facing = cam->GetFacing();
            }

            if (velocity_axis) {
                if (particle_type->speed_stretch) {
                    up = velocity / sqrtf(length(velocity)) * 2.0f * particle_type->speed_mult;
                } else {
                    up = normalize(velocity);
                }
            } else {
                up = AngleAxisRotation(cam->GetUpVector(), particle_facing, interp_rotation);
            }

            right = normalize(cross(up, particle_facing));
            if (particle_type->min_squash) {
                if (particle_type->speed_stretch) {
                    float velocity_length = length(up);
                    if (fabs(dot(normalize(particle_facing), normalize(up))) > 1.0f - 0.2f / velocity_length / velocity_length) {
                        float v = 1.0f - 0.2f / velocity_length / velocity_length;
                        if (v < -1.0f) {
                            v = -1.0f;
                        } else if (v > 1.0f) {
                            v = 1.0f;
                        }

                        up = AngleAxisRotationRadian(particle_facing * velocity_length, right, acosf(v));
                    }
                }
                if (!particle_type->speed_stretch && fabs(dot(particle_facing, up)) > 0.8f) {
                    up = AngleAxisRotationRadian(particle_facing, right, acosf(0.8f));
                }
            }
            if (velocity_axis) {
                particle_facing = normalize(cross(up, right));
            }

            temp_pos = interp_position;
            temp_size = interp_size;

            float offset = interp_size * 0.5f;
            temp_pos += look * offset;
            temp_size *= (cam_distance - offset) / cam_distance;
        } else {
            temp_size = interp_size;
            temp_pos = interp_position;
            particle_facing = normalize(scenegraph->primary_light.pos);
            right = cross(particle_facing, vec3(0, -1, 0));
            up = cross(right, particle_facing);
        }

        vec3 vertices[4];
        vertices[0] = temp_pos + (up + right) * temp_size;
        vertices[1] = temp_pos + (up - right) * temp_size;
        vertices[2] = temp_pos - (up + right) * temp_size;
        vertices[3] = temp_pos - (up - right) * temp_size;

        GLfloat data[] = {
            1,
            1,
            vertices[0][0],
            vertices[0][1],
            vertices[0][2],
            right[0],
            right[1],
            right[2],
            particle_facing[0],
            particle_facing[1],
            particle_facing[2],
            0,
            1,
            vertices[1][0],
            vertices[1][1],
            vertices[1][2],
            right[0],
            right[1],
            right[2],
            particle_facing[0],
            particle_facing[1],
            particle_facing[2],
            0,
            0,
            vertices[2][0],
            vertices[2][1],
            vertices[2][2],
            right[0],
            right[1],
            right[2],
            particle_facing[0],
            particle_facing[1],
            particle_facing[2],
            1,
            0,
            vertices[3][0],
            vertices[3][1],
            vertices[3][2],
            right[0],
            right[1],
            right[2],
            particle_facing[0],
            particle_facing[1],
            particle_facing[2],
        };

        CHECK_GL_ERROR();
        static VBORingContainer data_vbo(sizeof(data), kVBODynamic | kVBOFloat);
        data_vbo.Fill(sizeof(data), data);
        static VBOContainer index_vbo;
        if (!index_vbo.valid()) {
            static const GLuint index[] = {0, 1, 2, 0, 3, 2};
            index_vbo.Fill(kVBOStatic | kVBOElement, sizeof(index), (void*)index);
        }
        CHECK_GL_ERROR();
        int vert_attrib_id = shaders->returnShaderAttrib("vertex_attrib", shader_id);
        int normal_attrib_id = shaders->returnShaderAttrib("normal_attrib", shader_id);
        int tangent_attrib_id = shaders->returnShaderAttrib("tangent_attrib", shader_id);
        int tex_coord_attrib_id = shaders->returnShaderAttrib("tex_coord_attrib", shader_id);
        data_vbo.Bind();
        CHECK_GL_ERROR();
        graphics->EnableVertexAttribArray(vert_attrib_id);
        CHECK_GL_ERROR();
        graphics->EnableVertexAttribArray(tex_coord_attrib_id);
        CHECK_GL_ERROR();
        glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(data_vbo.offset() + 2 * sizeof(GLfloat)));
        CHECK_GL_ERROR();
        glVertexAttribPointer(tex_coord_attrib_id, 2, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(data_vbo.offset()));
        CHECK_GL_ERROR();
        if (normal_attrib_id != -1) {
            graphics->EnableVertexAttribArray(normal_attrib_id);
            glVertexAttribPointer(normal_attrib_id, 3, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(data_vbo.offset() + 8 * sizeof(GLfloat)));
        }
        if (tangent_attrib_id != -1) {
            graphics->EnableVertexAttribArray(tangent_attrib_id);
            glVertexAttribPointer(tangent_attrib_id, 3, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(data_vbo.offset() + 5 * sizeof(GLfloat)));
        }
        CHECK_GL_ERROR();
        index_vbo.Bind();
        CHECK_GL_ERROR();
        graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)index_vbo.offset());
        CHECK_GL_ERROR();
        graphics->ResetVertexAttribArrays();
        CHECK_GL_ERROR();
    }
    for (auto& iter : connected) {
        if (this < iter) {
            float thickness = interp_size;
            vec3 a = interp_position;
            vec3 b = iter->interp_position;

            vec3 up, right, cam_to_particle;
            vec3 vertices[4];

            if (draw_type == COLOR) {
                cam_to_particle = cam->GetPos() - (a + b) * 0.5f;
                float cam_distance = length(cam_to_particle);
                if (cam_distance > 0.0f) {
                    cam_to_particle /= cam_distance;  // Normalize cam_to_particle
                }

                if (dot(cam_to_particle, cam->GetFacing()) > 0.0f) {
                    return;  // Return if particle is behind cam
                }
            } else {
                cam_to_particle = scenegraph->primary_light.pos;
            }

            vec3 particle_facing = cam_to_particle;
            up = b - a;
            right = normalize(cross(normalize(up), particle_facing));
            vec3 true_facing = normalize(cross(up, right));

            vec3 temp_pos = (a + b) * 0.5f;

            vertices[0] = temp_pos + up + right * thickness;
            vertices[1] = temp_pos + up - right * thickness;
            vertices[2] = temp_pos - up - right * thickness;
            vertices[3] = temp_pos - up + right * thickness;

            GLfloat data[] = {
                1,
                1,
                vertices[0][0],
                vertices[0][1],
                vertices[0][2],
                right[0],
                right[1],
                right[2],
                true_facing[0],
                true_facing[1],
                true_facing[2],
                0,
                1,
                vertices[1][0],
                vertices[1][1],
                vertices[1][2],
                right[0],
                right[1],
                right[2],
                true_facing[0],
                true_facing[1],
                true_facing[2],
                0,
                0,
                vertices[2][0],
                vertices[2][1],
                vertices[2][2],
                right[0],
                right[1],
                right[2],
                true_facing[0],
                true_facing[1],
                true_facing[2],
                1,
                0,
                vertices[3][0],
                vertices[3][1],
                vertices[3][2],
                right[0],
                right[1],
                right[2],
                true_facing[0],
                true_facing[1],
                true_facing[2],
            };

            CHECK_GL_ERROR();
            static VBOContainer data_vbo;
            data_vbo.Fill(kVBODynamic | kVBOFloat, sizeof(data), data);
            static VBOContainer index_vbo;
            if (!index_vbo.valid()) {
                static const GLuint index[] = {0, 1, 2, 0, 3, 2};
                index_vbo.Fill(kVBOStatic | kVBOElement, sizeof(index), (void*)index);
            }
            CHECK_GL_ERROR();
            int vert_attrib_id = shaders->returnShaderAttrib("vertex_attrib", shader_id);
            int normal_attrib_id = shaders->returnShaderAttrib("normal_attrib", shader_id);
            int tangent_attrib_id = shaders->returnShaderAttrib("tangent_attrib", shader_id);
            int tex_coord_attrib_id = shaders->returnShaderAttrib("tex_coord_attrib", shader_id);
            data_vbo.Bind();
            CHECK_GL_ERROR();
            graphics->EnableVertexAttribArray(vert_attrib_id);
            CHECK_GL_ERROR();
            graphics->EnableVertexAttribArray(tex_coord_attrib_id);
            CHECK_GL_ERROR();
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(2 * sizeof(GLfloat)));
            CHECK_GL_ERROR();
            glVertexAttribPointer(tex_coord_attrib_id, 2, GL_FLOAT, false, 11 * sizeof(GLfloat), 0);
            CHECK_GL_ERROR();
            if (normal_attrib_id != -1) {
                graphics->EnableVertexAttribArray(normal_attrib_id);
                glVertexAttribPointer(normal_attrib_id, 3, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(8 * sizeof(GLfloat)));
            }
            if (tangent_attrib_id != -1) {
                graphics->EnableVertexAttribArray(tangent_attrib_id);
                glVertexAttribPointer(tangent_attrib_id, 3, GL_FLOAT, false, 11 * sizeof(GLfloat), (const void*)(5 * sizeof(GLfloat)));
            }
            CHECK_GL_ERROR();
            index_vbo.Bind();
            CHECK_GL_ERROR();
            graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            CHECK_GL_ERROR();
            graphics->ResetVertexAttribArrays();
            CHECK_GL_ERROR();
        }
    }
    CHECK_GL_ERROR();
}

// Update particle
void Particle::Update(SceneGraph* scenegraph, float timestep, float curr_game_time) {
    old_position = position;
    old_color = color;
    old_size = size;
    old_rotation = rotation;

    if (ae_reader.valid()) {
        ae_reader.Update(timestep);
        if (ae_reader.Done()) {
            size = 0.0f;
            color[3] = 0.0f;
        }
    }

    position += velocity * timestep;
    alive_time += timestep;

    velocity *= particle_type->inertia;
    Physics* pi = Physics::Instance();
    velocity += pi->gravity * particle_type->gravity * timestep;
    velocity += pi->GetWind(position, curr_game_time, 1.0f) * particle_type->wind * timestep;
    if (particle_type->quadratic_expansion) {
        size = initial_size * sqrtf(alive_time * particle_type->qe_speed);
    } else {
        size -= particle_type->size_decay_rate * timestep;
    }
    if (particle_type->quadratic_dispersion) {
        color[3] = square(initial_size / (size + initial_size)) * particle_type->qd_mult;
        color[3] = min(1.0f, color[3]) * min((alive_time + 0.05f) * 10, 1.0f) * initial_opacity - 0.1f;
    } else {
        color[3] = initial_opacity - particle_type->opacity_decay_rate * alive_time;
        if (alive_time < particle_type->opacity_ramp_time) {
            color[3] *= alive_time / particle_type->opacity_ramp_time;
        }
    }
    rotation += timestep * rotate_speed * 0.3f / (size / initial_size);

    if (!connected.empty()) {
        has_last_connected = true;
        last_connected_pos = (*connected.begin())->position;
    }

    if (particle_type->collision) {
        SimpleRayResultCallbackInfo cb;
        vec3 point, normal;
        bool hit = (scenegraph->bullet_world_->CheckRayCollision(
                        old_position,
                        position,
                        &point,
                        &normal) != NULL);
        vec3 new_pos = hit ? point : position;
        int char_id = -1;
        if (particle_type->character_collide) {
            char_id = scenegraph->CheckRayCollisionCharacters(
                old_position,
                new_pos,
                &point,
                &normal,
                NULL);
        }
        if (hit && char_id == -1) {
            // SDL.position = point;
            // SDL.velocity = reflect(SDL.velocity, normal);
            if (particle_type->collision_destroy) {
                color[3] = 0.0f;
            }
            if (!particle_type->collision_decal.empty() && !collided) {
                // Create decal at collision point
                EntityDescriptionList desc_list;
                std::string file_type;
                Path source;
                ActorsEditor_LoadEntitiesFromFile(particle_type->collision_decal, desc_list, &file_type, &source);
                Object* obj = CreateObjectFromDesc(desc_list[0]);
                if (obj) {
                    obj->permission_flags = obj->permission_flags & ~Object::CAN_SELECT;
                    // align decal up (y+) with surface normal
                    vec3 up = vec3(0.0f, 1.0f, 0.0f);
                    vec3 rot_axis = cross(up, normal);
                    float angle2_cos = dot(up, normal);

                    // Sometimes rounding errors in combination to similar vectors cause the dot product to be more than 1.0f
                    // Which results in a NaN when used in the sqrtf later.
                    if (angle2_cos > 1.0f) {
                        angle2_cos = 1.0f;
                    }

                    // constructing quaternion requires half-angle
                    // use trig identities to calculate sin and cos without
                    // actually calculating the angle
                    float angle_cos = sqrtf((1.0f + angle2_cos) / 2.0f);
                    float angle_sin = sqrtf(1.0f - (angle_cos * angle_cos));

                    quaternion rot;
                    rot.entries[0] = rot_axis.x() * angle_sin;
                    rot.entries[1] = rot_axis.y() * angle_sin;
                    rot.entries[2] = rot_axis.z() * angle_sin;
                    rot.entries[3] = angle_cos;
                    obj->SetRotation(rot);

                    if (has_last_connected) {
                        vec3 dir = normalize(velocity);
                        vec3 direction = normal * -1.0f;
                        dir = last_connected_pos - position;
                        float length_dir = length(dir);
                        if (length_dir < 0.01f) {
                            // if dir is zero, just construct any vector that is non-collinear with direction
                            // so that we can construct orthogonal u and v properly later
                            if (direction[0] != 0.0f || direction[1] != 0.0f) {
                                dir = vec3(-direction[1], direction[0], direction[2]);
                            } else {
                                dir = vec3(direction[0], -direction[2], direction[1]);
                            }
                        }
                        float decal_size = size * particle_type->collision_decal_size_mult;
                        length_dir = min(length_dir, decal_size * 3);
                        // Decal placement position and direction
                        obj->SetTranslation((position + last_connected_pos) * 0.5f);
                        obj->SetScale(vec3(decal_size, length_dir + decal_size, length_dir + decal_size));
                        {
                            vec3 axes[3];
                            axes[1] = normal;
                            axes[2] = normalize(dir);
                            axes[0] = normalize(cross(axes[1], axes[2]));
                            axes[2] = normalize(cross(axes[1], axes[0]));
                            mat4 temp;
                            temp.SetColumn(0, -axes[0]);
                            temp.SetColumn(1, axes[1]);
                            temp.SetColumn(2, axes[2]);
                            obj->SetRotation(QuaternionFromMat4(temp));
                        }
                    } else {
                        obj->SetTranslation(point);
                        obj->SetRotation(obj->GetRotation() * quaternion(vec4(0.0f, 1.0f, 0.0f, RangedRandomFloat(0.0f, PI_f * 2.0f))));
                        obj->SetScale(vec3(size * particle_type->collision_decal_size_mult));
                    }
                    obj->exclude_from_undo = true;
                    obj->exclude_from_save = true;

                    DecalObject* dec = static_cast<DecalObject*>(obj);

                    if (dec->decal_file_ref->special_type == 2) {
                        // Only blood decals should use user specified color tint
                        const vec3 blood_color = Graphics::Instance()->config_.blood_color();
                        dec->color_tint_component_.tint_ = blood_color;
                    }

                    if (scenegraph->AddDynamicDecal(dec)) {
                    } else {
                        delete dec;
                    }
                } else {
                    LOGE << "Failed at loading object for particle system." << std::endl;
                }
                // DebugDraw::Instance()->AddLine(decal_info.point-decal_info.u * decal_info.
                /*
                std::vector<Decal*> decals =
                    scenegraph->decals->makeDecalOnScene(scenegraph, decal_info, NULL, curr_game_time);
                for(unsigned i=0; i<decals.size(); ++i){
                    scenegraph->decals->AddToDecalList(decals[i]);
                }
                */
            }
            if (!particle_type->collision_event.empty() && !collided) {
                const MaterialEvent* me = scenegraph->GetMaterialEvent(particle_type->collision_event, point);
                if (me && !me->soundgroup.empty()) {
                    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(me->soundgroup);
                    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(me->soundgroup);
                    SoundGroupPlayInfo sgpi(*sgr, point);
                    sgpi.gain = min(1.0f, length_squared(velocity) * 0.2f * size);
                    sgpi.priority = _sound_priority_low;
                    unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
                    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
                }
            }
            collided = true;
        } else if (char_id != -1) {
            if (particle_type->collision_destroy) {
                color[3] = 0.0f;
            }
            if (!particle_type->collision_event.empty() && !collided) {
                Object* o = scenegraph->GetObjectFromID(char_id);
                const MaterialEvent& me = o->GetMaterialEvent(particle_type->collision_event, point);
                if (!me.soundgroup.empty()) {
                    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(me.soundgroup);
                    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(me.soundgroup);
                    SoundGroupPlayInfo sgpi(*sgr, point);
                    sgpi.gain = min(1.0f, length_squared(velocity) * 0.2f * size);
                    sgpi.priority = _sound_priority_low;
                    unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
                    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
                }
            }
            if (particle_type->character_add_blood && !collided) {
                MovementObject* mo = (MovementObject*)scenegraph->GetObjectFromID(char_id);
                mo->rigged_object()->AddBloodAtPoint(point);
            }
            collided = true;
        }
    }

    if (alive_time == timestep) {
        old_position = position;
        old_color = color;
        old_size = size;
        old_rotation = rotation;
    }
}

UniformRingBuffer particle_uniform_buffer;

// Draw all particles
void ParticleSystem::Draw(SceneGraph* scenegraph) {
    if (g_debug_runtime_disable_particle_system_draw) {
        return;
    }

    Graphics* graphics = Graphics::Instance();
    CHECK_GL_ERROR();
    if (!particles.empty() && !graphics->drawing_shadow) {
        Camera* cam = ActiveCameras::Get();
        GLubyte* blockBuffer = (GLubyte*)alloca(16384);
        GLState gl_state;
        gl_state.blend = true;
        gl_state.depth_test = true;
        gl_state.depth_write = false;
        gl_state.cull_face = false;
        graphics->setGLState(gl_state);
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

        vec2 viewport_dims;
        for (int i = 0; i < 2; ++i) {
            viewport_dims[i] = (float)graphics->viewport_dim[i + 2];
        }

        static std::vector<mat4> shadow_matrix;
        shadow_matrix.resize(4);
        for (int i = 0; i < 4; ++i) {
            shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
        }
        static std::vector<mat4> temp_shadow_matrix;
        temp_shadow_matrix = shadow_matrix;
        if (g_simple_shadows || !g_level_shadows) {
            temp_shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
        }

        PROFILER_ENTER(g_profiler_ctx, "Interpolate particles");
        float interp = game_timer.GetInterpWeight();
        for (auto& particle : particles) {
            Particle& p = *particle;
            p.interp_position = mix(p.old_position, p.position, interp);
            p.interp_rotation = mix(p.old_rotation, p.rotation, interp);
            p.interp_size = mix(p.old_size, p.size, interp);
            p.interp_color = mix(p.old_color, p.color, interp);
        }
        PROFILER_LEAVE(g_profiler_ctx);  // Interpolate particles

        vec3 cam_pos = ActiveCameras::Get()->GetPos();
        for (auto& particle : particles) {
            particle->cam_dist = distance_squared(particle->interp_position, cam_pos);
        }
        PROFILER_ENTER(g_profiler_ctx, "Sort particles");
        std::sort(particles.begin(), particles.end(), CParticleCompare);
        PROFILER_LEAVE(g_profiler_ctx);  // Sort particles
        mat4 proj_view_matrix = cam->GetProjMatrix() * cam->GetViewMatrix();
        ParticleTypeRef curr_type = particles[0]->particle_type;
        bool unconnected = particles[0]->connected.empty();
        unsigned start = 0;
        PROFILER_ENTER(g_profiler_ctx, "Draw Loop");
        for (unsigned i = 0, len = particles.size(); i <= len; i++) {
            if (i == particles.size() || particles[i]->particle_type != curr_type || particles[i]->connected.empty() != unconnected) {
                ParticleTypeRef particle_type = particles[i - 1]->particle_type;
                if (!particle_type->color_map.valid() || !particles[i - 1]->connected.empty()) {
                    for (unsigned index = start; index < i; ++index) {
                        particles[index]->Draw(scenegraph, Particle::COLOR, proj_view_matrix);
                    }
                } else {
                    PROFILER_ENTER(g_profiler_ctx, "Prepare instanced particle draw");
                    Graphics* graphics = Graphics::Instance();
                    Shaders* shaders = Shaders::Instance();
                    Camera* cam = ActiveCameras::Get();
                    Textures* textures = Textures::Instance();
                    CHECK_GL_ERROR();

                    PROFILER_ENTER(g_profiler_ctx, "Set up shader");
                    PROFILER_ENTER(g_profiler_ctx, "Bind shader");
                    int shader_id;
                    {
                        // Get shader id with flags
                        char shader_name[ParticleType::kMaxNameLen];

                        FormatString(shader_name, ParticleType::kMaxNameLen, "%s #INSTANCED %s", particle_type->shader_name, global_shader_suffix);
                        shader_id = shaders->returnProgram(shader_name);
                        // LOGI << shader_name << std::endl;
                    }

                    shaders->setProgram(shader_id);
                    PROFILER_LEAVE(g_profiler_ctx);
                    PROFILER_ENTER(g_profiler_ctx, "Uniforms");
                    shaders->SetUniformMat4("mvp", proj_view_matrix);
                    shaders->SetUniformFloat("time", game_timer.GetRenderTime());
                    shaders->SetUniformFloat("haze_mult", scenegraph->haze_mult);
                    {  // Bind shadow matrices to shader
                        shaders->SetUniformMat4Array("shadow_matrix", temp_shadow_matrix);
                    }
                    CHECK_GL_ERROR();
                    shaders->SetUniformVec3("ws_light", scenegraph->primary_light.pos);
                    shaders->SetUniformVec4("primary_light_color", vec4(scenegraph->primary_light.color, scenegraph->primary_light.intensity));
                    shaders->SetUniformVec3("cam_pos", cam->GetPos());
                    shaders->SetUniformVec2("viewport_dims", viewport_dims);
                    PROFILER_LEAVE(g_profiler_ctx);
                    PROFILER_ENTER(g_profiler_ctx, "BindLights");
                    scenegraph->BindLights(shader_id);
                    PROFILER_LEAVE(g_profiler_ctx);

                    if (scenegraph->light_probe_collection.ShaderNumLightProbes() == 0) {
                        shaders->SetUniformInt("light_volume_num", 0);
                        shaders->SetUniformInt("num_tetrahedra", 0);
                        shaders->SetUniformInt("num_light_probes", 0);
                    } else {
                        std::vector<mat4> light_volume_matrix;
                        std::vector<mat4> light_volume_matrix_inverse;
                        for (auto obj : scenegraph->light_volume_objects_) {
                            const mat4& mat = obj->GetTransform();
                            light_volume_matrix.push_back(mat);
                            light_volume_matrix_inverse.push_back(invert(mat));
                        }
                        shaders->SetUniformInt("light_volume_num", light_volume_matrix.size());
                        if (!light_volume_matrix.empty()) {
                            assert(!light_volume_matrix_inverse.empty());
                            shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
                            shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
                        }
                        shaders->SetUniformInt("num_tetrahedra", scenegraph->light_probe_collection.ShaderNumTetrahedra());
                        shaders->SetUniformInt("num_light_probes", scenegraph->light_probe_collection.ShaderNumLightProbes());
                        shaders->SetUniformVec3("grid_bounds_min", scenegraph->light_probe_collection.grid_lookup.bounds[0]);
                        shaders->SetUniformVec3("grid_bounds_max", scenegraph->light_probe_collection.grid_lookup.bounds[1]);
                        shaders->SetUniformInt("subdivisions_x", scenegraph->light_probe_collection.grid_lookup.subdivisions[0]);
                        shaders->SetUniformInt("subdivisions_y", scenegraph->light_probe_collection.grid_lookup.subdivisions[1]);
                        shaders->SetUniformInt("subdivisions_z", scenegraph->light_probe_collection.grid_lookup.subdivisions[2]);
                        shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
                        shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
                    }

                    PROFILER_ENTER(g_profiler_ctx, "reflection capture");
                    shaders->SetUniformInt("reflection_capture_num", scenegraph->ref_cap_matrix.size());
                    if (!scenegraph->ref_cap_matrix.empty()) {
                        assert(!scenegraph->ref_cap_matrix_inverse.empty());
                        shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph->ref_cap_matrix);
                        shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph->ref_cap_matrix_inverse);
                    }
                    PROFILER_LEAVE(g_profiler_ctx);

                    graphics->SetBlendFunc(
                        shaders->GetProgramBlendSrc(shader_id),
                        shaders->GetProgramBlendDst(shader_id));

                    CHECK_GL_ERROR();
                    PROFILER_LEAVE(g_profiler_ctx);  // "Set up shader"

                    PROFILER_ENTER(g_profiler_ctx, "Textures");
                    if (particle_type->color_map.valid()) {
                        textures->bindTexture(particle_type->color_map);
                    }
                    if (g_simple_shadows || !g_level_shadows) {
                        textures->bindTexture(graphics->static_shadow_depth_ref, TEX_SHADOW);
                    } else {
                        textures->bindTexture(graphics->cascade_shadow_depth_ref, TEX_SHADOW);
                    }
                    if (particle_type->normal_map.valid()) {
                        textures->bindTexture(particle_type->normal_map, 1);
                    }
                    textures->bindTexture(scenegraph->sky->GetSpecularCubeMapTexture(), 2);
                    textures->bindTexture(scenegraph->sky->GetSpecularCubeMapTexture(), 3);
                    textures->bindTexture(graphics->screen_depth_tex, 5);

                    if (g_no_reflection_capture == false) {
                        textures->bindTexture(scenegraph->cubemaps, 19);
                    }
                    if (scenegraph->light_probe_collection.light_probe_buffer_object_id != -1) {
                        glBindBuffer(GL_TEXTURE_BUFFER, scenegraph->light_probe_collection.light_probe_buffer_object_id);
                    }
                    if (scenegraph->light_probe_collection.light_volume_enabled && scenegraph->light_probe_collection.ambient_3d_tex.valid()) {
                        textures->bindTexture(scenegraph->light_probe_collection.ambient_3d_tex, 16);
                    }
                    PROFILER_LEAVE(g_profiler_ctx);  //"Textures"
                    CHECK_GL_ERROR();
                    PROFILER_LEAVE(g_profiler_ctx);  // Prepare instanced particle draw

                    int instance_block_index = shaders->GetUBOBindIndex(shader_id, "InstanceInfo");
                    if ((unsigned)instance_block_index != GL_INVALID_INDEX) {
                        const GLchar* names[] = {
                            "instance_color[0]",
                            "instance_transform[0]",
                            // These long names were necessary on a Mac OS 10.7 ATI card
                            "InstanceInfo.instance_color[0]",
                            "InstanceInfo.instance_transform[0]"};

                        GLuint indices[2];
                        for (int i = 0; i < 2; ++i) {
                            indices[i] = shaders->returnShaderVariableIndex(names[i], shader_id);
                            if (indices[i] == GL_INVALID_INDEX) {
                                indices[i] = shaders->returnShaderVariableIndex(names[i + 2], shader_id);
                            }
                        }
                        GLint offset[2];
                        for (int i = 0; i < 2; ++i) {
                            if (indices[i] != GL_INVALID_INDEX) {
                                offset[i] = shaders->returnShaderVariableOffset(indices[i], shader_id);
                            }
                        }
                        GLint block_size = shaders->returnShaderBlockSize(instance_block_index, shader_id);

                        // TODO: Allocate this memory on a stack_allocator, it's safer, this solution is less predictable and might blow the stack in certain cases, but at least it has a slightly higher chance of running than a statically defined array size.
                        LOG_ASSERT_GT(block_size, 0);
                        LOG_ASSERT_LT(block_size, 32 * 1024);  // Assert that the block size doesn't exceed a hefty 32kib

                        PROFILER_ENTER(g_profiler_ctx, "Setup uniform block");
                        if (particle_uniform_buffer.gl_id == -1) {
                            particle_uniform_buffer.Create(128 * 1024);
                        }
                        PROFILER_LEAVE(g_profiler_ctx);  // Setup uniform block

                        vec4* instance_color = (vec4*)((uintptr_t)blockBuffer + offset[0]);
                        mat4* instance_transform = (mat4*)((uintptr_t)blockBuffer + offset[1]);

                        const unsigned kBatchSize = 100;

                        for (unsigned index = start; index < i; index += kBatchSize) {
                            glUniformBlockBinding(shaders->programs[shader_id].gl_program, instance_block_index, 0);
                            unsigned to_draw = min(kBatchSize, ((int)i - index));
                            PROFILER_ENTER(g_profiler_ctx, "Setup batch data");

                            for (unsigned j = 0; j < to_draw; ++j) {
                                Particle* particle = particles[index + j];
                                mat4 transform;
                                vec3 up, right;
                                float temp_size;
                                vec3 temp_pos;
                                vec3 particle_facing;

                                vec3 look;
                                look = cam->GetPos() - particle->interp_position;

                                float cam_distance = length(look);
                                if (cam_distance > 0.0f) {
                                    look /= cam_distance;
                                }

                                bool velocity_axis = particle_type->velocity_axis;
                                if (velocity_axis || particle_type->speed_stretch) {
                                    particle_facing = look;
                                } else {
                                    particle_facing = cam->GetFacing();
                                }

                                if (velocity_axis) {
                                    if (particle_type->speed_stretch) {
                                        up = particle->velocity / sqrtf(length(particle->velocity)) * 2.0f * particle_type->speed_mult;
                                    } else {
                                        up = normalize(particle->velocity);
                                    }
                                } else {
                                    up = AngleAxisRotation(cam->GetUpVector(), particle_facing, particle->interp_rotation);
                                }

                                right = normalize(cross(up, particle_facing));
                                if (particle_type->min_squash) {
                                    if (particle_type->speed_stretch) {
                                        float velocity_length = length(up);
                                        if (fabs(dot(normalize(particle_facing), normalize(up))) > 1.0f - 0.2f / velocity_length / velocity_length) {
                                            float v = 1.0f - 0.2f / velocity_length / velocity_length;

                                            if (v < -1.0f) {
                                                v = -1.0f;
                                            } else if (v > 1.0f) {
                                                v = 1.0f;
                                            }

                                            up = AngleAxisRotationRadian(particle_facing * velocity_length, right, acosf(v));
                                        }
                                    }
                                    if (!particle_type->speed_stretch && fabs(dot(particle_facing, up)) > 0.8f) {
                                        up = AngleAxisRotationRadian(particle_facing, right, acosf(0.8f));
                                    }
                                }
                                if (velocity_axis) {
                                    particle_facing = normalize(cross(up, right));
                                }

                                temp_pos = particle->interp_position;
                                temp_size = particle->interp_size;

                                float offset = particle->interp_size * 0.5f;
                                temp_pos += look * offset;
                                temp_size *= (cam_distance - offset) / cam_distance;

                                mat4 translation;
                                translation.SetTranslation(temp_pos);
                                mat4 rotation;
                                rotation.SetColumn(0, right);
                                rotation.SetColumn(1, normalize(up));
                                rotation.SetColumn(2, particle_facing);
                                mat4 scale;
                                scale[0] = temp_size;
                                scale[5] = temp_size * length(up);
                                scale[10] = temp_size;
                                transform = translation * rotation * scale;

                                if (indices[0] != GL_INVALID_INDEX) {
                                    instance_color[j] = particle->interp_color;
                                }
                                if (indices[1] != GL_INVALID_INDEX) {
                                    instance_transform[j] = transform;
                                }
                            }
                            PROFILER_LEAVE(g_profiler_ctx);  // Setup batch data
                            {
                                particle_uniform_buffer.Fill(16384 - 1, blockBuffer);
                                glBindBuffer(GL_UNIFORM_BUFFER, particle_uniform_buffer.gl_id);
                                glBindBufferRange(GL_UNIFORM_BUFFER, 0, particle_uniform_buffer.gl_id, particle_uniform_buffer.offset, particle_uniform_buffer.next_offset - particle_uniform_buffer.offset);
                            }
                            CHECK_GL_ERROR();
                            {
                                static VBOContainer data_vbo;
                                if (!data_vbo.valid()) {
                                    static const GLfloat data[] = {
                                        1, 1, 1, 1, 0,
                                        0, 1, -1, 1, 0,
                                        0, 0, -1, -1, 0,
                                        1, 0, 1, -1, 0};
                                    data_vbo.Fill(kVBOStatic | kVBOFloat, sizeof(data), (void*)data);
                                }
                                static VBOContainer index_vbo;
                                if (!index_vbo.valid()) {
                                    static const GLuint index[] = {0, 1, 2, 0, 3, 2};
                                    index_vbo.Fill(kVBOStatic | kVBOElement, sizeof(index), (void*)index);
                                }
                                CHECK_GL_ERROR();
                                int vert_attrib_id = shaders->returnShaderAttrib("vertex_attrib", shader_id);
                                int tex_coord_attrib_id = shaders->returnShaderAttrib("tex_coord_attrib", shader_id);
                                data_vbo.Bind();
                                CHECK_GL_ERROR();
                                graphics->EnableVertexAttribArray(vert_attrib_id);
                                CHECK_GL_ERROR();
                                graphics->EnableVertexAttribArray(tex_coord_attrib_id);
                                CHECK_GL_ERROR();
                                glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 5 * sizeof(GLfloat), (const void*)(data_vbo.offset() + 2 * sizeof(GLfloat)));
                                CHECK_GL_ERROR();
                                glVertexAttribPointer(tex_coord_attrib_id, 2, GL_FLOAT, false, 5 * sizeof(GLfloat), (const void*)data_vbo.offset());
                                CHECK_GL_ERROR();
                                index_vbo.Bind();
                                CHECK_GL_ERROR();
                                {
                                    PROFILER_ZONE(g_profiler_ctx, "glDrawElementsInstanced");
                                    graphics->DrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, to_draw);
                                }
                                graphics->ResetVertexAttribArrays();
                                CHECK_GL_ERROR();
                            }
                            CHECK_GL_ERROR();
                        }
                    }
                }
                if (i != particles.size()) {
                    curr_type = particles[i]->particle_type;
                    unconnected = particles[i]->connected.empty();
                    start = i;
                }
            }
        }
        PROFILER_LEAVE(g_profiler_ctx);  //"Draw Loop"
    }
    CHECK_GL_ERROR();
}

// Update particles
void ParticleSystem::Update(SceneGraph* scenegraph, float timestep, float curr_game_time) {
    {
        PROFILER_ZONE(g_profiler_ctx, "Particle script update");
        script_context_.CallScriptFunction(as_funcs.update);
    }
    {
        PROFILER_ZONE(g_profiler_ctx, "Looping through all particles");
        for (int i = (int)particles.size() - 1; i >= 0; i--) {
            particles[i]->Update(scenegraph, timestep, curr_game_time);
            if (particles[i]->color[3] <= 0 || particles[i]->size <= 0) deleteParticle(i);
        }
    }
}

ParticleSystem::ParticleSystem(const ASData& as_data) : script_context_("particle_system", as_data), last_id_created(0) {
    Path script_path = FindFilePath(script_dir_path + "particle_system.as", kModPaths | kDataPaths);
    as_funcs.update = script_context_.RegisterExpectedFunction("void Update()", false);
    script_context_.LoadScript(script_path);
}

// Delete particles
void ParticleSystem::deleteParticle(unsigned int which) {
    if (which < particles.size()) {
        Particle* deleted_particle = particles[which];
        for (std::list<Particle*>::iterator iter = deleted_particle->connected_from.begin();
             iter != deleted_particle->connected_from.end();
             ++iter) {
            (*iter)->connected.remove(deleted_particle);
        }
        for (std::list<Particle*>::iterator iter = deleted_particle->connected.begin();
             iter != deleted_particle->connected.end();
             ++iter) {
            (*iter)->connected_from.remove(deleted_particle);
        }
        particle_map.erase(particle_map.find(particles[which]->id));
        std::swap(particles[which], particles.back());
        delete particles.back();
        particles.pop_back();
    }
}

unsigned ParticleSystem::CreateID() {
    ++last_id_created;
    return last_id_created;
}

unsigned ParticleSystem::MakeParticle(SceneGraph* scenegraph, const std::string& path, const vec3& pos, const vec3& vel, const vec3& tint) {
    // ParticleTypeRef particle_type_ref = particle_types->ReturnRef(path);
    ParticleTypeRef particle_type_ref = Engine::Instance()->GetAssetManager()->LoadSync<ParticleType>(path);
    Particle* particle = new Particle();
    if (particle_type_ref->ae_ref.valid()) {
        particle->ae_reader.AttachTo(particle_type_ref->ae_ref);
    }
    particle->size = RangedRandomFloat(particle_type_ref->size_range[0], particle_type_ref->size_range[1]) * 0.5f;
    for (int i = 0; i < 4; ++i) {
        particle->color[i] = RangedRandomFloat(particle_type_ref->color_range[0][i], particle_type_ref->color_range[1][i]);
    }
    particle->position = pos;
    particle->velocity = vel;
    for (int i = 0; i < 3; ++i) {
        particle->color[i] *= tint[i];
    }
    particle->particle_type = particle_type_ref;
    particle->collided = false;
    particle->alive_time = 0.0f;
    particle->initial_size = particle->size;
    particle->initial_opacity = particle->color[3];
    if (particle_type_ref->opacity_ramp_time > 0.0f) {
        particle->color[3] = 0.0f;
    }
    if (!particle_type_ref->no_rotation) {
        particle->rotate_speed = RangedRandomFloat(particle_type_ref->rotation_range[0], particle_type_ref->rotation_range[1]);
        particle->rotation = (float)abs(rand() % 720) - 360;
    } else {
        particle->rotate_speed = 0.0f;
        particle->rotation = 0.0f;
    }
    particle->old_position = particle->position;
    particle->old_color = particle->color;
    particle->old_size = particle->size;
    particle->old_rotation = particle->rotation;
    particle->has_last_connected = false;
    int id = CreateID();
    particle->id = id;

    particles.push_back(particle);
    particle_map[particle->id] = particle;
    // Only allow one theora-reading particle at any given time
    if (particle->ae_reader.valid()) {
        for (int i = (int)particles.size() - 2; i >= 0; --i) {
            if (particles[i]->ae_reader.valid()) {
                deleteParticle(i);
            }
        }
    }
    return id;
}

// Dispose of particle system
void ParticleSystem::Dispose() {
    for (auto& particle : particles) {
        delete particle;
    }
    particles.clear();
}

void ParticleSystem::ConnectParticles(unsigned a, unsigned b) {
    ParticleMap::iterator a_iter = particle_map.find(a);
    ParticleMap::iterator b_iter = particle_map.find(b);
    if (a_iter == particle_map.end() || b_iter == particle_map.end()) {
        return;
    }
    a_iter->second->connected.push_back(b_iter->second);
    b_iter->second->connected_from.push_back(a_iter->second);
}

void ParticleSystem::TintParticle(unsigned id, const vec3& color) {
    ParticleMap::iterator iter = particle_map.find(id);
    if (iter != particle_map.end()) {
        Particle& particle = *(iter->second);
        particle.color[0] *= color[0];
        particle.color[1] *= color[1];
        particle.color[2] *= color[2];
        particle.old_color = particle.color;
    }
}

ParticleSystem::~ParticleSystem() {
    Dispose();
}
