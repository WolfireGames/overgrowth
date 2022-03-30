//-----------------------------------------------------------------------------
//           Name: hudimage.cpp
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
#include "hudimage.h"

#include <Graphics/shaders.h>
#include <Graphics/graphics.h>
#include <Graphics/vboringcontainer.h>
#include <Graphics/text.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Scripting/angelscript/ascontext.h>
#include <Wrappers/glm.h>
#include <Internal/profiler.h>
#include <Main/engine.h>

class HUDImageZCompare {
public:
    bool operator()(const HUDImage &a, const HUDImage &b) {
        return a.pos[2] < b.pos[2];
    }
};

struct HUDImageGLState {
    GLState gl_state;
    HUDImageGLState() {
        gl_state.blend = true;
        gl_state.blend_src = GL_SRC_ALPHA;
        gl_state.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
        gl_state.cull_face = false;
        gl_state.depth_test = false;
        gl_state.depth_write = false;
    }
};

static const HUDImageGLState hud_gl_state;

void HUDImages::Draw() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "HUDImages::Draw()");
	if(hud_images.empty()){
		return;
	}

    CHECK_GL_ERROR();
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();

    {
        PROFILER_ZONE(g_profiler_ctx, "HUDImages sort");
        // Sort HUD images so we can draw them in order of depth
        std::sort(hud_images.begin(), hud_images.end(), HUDImageZCompare());
    }

    PROFILER_ENTER(g_profiler_ctx, "HUDImages setup");
    int shader_id = shaders->returnProgram("simple_2d #TEXTURE #FLIPPED");
    shaders->setProgram(shader_id);
    int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
    int tex_coord_id = shaders->returnShaderAttrib("tex_coord", shader_id);
    int mvp_mat_id = shaders->returnShaderVariable("mvp_mat", shader_id);
    int color_id = shaders->returnShaderVariable("color", shader_id);
    graphics->setGLState(hud_gl_state.gl_state);
    glm::mat4 proj_mat;
    proj_mat = glm::ortho(0.0f, (float)graphics->window_dims[0], 0.0f, (float)graphics->window_dims[1]);
    glUniformMatrix4fv(mvp_mat_id, 1, false, (GLfloat*)&proj_mat);
    graphics->SetClientActiveTexture(0);
    graphics->EnableVertexAttribArray(vert_coord_id);
    graphics->EnableVertexAttribArray(tex_coord_id);

    CHECK_GL_ERROR();
    PROFILER_LEAVE(g_profiler_ctx);

    {
        PROFILER_ZONE(g_profiler_ctx, "HUDImages draw loop");
        for(unsigned i=0; i<hud_images.size(); ++i){
            PROFILER_GPU_ZONE(g_profiler_ctx, "HUDImages draw iteration");
            CHECK_GL_ERROR();
            const HUDImage& hi = hud_images[i];
            if(hi.color[3] <= 0.0f){
                continue;
            }
            int width = textures->getWidth(hi.texture_ref);
            int height = textures->getHeight(hi.texture_ref);
            if(!hi.text){
                // Image need to be scaled up to compensate for texture reduction
                width = textures->getReducedWidth(hi.texture_ref);
                height = textures->getReducedHeight(hi.texture_ref);
                // Image use non-premultiplied alpha
                graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else {
                graphics->SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Text uses premultiplied alpha
            }
            CHECK_GL_ERROR();
            if( hi.texture_ref.valid() ) {
                textures->bindTexture(hi.texture_ref);
                if(hi.tex_scale[0] != 1.0f){
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                } else {
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                }
                if(hi.tex_scale[1] != 1.0f){
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                } else {
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
                }
                glUniform4fv(color_id, 1, (GLfloat*)&hi.color);
                CHECK_GL_ERROR();

                const vec2 &to = hi.tex_offset;
                const vec2 &ts = hi.tex_scale;
                const vec3 &s = hi.scale;
                const vec3 &p = hi.pos;
                float w = (float)width;
                float h = (float)height;
                GLfloat data[] = {
                    to[0],       to[1],       p[0],        p[1],
                    ts[0]+to[0], to[1],       p[0]+w*s[0], p[1],
                    ts[0]+to[0], ts[1]+to[1], p[0]+w*s[0], p[1]+h*s[1],
                    to[0],       ts[1]+to[1], p[0],        p[1]+h*s[1],
                };
                if(hi.text){
                    data[1] = 1.0f - data[1];
                    data[5] = 1.0f - data[5];
                    data[9] = 1.0f - data[9];
                    data[13] = 1.0f - data[13];
                }
                static const GLuint indices[] = {0, 1, 2,  0, 2, 3};
                static VBORingContainer data_vbo(sizeof(data), kVBOFloat | kVBODynamic );
                static VBOContainer index_vbo;
                if(!index_vbo.valid()){
                    index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
                }
                index_vbo.Bind();
                data_vbo.Fill(sizeof(data), (void*)data);
                data_vbo.Bind();
                index_vbo.Bind();
                CHECK_GL_ERROR();
                glVertexAttribPointer(vert_coord_id, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (const void*)(data_vbo.offset()+(2*sizeof(GLfloat))));
                CHECK_GL_ERROR();
                glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (const void*)(data_vbo.offset()));
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            CHECK_GL_ERROR();
        }
    }
    graphics->ResetVertexAttribArrays();
    
    hud_images.clear();
}

