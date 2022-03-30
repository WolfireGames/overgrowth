//-----------------------------------------------------------------------------
//           Name: sky.cpp
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
#include "sky.h"

#include <Graphics/camera.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/textures.h>
#include <Graphics/cubemap.h>

#include <Internal/common.h>
#include <Internal/timer.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Main/scenegraph.h>
#include <Images/image_export.hpp>
#include <Math/vec3math.h>
#include <Wrappers/glm.h>
#include <Main/engine.h>
#include <Compat/compat.h>

#include <cmath>

extern Timer game_timer;
extern char* global_shader_suffix;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_debug_runtime_disable_sky_draw;

namespace {
    const int kSkyBoxRes = 512; ///Resolution of each skybox face
    const int kDiffuseBoxRes = 128; ///Resolution of blurred skybox faces

    struct CubemapPixelCoord {
        int x,y,face;
    };

    CubemapPixelCoord PixelCoordFromByte(int index, int tex_width, int bytes_per_face) {    
        CubemapPixelCoord cpc;
        cpc.face = index / bytes_per_face;
        cpc.y = (index - (cpc.face * bytes_per_face)) / (tex_width * 4);
        cpc.x = (index - (cpc.face * bytes_per_face) - (cpc.y * tex_width * 4))/4;
        return cpc;
    }

    int ByteFromPixelCoord(const CubemapPixelCoord &cpc, int tex_width, int bytes_per_face) {
        return cpc.face * bytes_per_face + cpc.y * tex_width * 4 + cpc.x * 4;
    }

    void GetMipmapLevel(std::vector<GLubyte> &pixels, int target) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "GetMipmapLevel");
        int mipmap_width = (int)sqrt(pixels.size()/4/6);
        int mipmap_level = 0;
        std::vector<GLubyte> last_mipmap = pixels;
        while(mipmap_width > 1 && mipmap_level < target){
            int last_mipmap_width = mipmap_width;
            mipmap_width /= 2;
            ++mipmap_level;
            std::vector<GLubyte> new_pixels(mipmap_width*mipmap_width*4*6);

            int dst_index = 0;
            int src_index = 0;
            for (int face=0; face<6; ++face) {
                for(int y=0; y<mipmap_width; ++y){
                    for(int x=0; x<mipmap_width; ++x){
                        int temp_index = src_index;
                        vec3 accum = vec3(last_mipmap[temp_index+0], last_mipmap[temp_index+1], last_mipmap[temp_index+2]);
                        temp_index += 4;
                        accum += vec3(last_mipmap[temp_index+0], last_mipmap[temp_index+1], last_mipmap[temp_index+2]);
                        temp_index = src_index + last_mipmap_width*4;
                        accum += vec3(last_mipmap[temp_index+0], last_mipmap[temp_index+1], last_mipmap[temp_index+2]);
                        temp_index += 4;

                        accum /= 4.0f;
                        new_pixels[dst_index+0] = (GLubyte)accum[0];
                        new_pixels[dst_index+1] = (GLubyte)accum[1];
                        new_pixels[dst_index+2] = (GLubyte)accum[2];
                        new_pixels[dst_index+3] = 255;

                        dst_index += 4;
                        src_index += 8;
                    }
                    src_index += last_mipmap_width*4;
                }
            }
            last_mipmap = new_pixels;
        }
        pixels = last_mipmap;
    }

