//-----------------------------------------------------------------------------
//           Name: Billboard.cpp
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Images that rotate to face the camera
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
#include "Billboard.h"

#include <Graphics/vbocontainer.h>
#include <Graphics/camera.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/textures.h>

#include <Math/overgrowth_geometry.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Internal/profiler.h>

#include <cmath>

static RC_VBOContainer vbo;

void DrawBillboard(const TextureRef& texture_ref, const vec3& pos, float scale, const vec4& color, AlphaMode alpha_mode) {
    if (!texture_ref.valid()) return;
    Camera* cam = ActiveCameras::Get();
    if (!cam->checkSphereInFrustum(pos, scale * 0.25f)) {
        return;
    }
    PROFILER_GPU_ZONE(g_profiler_ctx, "DrawBillboard()");
    if (!vbo->valid()) {
        const float origo[] = {
            0.0f,
            0.0f,
            0.0f,
        };

        vbo->Fill(kVBOFloat | kVBOStatic, sizeof(origo), (void*)&origo[0]);
    }

    Shaders* shaders = Shaders::Instance();
    Graphics* graphics = Graphics::Instance();

    GLState gl_state;
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "GLState");
        gl_state.blend = (alpha_mode == kStraightBlend);
        gl_state.cull_face = false;
        gl_state.depth_write = !gl_state.blend;
        gl_state.depth_test = true;
        if (alpha_mode == kPremultiplied) {
            gl_state.blend_src = GL_ONE;
        }
        CHECK_GL_ERROR();
        graphics->setGLState(gl_state);
    }

    int shader_id;
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Shader");
        if (graphics->use_sample_alpha_to_coverage || gl_state.blend) {
            shader_id = shaders->returnProgram("billboard #ALPHA_TO_COVERAGE #FLIP_Y", Shaders::kGeometry);
        } else {
            shader_id = shaders->returnProgram("billboard #FLIP_Y", Shaders::kGeometry);
        }

        shaders->setProgram(shader_id);
    }

    Textures::Instance()->bindTexture(texture_ref);

    mat4 model;
    model.SetTranslation(pos);

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Uniforms");
        shaders->SetUniformMat4("mv", cam->GetViewMatrix() * model);
        shaders->SetUniformMat4("p", cam->GetProjMatrix());
        shaders->SetUniformVec2("billboardDim", vec2(1, 1));
        vec4 temp_color = color;
        float mult = powf(Graphics::Instance()->hdr_white_point, 2.2f);
        temp_color[0] *= mult;
        temp_color[1] *= mult;
        temp_color[2] *= mult;
        shaders->SetUniformVec4("color", temp_color);
        // shaders->SetUniformVec2("screenDim", vec2(graphics->config_.screen_width(), graphics->config_.screen_height()));
        shaders->SetUniformFloat("scale", scale);
    }

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Attrib");
        int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
        graphics->EnableVertexAttribArray(vert_attrib_id);
        vbo->Bind();
        glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, sizeof(float) * 3, (const void*)0);
    }

    if (graphics->use_sample_alpha_to_coverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "DrawArrays");
        graphics->DrawArrays(GL_POINTS, 0, vbo->size() / sizeof(float) / 3);
    }

    if (graphics->use_sample_alpha_to_coverage) {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    graphics->ResetVertexAttribArrays();
}
