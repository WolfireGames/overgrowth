//-----------------------------------------------------------------------------
//           Name: flares.cpp
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
#include "flares.h"

#include <Graphics/shaders.h>
#include <Graphics/graphics.h>
#include <Graphics/camera.h>

#include <Math/vec4.h>
#include <Math/mat4.h>
#include <Math/vec3math.h>

#include <Internal/timer.h>
#include <Internal/profiler.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Main/engine.h>
#include <Main/scenegraph.h>

#include <Physics/bulletworld.h>
#include <Wrappers/glm.h>

#include <opengl.h>

#include <cassert>

extern Timer game_timer;

static const int _occlusion_size = 5;

extern bool g_perform_occlusion_query;
extern bool g_debug_runtime_disable_flares_draw;

Flares::Flares() : animation(0.0f),
                   shader("flare") {
    old_angle = 0.0f;
    new_angle = 90.0f;

    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_write = false;

    shader_id = Shaders::Instance()->returnProgram(shader);
    flare_texture_matte = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/eyeflarematte.tga", PX_SRGB, 0x0);
    flare_texture_streaks = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/eyeflarestreaks_nocompress.tga", PX_SRGB, 0x0);
    flare_texture_blur1 = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/eyeflarestreaksblur1_nocompress.tga", PX_SRGB, 0x0);
    flare_texture_blur2 = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/eyeflarestreaksblur2_nocompress.tga", PX_SRGB, 0x0);
    flare_texture_color = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/eyeflarecolor_nocompress.tga", PX_SRGB, 0x0);
}

Flares::~Flares() {
    CleanupFlares();
}

const float _flare_grow = 0.1f;