void ProcessCubeMap(TextureRef src_ref, TextureRef ref, GLuint framebuffer, GLuint framebuffer2) {
    Graphics* graphics = Graphics::Instance();
        PROFILER_GPU_ZONE(g_profiler_ctx, "Sky::ProcessCubeMap");
        // Read source cube map
        int tex_width = Textures::Instance()->getWidth(src_ref);
        // Find out how much source has to be scaled down to match target
        int mip = 0;
        while(tex_width > kDiffuseBoxRes){
            ++mip;
            tex_width /= 2;
        }
        graphics->PushFramebuffer();
        for(int i=0; i<6; ++i){
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+i, Textures::Instance()->returnTexture(src_ref), mip);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer2);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+i, Textures::Instance()->returnTexture(ref), 0);

            //LOG_ASSERT(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            //LOG_ASSERT(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	    //LOGE << "Process cubemap blit from " << Textures::Instance()->returnTexture(src_ref) << " to " <<  Textures::Instance()->returnTexture(ref) << std::endl;
	    //GLint viewport[4];
            //glGetIntegerv(GL_VIEWPORT,viewport);
            //LOGE << "CUrrent viewport " << viewport[0] << ", " << viewport[1] << ", "<< viewport[2] << ", "<< viewport[3] << std::endl;

            glClearColor(1,0,1,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //Intel actually cares about the scissor test flag in the blit, so we have to disable it.
            glDisable(GL_SCISSOR_TEST);
            //We set these incase someone in the future decides blit should care about these too.
            glViewport(0,0,kDiffuseBoxRes,kDiffuseBoxRes);
            glDisable(GL_DEPTH_TEST);
            
            CHECK_FBO_ERROR();
            CHECK_GL_ERROR();
            //glReadBuffer(GL_COLOR_ATTACHMENT0); 
            //glDrawBuffer(GL_COLOR_ATTACHMENT0); 
            glBlitFramebuffer(0, 0, kDiffuseBoxRes, kDiffuseBoxRes, 0, 0, kDiffuseBoxRes, kDiffuseBoxRes, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            
        }
        graphics->PopFramebuffer();
        CubemapMipChain(ref, Cubemap::SPHERE, NULL);
        if(Graphics::Instance()->vendor == _intel){
            CubemapMipChain(ref, Cubemap::SPHERE, NULL); // This has to be called twice on the brix for some reason
        }
    }
} //namespace ""