HUDImages::HUDImages()
{}

HUDImage* HUDImages::AddImage() {
    hud_images.resize(hud_images.size()+1);
    HUDImage& hud_image = hud_images.back();
    hud_image.pos = vec3(0.0f);
    hud_image.tex_offset = vec2(0.0f);
    hud_image.tex_scale = vec2(1.0f);
    hud_image.scale = vec3(1.0f);
    hud_image.color = vec4(1.0f);
    return &hud_image;
}

void HUDImages::AttachToContext( ASContext *as_context ) {
    as_context->RegisterObjectType("HUDImage", 0, asOBJ_REF | asOBJ_NOCOUNT );
    as_context->RegisterObjectProperty("HUDImage","vec3 position",asOFFSET(HUDImage,pos));
    as_context->RegisterObjectProperty("HUDImage","vec3 scale",asOFFSET(HUDImage,scale));
    as_context->RegisterObjectProperty("HUDImage","vec2 tex_scale",asOFFSET(HUDImage,tex_scale));
    as_context->RegisterObjectProperty("HUDImage","vec2 tex_offset",asOFFSET(HUDImage,tex_offset));
    as_context->RegisterObjectProperty("HUDImage","vec4 color",asOFFSET(HUDImage,color));
    as_context->RegisterObjectMethod("HUDImage", "float GetWidth() const", asMETHOD(HUDImage, GetWidth), asCALL_THISCALL); 
    as_context->RegisterObjectMethod("HUDImage", "float GetHeight() const", asMETHOD(HUDImage, GetHeight), asCALL_THISCALL); 
    as_context->RegisterObjectMethod("HUDImage", "bool SetImageFromPath(const string &in)", asMETHOD(HUDImage, SetImageFromPath), asCALL_THISCALL); 
    as_context->RegisterObjectMethod("HUDImage", "void SetImageFromText(const TextCanvasTexture@)", asMETHOD(HUDImage, SetImageFromText), asCALL_THISCALL); 
    as_context->DocsCloseBrace();

    as_context->RegisterObjectType("HUDImages", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectMethod("HUDImages","HUDImage@ AddImage()",asMETHOD(HUDImages, AddImage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("HUDImages","void Draw()", asMETHOD(HUDImages, Draw), asCALL_THISCALL); 
    as_context->DocsCloseBrace();
    
    as_context->RegisterGlobalProperty("HUDImages hud", this);
}

void HUDImage::SetImageFromText( const TextCanvasTexture *text_canvas_texture) {
    texture_asset.clear();
    texture_ref = text_canvas_texture->GetTexture();
    text = true;
}

bool HUDImage::SetImageFromPath( const std::string &path ) {
    texture_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(path, PX_NOMIPMAP | PX_NOREDUCE, 0x0);
    if( texture_asset.valid() ) {
        texture_ref = texture_asset->GetTextureRef();
        text = false;
        return true;
    } else {
        LOGE << "Loading " << path << "for HUDImage failed, will disable rendering for this object";
        texture_ref.clear(); 
        return false;
    }
}

float HUDImage::GetWidth() const {
    return (float)(Textures::Instance()->getReducedWidth(texture_ref));
}

float HUDImage::GetHeight() const {
    return (float)(Textures::Instance()->getReducedHeight(texture_ref));
}