static void DrawQuad() {
    Graphics::Instance()->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Flares::DrawFlare(int which) {
    Flare* flare = flares[which];
    if (flare->tex_opac[0] <= 0.0f &&
        flare->tex_opac[1] <= 0.0f &&
        flare->tex_opac[2] <= 0.0f) {
        return;
    }

    if (flare->visible == 0.0f &&
        flare->slow_visible == 0.0f) {
        return;
    }

    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();

    vec3 cam_to_flare;
    vec3 cam_pos = cam->GetPos();

    if (flare->distant) {
        cam_to_flare = flare->position;
    } else {
        cam_to_flare = flare->position - cam_pos;
    }
    cam_to_flare = normalize(cam_to_flare);
    float dot_prod = dot(cam_to_flare, cam->GetFacing());
    if (dot_prod < 0.0f) {
        return;
    }

    float zoom = graphics->config_.screen_height() / 1100.0f;

    glm::mat4 model_view;

    vec3 flare_pos;
    vec3 interp_pos = mix(flare->old_position, flare->position, game_timer.GetInterpWeight());
    if (flare->distant) {
        flare_pos = interp_pos + cam_pos;
    } else {
        flare_pos = interp_pos;
    }
    vec3 projected = cam->ProjectPoint(flare_pos);
    float size = 512 * zoom * flare->size_mult;
    model_view = glm::translate(model_view, glm::vec3(projected[0], projected[1], 0.0f));

    // Draw flare itself
    glm::mat4 model_view2 = glm::scale(model_view, glm::vec3(size * 0.5f, size * 0.5f, 1.0f));
    shaders->SetUniformMat4("modelview_mat", (const GLfloat*)&model_view2);
    float color_alpha;
    float total_opac = flare->tex_opac[0] +
                       flare->tex_opac[1];
    total_opac = max(0.0f, min(1.0f, total_opac));
    float mult = powf(Graphics::Instance()->hdr_white_point, 2.2f);
    if (total_opac > 0.0f) {
        float base_alpha = flare->slow_visible +
                           max(0.0f, (flare->slow_visible - flare->visible));
        color_alpha = base_alpha * flare->brightness * total_opac;
        shaders->SetUniformVec4("color", vec4(flare->color, color_alpha * 0.5f * mult));
        graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textures->bindTexture(flare_texture_matte->GetTextureRef());
        DrawQuad();
    }
    graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
    TextureAssetRef texref;
    for (int i = 0; i < 3; ++i) {
        switch (i) {
            case 0:
                texref = flare_texture_streaks;
                break;
            case 1:
                texref = flare_texture_blur1;
                break;
            case 2:
                texref = flare_texture_blur2;
                break;
        }
        float opac_mult = flare->tex_opac[i];

        textures->bindTexture(texref->GetTextureRef());
        float color_alpha = (1.0f - animation) * flare->visible * flare->brightness * opac_mult;
        shaders->SetUniformVec4("color", vec4(flare->color, color_alpha * mult));
        glm::mat4 scaled_model_view = glm::scale(
            model_view2, glm::vec3(1.0f + animation * _flare_grow, 1.0f + animation * _flare_grow, 1.0f));
        scaled_model_view = glm::rotate(
            scaled_model_view, old_angle, glm::vec3(0.0f, 0.0f, 1.0f));
        shaders->SetUniformMat4("modelview_mat", (const GLfloat*)&scaled_model_view);
        DrawQuad();

        color_alpha = animation * flare->visible * flare->brightness * opac_mult;
        shaders->SetUniformVec4("color", vec4(flare->color, color_alpha * mult));
        scaled_model_view = glm::scale(
            model_view2, glm::vec3(1.0f - _flare_grow + animation * _flare_grow, 1.0f - _flare_grow + animation * _flare_grow, 1.0f));
        scaled_model_view = glm::rotate(
            scaled_model_view, new_angle, glm::vec3(0.0f, 0.0f, 1.0f));
        shaders->SetUniformMat4("modelview_mat", (const GLfloat*)&scaled_model_view);
        DrawQuad();
    }

    // Draw rainbow streaks
    if (flare->tex_opac[0] > 0.0f) {
        textures->bindTexture(flare_texture_color->GetTextureRef());

        color_alpha = dot_prod * dot_prod * flare->visible * flare->brightness * 0.1f * flare->tex_opac[0];
        shaders->SetUniformVec4("color", vec4(flare->color, color_alpha * 0.7f * mult));
        glm::mat4 inner_model_view = glm::translate(
            model_view, glm::vec3(
                            ((graphics->config_.screen_width() / 2) - (GLfloat)projected[0]) * 0.3f,
                            ((graphics->config_.screen_height() / 2) - (GLfloat)projected[1]) * 0.3f,
                            0.0f));
        inner_model_view = glm::scale(
            inner_model_view, glm::vec3(size * 1.5f, size * 1.5f, 1.0f));
        shaders->SetUniformMat4("modelview_mat", (const GLfloat*)&inner_model_view);
        DrawQuad();

        shaders->SetUniformVec4("color", vec4(flare->color, color_alpha * mult));
        inner_model_view = glm::translate(
            model_view, glm::vec3(
                            ((graphics->config_.screen_width() / 2) - (GLfloat)projected[0]) * 0.6f,
                            ((graphics->config_.screen_height() / 2) - (GLfloat)projected[1]) * 0.6f,
                            0.0f));
        inner_model_view = glm::scale(
            inner_model_view, glm::vec3(size * 3.0f, size * 3.0f, 1.0f));
        shaders->SetUniformMat4("modelview_mat", (const GLfloat*)&inner_model_view);
        DrawQuad();
    }
}

void Flares::OcclusionQuery(int which) {
    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();
    Flare* flare = flares[which];
    CHECK_GL_ERROR();
    int active_cam_id = ActiveCameras::Instance()->GetID();
    if ((int)flare->query.size() <= active_cam_id) {
        flare->query.resize(active_cam_id + 1);
    }
    OccQuery& query = flare->query[active_cam_id];
    if (g_perform_occlusion_query) {
        if (!query.created) {
            glGenQueries(1, &query.id);
            query.created = true;
        }
        CHECK_GL_ERROR();
        GLuint count = 0;
        if (query.started) {
            PROFILER_ZONE(g_profiler_ctx, "glGetQueryObjectuiv a");
            glGetQueryObjectuiv(query.id, GL_QUERY_RESULT_AVAILABLE, &count);
        } else {
            count = 1;
        }
        CHECK_GL_ERROR();
        if (count) {
            if (query.started) {
                PROFILER_ZONE(g_profiler_ctx, "glGetQueryObjectuiv b");
                glGetQueryObjectuiv(query.id, GL_QUERY_RESULT, &count);
                unsigned total = _occlusion_size * _occlusion_size *
                                 max(1, graphics->config_.FSAA_samples());
                flare->visible = min(1.0f, max(0.0f, ((float)count) / ((float)total)));
                flare->visible *= flare->visible_mult;
            } else {
                query.started = true;
                flare->visible = 0.0f;
            }
            {
                PROFILER_ZONE(g_profiler_ctx, "glBeginQuery");
                // LOGI << "flare_begin_query " << query.id << std::endl;
                glBeginQuery(GL_SAMPLES_PASSED, query.id);
            }
            if (flare->visible_mult > 0.0f) {
                vec3 cam_to_flare;
                vec3 cam_pos = cam->GetPos();

                if (flare->distant) {
                    cam_to_flare = flare->position;
                } else {
                    cam_to_flare = flare->position - cam_pos;
                }
                cam_to_flare = normalize(cam_to_flare);
                float dot_prod = dot(cam_to_flare, cam->GetFacing());
                if (dot_prod > 0.0f) {
                    CHECK_GL_ERROR();
                    vec3 flare_pos;
                    if (flare->distant) {
                        flare_pos = flare->position + cam_pos;
                    } else {
                        flare_pos = flare->position;
                    }
                    vec3 projected = cam->ProjectPoint(flare_pos);

                    CHECK_GL_ERROR();
                    glm::mat4 modelview;
                    if (flare->distant) {
                        modelview = glm::translate(modelview, glm::vec3(projected[0], projected[1], -99.9f));
                    } else {
                        modelview = glm::translate(modelview, glm::vec3(projected[0], projected[1], projected[2] * -200.0f + 100.0f));
                    }
                    CHECK_GL_ERROR();
                    modelview = glm::scale(modelview, glm::vec3(_occlusion_size * 0.5f, _occlusion_size * 0.5f, 1.0f));
                    shaders->SetUniformMat4("modelview_mat", (const GLfloat*)&modelview);
                    shaders->SetUniformVec4("color", vec4(1.0f, 0.0f, 0.0f, 0.01f));

                    textures->bindBlankTexture();
                    graphics->setDepthTest(true);
                    DrawQuad();
                    graphics->setDepthTest(false);
                    CHECK_GL_ERROR();
                }
            }
            CHECK_GL_ERROR();
            {
                PROFILER_ZONE(g_profiler_ctx, "glEndQuery");
                // LOGI << "flare_end_query " << query.id << std::endl;
                glEndQuery(GL_SAMPLES_PASSED);
            }
            CHECK_GL_ERROR();
        }
    } else {
        flare->visible = 1.0f;
    }
}

void Flares::Draw(Flares::FlareType flare_type) {
    if (g_debug_runtime_disable_flares_draw) {
        return;
    }

    CHECK_GL_ERROR();
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    // Set basic gl state
    graphics->setGLState(gl_state);
    Shaders::Instance()->setProgram(shader_id);
    // Set viewport projection matrix
    glm::mat4 proj;
    proj = glm::ortho(0.0f, (float)graphics->viewport_dim[2], 0.0f, (float)graphics->viewport_dim[3], -100.0f, 100.0f);
    shaders->SetUniformMat4("proj_mat", (const GLfloat*)&proj);
    // Make sure quad-drawing VBOs are filled
    static const GLfloat data[] = {
        0, 0, -1, -1,
        1, 0, 1, -1,
        1, 1, 1, 1,
        0, 1, -1, 1};
    static const GLuint index[] = {
        0, 1, 2, 0, 3, 2};
    if (!vert_vbo.valid()) {
        PROFILER_ZONE(g_profiler_ctx, "vert_vbo fill");
        vert_vbo.Fill(kVBOStatic | kVBOFloat, sizeof(data), (void*)data);
        index_vbo.Fill(kVBOStatic | kVBOElement, sizeof(index), (void*)index);
    }
    CHECK_GL_ERROR();
    // Assign vertex attributes
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shaders->bound_program);
    int tex_attrib_id = shaders->returnShaderAttrib("tex_attrib", shaders->bound_program);
    CHECK_GL_ERROR();
    graphics->EnableVertexAttribArray(vert_attrib_id);
    graphics->EnableVertexAttribArray(tex_attrib_id);
    vert_vbo.Bind();
    index_vbo.Bind();
    CHECK_GL_ERROR();
    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (const void*)(sizeof(GLfloat) * 2));
    glVertexAttribPointer(tex_attrib_id, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), 0);
    CHECK_GL_ERROR();

    for (unsigned i = 0; i < flares.size(); i++) {
        CHECK_GL_ERROR();
        bool to_draw = false;
        if (flare_type == Flares::kSharp) {
            PROFILER_ZONE(g_profiler_ctx, "OcclusionQuery");
            CHECK_GL_ERROR();
            OcclusionQuery(i);
            CHECK_GL_ERROR();
            if (flares[i]->diffuse < _soft_glare_threshold) {
                to_draw = true;
            }
        } else {
            if (flares[i]->diffuse >= _soft_glare_threshold) {
                to_draw = true;
            }
        }
        CHECK_GL_ERROR();
        if (to_draw) {
            PROFILER_ZONE(g_profiler_ctx, "DrawFlare");
            DrawFlare(i);
        }
        CHECK_GL_ERROR();
    }

    // Reset vertex attribute state
    CHECK_GL_ERROR();
    graphics->ResetVertexAttribArrays();
    CHECK_GL_ERROR();
    graphics->BindElementVBO(0);
    graphics->BindArrayVBO(0);
    CHECK_GL_ERROR();
}