static glm::mat4 CubeFaceTransform(int i) {
    glm::mat4 mat(1.0f);
    switch(i){
    case 0:
        mat = glm::rotate(mat, -90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        mat = glm::rotate(mat, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    case 1:
        mat = glm::rotate(mat, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        mat = glm::rotate(mat, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    case 2:
        mat = glm::rotate(mat, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 3:
        mat = glm::rotate(mat, 90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 4:
        mat = glm::rotate(mat, -180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        mat = glm::rotate(mat, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    case 5:
        mat = glm::rotate(mat, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    }
    return mat;
}

void Sky::RenderCubeMap(int flags, const TextureRef* land_texture_ref) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Sky::RenderCubeMap");
    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();
    if(!sky_dome_model.VBO_vertices.valid()){
        sky_dome_model.createVBO();
    }
    if(!sky_land_model.VBO_vertices.valid()){
        sky_land_model.createVBO();
    }
    if(!horizon_model.VBO_vertices.valid()){
        horizon_model.createVBO();
    }
    // Set viewport to cube map size
    CHECK_GL_ERROR();
    int size = textures->getWidth(cube_map_texture_ref);
    graphics->PushViewport();
    CHECK_GL_ERROR();
    graphics->setViewport(0, 0, size, size);
    // Set 90 degree projection matrix
    glm::mat4 proj_mat = glm::perspective(90.0f, 1.0f, 0.01f, 100.0f);
    CHECK_GL_ERROR();
    graphics->PushFramebuffer();
    CHECK_GL_ERROR();
    graphics->bindFramebuffer(framebuffer);
    CHECK_GL_ERROR();
    GLuint cube_map_texture = textures->returnTexture(cube_map_texture_ref);
    CHECK_GL_ERROR();
    for (int i = 0; i < 6; i++) {
        // Rotate view to correct orientation for cube map face
        CHECK_GL_ERROR();
        glm::mat4 view_mat = CubeFaceTransform(i);
        CHECK_GL_ERROR();
        // Attach cube map face to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                  GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+i, cube_map_texture, 0);
        // Set up shader
        int shader_id = shaders->returnProgram(simple_tex_3d_flipped);
        CHECK_GL_ERROR();
        shaders->setProgram(shader_id);    
        CHECK_GL_ERROR();
        shaders->SetUniformVec4("color", vec4(1.0f));
        int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
        int tex_coord_id = shaders->returnShaderAttrib("tex_coord", shader_id);
        graphics->BindArrayVBO(0);
        CHECK_GL_ERROR();

        GLState temp_gl_state;
        temp_gl_state.blend          = false;
        temp_gl_state.cull_face      = false;
        temp_gl_state.depth_test     = false;
        temp_gl_state.depth_write    = false;
        graphics->setGLState(temp_gl_state);
        CHECK_GL_ERROR();

        // Clear framebuffer and draw skydome (if valid texture)
        if(sky_texture_ref.valid()){
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw skydome");
            CHECK_GL_ERROR();
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            CHECK_GL_ERROR();

            glm::mat4 model_mat(1.0f);
            model_mat = glm::rotate(model_mat, 180.0f+sky_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
            model_mat = glm::scale(model_mat, glm::vec3(-1.0f, 1.0f, -1.0f));
            glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
            shaders->SetUniformMat4("mvp_mat", (const GLfloat*)&mvp_mat);
            Textures::Instance()->bindTexture(sky_texture_ref);
            CHECK_GL_ERROR();


            graphics->EnableVertexAttribArray(vert_coord_id);
            graphics->EnableVertexAttribArray(tex_coord_id);
            sky_dome_model.VBO_vertices.Bind();
            glVertexAttribPointer(vert_coord_id, 3, GL_FLOAT, false, 3*sizeof(GLfloat), 0);  
            sky_dome_model.VBO_tex_coords.Bind();
            glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);  
            sky_dome_model.VBO_faces.Bind();
            graphics->DrawElements(GL_TRIANGLES, sky_dome_model.faces.size(), GL_UNSIGNED_INT, 0);
            graphics->ResetVertexAttribArrays();
            CHECK_GL_ERROR();
        } else {
            glClearColor(0.15f,0.15f,0.15f,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        CHECK_GL_ERROR();

        if(flags & DRAW_LAND) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw land");
            CHECK_GL_ERROR();
            { // Draw terrain plane
                glm::mat4 model_mat(1.0f);
                model_mat = glm::translate(model_mat, glm::vec3(0.0f, -0.2f, 0.0f));
                glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
                shaders->SetUniformMat4("mvp_mat", (const GLfloat*)&mvp_mat);
                if(land_texture_ref && land_texture_ref->valid()){
                    Textures::Instance()->bindTexture(*land_texture_ref);
                } else {
                    shaders->SetUniformVec4("color", vec4(vec3(0.5f),1.0f));
                    Textures::Instance()->bindBlankTexture();
                }
                graphics->EnableVertexAttribArray(vert_coord_id);
                graphics->EnableVertexAttribArray(tex_coord_id);
                sky_land_model.VBO_vertices.Bind();
                glVertexAttribPointer(vert_coord_id, 3, GL_FLOAT, false, 3*sizeof(GLfloat), 0);  
                sky_land_model.VBO_tex_coords.Bind();
                glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);  
                sky_land_model.VBO_faces.Bind();
                graphics->DrawElements(GL_TRIANGLES, sky_land_model.faces.size(), GL_UNSIGNED_INT, 0);
                graphics->ResetVertexAttribArrays();
            }
            CHECK_GL_ERROR();

            if((flags & DRAW_HORIZON)!=0){
                PROFILER_GPU_ZONE(g_profiler_ctx, "Draw horizon");
                // Draw fog band
                glm::mat4 model_mat(1.0f);
                model_mat = glm::translate(model_mat, glm::vec3(0.0f, -0.017f, 0.0f));
                model_mat = glm::scale(model_mat, glm::vec3(1.0f, 0.3f, 1.0f));
                graphics->setBlend(true);

                int shader_id = shaders->returnProgram(fog);
                shaders->setProgram(shader_id);    
                shaders->SetUniformVec4("color", vec4(1.0f));
                int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
                int tex_coord_id = shaders->returnShaderAttrib("tex_coord", shader_id);

                glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
                shaders->SetUniformMat4("mvp_mat", (const GLfloat*)&mvp_mat);
                shaders->SetUniformMat4("model_mat", (const GLfloat*)&model_mat);
                Textures::Instance()->bindTexture(horizon_texture_ref);
                Textures::Instance()->bindTexture(spec_cube_map_texture_ref, 3);

                graphics->EnableVertexAttribArray(vert_coord_id);
                graphics->EnableVertexAttribArray(tex_coord_id);
                horizon_model.VBO_vertices.Bind();
                glVertexAttribPointer(vert_coord_id, 3, GL_FLOAT, false, 3*sizeof(GLfloat), 0);  
                horizon_model.VBO_tex_coords.Bind();
                glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);  
                horizon_model.VBO_faces.Bind();
                graphics->DrawElements(GL_TRIANGLES, horizon_model.faces.size(), GL_UNSIGNED_INT, 0);
                graphics->ResetVertexAttribArrays();
                CHECK_GL_ERROR();
            }
            CHECK_GL_ERROR();
        }
    }
    // Restore old state
    graphics->PopFramebuffer();
    graphics->PopViewport();
    Textures::Instance()->bindTexture(cube_map_texture_ref);
    {
        PROFILER_ZONE(g_profiler_ctx, "glGenerateMipmap");
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
}

static GLState GetSkyGLState() {
    GLState gl_state;
    gl_state.depth_test = true;
    gl_state.cull_face = false;
    gl_state.depth_write = false;
    gl_state.blend = false;
    return gl_state;
}

void Sky::Draw(SceneGraph* scenegraph) {
    if (g_debug_runtime_disable_sky_draw) {
        return;
    }

    if(load_resources_queued) {
        return;
    }
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();
    Camera* cam = ActiveCameras::Get();
    graphics->setGLState(GetSkyGLState());
    mat4 view_mat = cam->GetViewMatrix();
    mat4 proj_mat = cam->GetProjMatrix();
    mat4 projection_view_mat = proj_mat * view_mat;

    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    FormatString(shader_str[0], kShaderStrSize, "envobject #SKY %s", global_shader_suffix);
    if(displaying_YCOCG_sky){
        // YCOCG_SRGB is ignored when preloading beacuse the use-case is unclear.
        // Perhaps it can be set once? Notice global_shader_suffix should be last for preloading
        FormatString(shader_str[1], kShaderStrSize, "%s #YCOCG_SRGB", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    int the_shader = shaders->returnProgram(shader_str[0]);

    shaders->setProgram(the_shader);
    shaders->SetUniformFloat("time", game_timer.GetRenderTime());
    shaders->SetUniformFloat("fog_amount", scenegraph->fog_amount);
    shaders->SetUniformFloat("haze_mult", scenegraph->haze_mult);
    shaders->SetUniformVec3("ws_light", scenegraph->primary_light.pos);
    shaders->SetUniformVec4("primary_light_color",vec4(scenegraph->primary_light.color, scenegraph->primary_light.intensity));
    shaders->SetUniformVec3("tint", sky_tint);
    shaders->SetUniformMat4("projection_view_mat", projection_view_mat);
    shaders->SetUniformVec3("cam_pos",ActiveCameras::Get()->GetPos());
    textures->bindTexture(cube_map_texture_ref);  
    textures->bindTexture(original_spec_cube_map_texture_ref, 1); 
    textures->bindTexture(spec_cube_map_texture_ref, 2); 

    { // Bind shadow matrices to shader
        std::vector<mat4> shadow_matrix;
        shadow_matrix.resize(4);
        for(int i=0; i<4; ++i){
            shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
        }
        std::vector<mat4> temp_shadow_matrix = shadow_matrix;
        if(g_simple_shadows || !g_level_shadows){
            temp_shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
        }
        shaders->SetUniformMat4Array("shadow_matrix", temp_shadow_matrix);
    }

    if(g_simple_shadows || !g_level_shadows){
        Textures::Instance()->bindTexture(graphics->static_shadow_depth_ref, TEX_SHADOW);
    } else {
        Textures::Instance()->bindTexture(graphics->cascade_shadow_depth_ref, TEX_SHADOW);
    }

    DrawModelVerts(sky_box_model, "vertex_attrib", shaders, graphics, the_shader);
}

void Sky::Dispose() {
    cube_map_texture_ref.clear();
    spec_cube_map_texture_ref.clear();
    original_spec_cube_map_texture_ref.clear();
    Textures::Instance()->DeleteUnusedTextures();    
    sky_dome_model.Dispose();
    sky_land_model.Dispose();
    sky_box_model.Dispose();
	horizon_model.Dispose();
	
	if(framebuffer != INVALID_FRAMEBUFFER){
		Graphics::Instance()->deleteFramebuffer(&framebuffer);
		framebuffer = INVALID_FRAMEBUFFER;
	}
	if(framebuffer2 != INVALID_FRAMEBUFFER){
		Graphics::Instance()->deleteFramebuffer(&framebuffer2);
		framebuffer2 = INVALID_FRAMEBUFFER;
	}
}

void Sky::Reload() {
    char abs_path[kPathSize];
    FindImagePath(dome_texture_name.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);
    int64_t check_modified = GetDateModifiedInt64(abs_path);
    if( check_modified != -1 && check_modified > modified) {
        live_updated = true;
        QueueLoadResources();
    }
}

void Sky::LightingChanged(bool terrain_exists) {
    if(modified != -1){
        live_updated = true;
        cached = false;
        lighting_changed = true;
        QueueLoadResources();
        BakeFirstPass();
        if(!terrain_exists){
            BakeSecondPass(NULL);
        }
    }
}

void Sky::QueueLoadResources() {
    load_resources_queued = true;
}

void Sky::LoadResources() {
	Dispose();
    PROFILER_GPU_ZONE(g_profiler_ctx, "Sky::LoadResources");
    // Scale down skybox if using low-res textures
    int sky_box_res = kSkyBoxRes / Graphics::Instance()->config_.texture_reduction_factor();
    sky_box_res = max(kDiffuseBoxRes, sky_box_res);
    
    // Check if the dds cache files are missing
    //bool missing_a_cubemap = false;
    char abs_path[kPathSize];
    FindImagePath(dome_texture_name.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);
    int64_t src_modified = GetDateModifiedInt64(abs_path);
    int64_t cache_modified = src_modified;
    int64_t curr_modified;
    std::string path[2];
    for(int i=0; i<2; i++){
        switch(i) {
            case 0:
                path[i] = dome_texture_name+"_"+level_name+"_cube.dds";
                break;
            case 1:
                path[i] = dome_texture_name+"_"+level_name+"_cube_blur.dds";
                break;
        }

        // Pick most recently modified cubemap from install dir or user write dir
        FindFilePath(path[i].c_str(), abs_path, kPathSize, kDataPaths | kModPaths, false);
        curr_modified = GetDateModifiedInt64(abs_path);
        FindFilePath(path[i].c_str(), abs_path, kPathSize, kWriteDir | kModWriteDirs, false);
        int64_t write_modified = GetDateModifiedInt64(abs_path);
        if(curr_modified == -1 || write_modified > curr_modified) {
            curr_modified = write_modified;
        }
        cache_modified = min(cache_modified, curr_modified);
        if(curr_modified == -1) {
            //missing_a_cubemap = true;
        } 
    }
    
    // If source texture not found, or cubemap is up to date, then load the cubemap
    /*if(!missing_a_cubemap && (src_modified == -1 || (src_modified <= cache_modified && !lighting_changed))) {
        modified = cache_modified;
        cube_map_texture_ref = Textures::Instance()->returnTextureAssetRef(path[0].c_str());
        spec_cube_map_texture_ref = Textures::Instance()->returnTextureAssetRef(path[1].c_str(), PX_SRGB);
        cached = true;
        displaying_YCOCG_sky = true;
    } else {*/
        modified = src_modified;
        cached = false;
    //}

    sky_box_model.LoadObj("Data/Models/skybox.obj");

    // Allocate resources for skybox creation and blurring
    if(!cached) {
        if(sky_dome_model.vertices.empty()){
            sky_dome_model.LoadObj("Data/Models/skydome.obj");
            sky_land_model.LoadObj("Data/Models/skyland.obj");
            horizon_model.LoadObj("Data/Models/horizonband.obj");
        }
    
        Textures::Instance()->setWrap(GL_REPEAT, GL_CLAMP_TO_EDGE);
        Textures::Instance()->setFilters(GL_LINEAR, GL_LINEAR);
        sky_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(dome_texture_name.c_str(), PX_NOCONVERT|PX_SRGB|PX_NOMIPMAP|PX_NOREDUCE, 0x0);

        horizon_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/skies/horizonband_nocompress.tga", PX_SRGB, 0x0);
        Textures::Instance()->setWrap(GL_REPEAT);
        Textures::Instance()->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
        
        cube_map_texture_ref = Textures::Instance()->makeCubemapTexture(sky_box_res, sky_box_res, GL_RGBA16F, GL_RGBA, Textures::MIPMAPS);
        Textures::Instance()->SetTextureName(cube_map_texture_ref, "Sky Cubemap");
        spec_cube_map_texture_ref = Textures::Instance()->makeCubemapTexture(kDiffuseBoxRes, kDiffuseBoxRes, GL_RGBA, GL_RGBA, Textures::MIPMAPS);
        Textures::Instance()->SetTextureName(spec_cube_map_texture_ref, "Sky Spec Cubemap");
        original_spec_cube_map_texture_ref = spec_cube_map_texture_ref;
    }
} 

void Sky::BakeFirstPass() {
    if(load_resources_queued){
        LoadResources();
        load_resources_queued = false;
    }
    if(!cached) {
        if(framebuffer == INVALID_FRAMEBUFFER) {
            Graphics::Instance()->genFramebuffers(&framebuffer, "bake_sky_1");
            Graphics::Instance()->genFramebuffers(&framebuffer2, "bake_sky_2");
        }
        displaying_YCOCG_sky = false;
        RenderCubeMap(0, NULL);
        ProcessCubeMap(cube_map_texture_ref, spec_cube_map_texture_ref, framebuffer, framebuffer2);
    }
}

void Sky::BakeSecondPass(const TextureRef *_land_texture_ref) {
    if(!cached) {
        if(framebuffer == INVALID_FRAMEBUFFER) {
            Graphics::Instance()->genFramebuffers(&framebuffer, "second_bake_sky_1");
            Graphics::Instance()->genFramebuffers(&framebuffer2, "second_bake_sky_2");
        }
                
        RenderCubeMap(DRAW_LAND, _land_texture_ref);
        ProcessCubeMap(cube_map_texture_ref, spec_cube_map_texture_ref, framebuffer, framebuffer2);        
        RenderCubeMap(DRAW_LAND | DRAW_HORIZON, _land_texture_ref);
        ProcessCubeMap(cube_map_texture_ref, spec_cube_map_texture_ref, framebuffer, framebuffer2);

        if(!lighting_changed){
            horizon_texture_ref.clear();
            sky_texture_ref.clear();

            sky_dome_model.Dispose();
            sky_land_model.Dispose();
            horizon_model.Dispose();
        }

        displaying_YCOCG_sky = false;

        Textures::Instance()->DeleteUnusedTextures();

        lighting_changed = false;
    }
    live_updated = false;
    //Textures::Instance()->TextureToVRAM(cube_map_texture_ref);
}

Sky::Sky() : 
    lighting_changed(false),
    framebuffer(INVALID_FRAMEBUFFER),
    framebuffer2(INVALID_FRAMEBUFFER),
    sky_rotation(0.0f),
    live_updated(false),
    sky_tint(1.0f),
    sky_base_tint(1.0f),
    modified(-1),
    simple_tex_3d_flipped("simple_tex_3d #FLIPPED"),
    fog("fog"),
    shader("envobject #SKY")
{}

void Sky::GetShaderNames(std::map<std::string, int>& shader_names) {
    shader_names[simple_tex_3d_flipped] = 0;
    shader_names[fog] = 0;
    shader_names[shader] = SceneGraph::kFullDraw;
}

void Sky::ResetSpecularCubeMapTexture() {
    spec_cube_map_texture_ref = original_spec_cube_map_texture_ref;
}

void Sky::SetSpecularCubeMapTexture( TextureRef new_spec_cube_map_texture_ref ) {
    spec_cube_map_texture_ref = new_spec_cube_map_texture_ref;
}

TextureRef Sky::GetSpecularCubeMapTexture() {
    return spec_cube_map_texture_ref;
}
