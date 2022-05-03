//-----------------------------------------------------------------------------
//           Name: cubemap.cpp
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
#include "cubemap.h"

#include <Graphics/graphics.h>
#include <Graphics/vbocontainer.h>
#include <Graphics/vboringcontainer.h>
#include <Graphics/textures.h>
#include <Graphics/shaders.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Math/enginemath.h>
#include <Internal/profiler.h>

mat4 GetCubeMapRotation( int i ) {
    mat4 rot[2];
    if (i==0) {
        rot[0].SetRotationY(PI_f*-0.5f);
        rot[1].SetRotationZ(PI_f);
        return rot[0]*rot[1];
    } else if (i==1){
        rot[0].SetRotationY(PI_f*0.5f);
        rot[1].SetRotationZ(PI_f);
        return rot[0]*rot[1];
    } else if (i==2){
        rot[0].SetRotationX(PI_f*0.5f);
        return rot[0];
    } else if (i==3){
        rot[0].SetRotationX(PI_f*-0.5f);
        return rot[0];
    }  else if (i==4){
        rot[0].SetRotationY(PI_f);
        rot[1].SetRotationZ(PI_f);
        return rot[0]*rot[1];
    } else if(i==5){
        rot[0].SetRotationZ(PI_f);
        return rot[0];
    }
    FatalError("Error", "Getting cube map rotation that is not 0-5");
    return mat4();
}

GLuint cube_map_fbo = UINT_MAX;

void CubemapMipChain(TextureRef cube_map_tex_ref, Cubemap::HemisphereEnabled hemisphere_enabled, const vec3 *hemisphere_vec) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "CubemapMipChain");
    int cube_map_tex_id = Textures::Instance()->returnTexture(cube_map_tex_ref);
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    CHECK_GL_ERROR();
    graphics->PushFramebuffer();
    CHECK_GL_ERROR();
    if(cube_map_fbo == UINT_MAX){
        graphics->genFramebuffers(&cube_map_fbo,"cube_map_fbo");
        CHECK_GL_ERROR();
    }
    graphics->bindFramebuffer(cube_map_fbo);
    CHECK_GL_ERROR();
    int tex_width = 128;
    int level = 0;
    float angle = PI_f * 0.5f / 32.0f;
    while(tex_width > 1){
        ++level;
        tex_width /= 2;
        angle *= 2.0f;
        for(int i=0; i<6; ++i){
            CHECK_GL_ERROR();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, cube_map_tex_id, level);
            CHECK_GL_ERROR();
            graphics->setViewport(0,0,tex_width,tex_width);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            graphics->Clear(true);
            CHECK_GL_ERROR();
            mat4 rot = GetCubeMapRotation(i);
            CHECK_GL_ERROR();
            int shader_id;
            if(hemisphere_enabled == Cubemap::HEMISPHERE){
                shader_id = shaders->returnProgram("cubemapblur #HEMISPHERE");
                shaders->setProgram(shader_id);
                shaders->SetUniformVec3("hemisphere_dir", *hemisphere_vec);
            } else {
                shader_id = shaders->returnProgram("cubemapblur");
                shaders->setProgram(shader_id);
            }
            CHECK_GL_ERROR();
            shaders->SetUniformMat4("rotate", rot);
            CHECK_GL_ERROR();
            shaders->SetUniformFloat("max_angle", angle);
            CHECK_GL_ERROR();
            shaders->SetUniformFloat("src_mip", (float)(level-1));
            CHECK_GL_ERROR();
            Textures::Instance()->bindTexture(cube_map_tex_ref);
            CHECK_GL_ERROR();
            static const GLfloat data[] = {
                -1, -1,
                 1, -1,
                 1,  1,
                -1,  1
            };
            static const GLuint indices[] = {
                0, 1, 2, 0, 3, 2
            };
            static VBOContainer data_vbo;
            static VBOContainer index_vbo;
            if(!data_vbo.valid()){
                data_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(data), (void*)data);
                index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
            }
            data_vbo.Bind();
            index_vbo.Bind();
            int vert_attrib_id = shaders->returnShaderAttrib("vertex", shader_id);
            graphics->EnableVertexAttribArray(vert_attrib_id);
            glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);
            graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            graphics->ResetVertexAttribArrays();
            CHECK_GL_ERROR();
        }
    }
    graphics->PopFramebuffer();
}