const float _jitter = 0.0f;
const float _visible_inertia = 0.6f;
const float _slow_visible_inertia = 0.8f;
const float _flicker_speed = 1.0f;

void Flares::Update(float timestep) {
    animation += timestep * _flicker_speed;
    if (animation > 1.0f) {
        animation -= 1.0f;
        old_angle = new_angle;
        new_angle = RangedRandomFloat(0.0f, 360.0f);
    }
    for (auto flare : flares) {
        flare->old_position = flare->position;
        flare->slow_visible = mix(flare->visible, flare->slow_visible, _slow_visible_inertia);
        if (flare->visible == 0.0f && flare->slow_visible < 0.01f) {
            flare->slow_visible = 0.0f;
        }
        const float _fade_speed = 3.0f;
        const float fade_step = _fade_speed * timestep;
        int curr_tex = -1;
        if (flare->diffuse < _sharp_glare_threshold) {
            curr_tex = 0;
        } else if (flare->diffuse < _soft_glare_threshold) {
            curr_tex = 1;
        } else if (flare->diffuse < _no_glare_threshold) {
            curr_tex = 2;
        }
        for (int i = 0; i < 3; ++i) {
            if (curr_tex == i) {
                flare->tex_opac[i] = min(1.0f, flare->tex_opac[i] + fade_step);
            } else {
                flare->tex_opac[i] = max(0.0f, flare->tex_opac[i] - fade_step);
            }
        }
    }
}

Flare* Flares::MakeFlare(vec3 _position, float _brightness, bool _distant) {
    Flare* flare = new Flare();
    flare->position = _position;
    flare->brightness = _brightness;
    flare->distant = _distant;
    flare->diffuse = 1.0f;
    flare->visible = 0.0f;
    flare->slow_visible = 0.0f;
    flare->color = vec3(1.0f, 1.0f, 1.0f);
    flare->tex_opac[0] = 1.0f;
    flare->tex_opac[1] = 0.0f;
    flare->tex_opac[2] = 0.0f;
    flare->size_mult = 1.0f;
    flare->visible_mult = 1.0f;
    flare->old_position = _position;
    flares.push_back(flare);
    return flare;
}

void Flares::DeleteFlare(Flare* flare) {
    std::vector<Flare*>::iterator flare_iter = std::find(flares.begin(), flares.end(), flare);
    if (flare_iter != flares.end()) {
        Flare* flare = (*flare_iter);
        for (auto& i : flare->query) {
            if (i.created) {
                glDeleteQueries(1, &i.id);
            }
        }
        delete flare;
        flares.erase(flare_iter);
    }
}

void Flares::GetShaderNames(std::map<std::string, int>& preload_shaders) {
    preload_shaders[shader] = 0;
}

void Flares::CleanupFlares() {
    std::vector<Flare*>::iterator it = flares.begin();
    for (; it != flares.end(); ++it) {
        Flare* flare = (*it);
        for (auto& i : flare->query) {
            if (i.created) {
                glDeleteQueries(1, &i.id);
            }
        }
        delete (*it);
    }
    flares.clear();
}

OccQuery::OccQuery() : started(false),
                       created(false),
                       id(0) {
}
