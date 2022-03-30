//-----------------------------------------------------------------------------
//           Name: bloodsurface.cpp
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
#include "bloodsurface.h"

#include <Graphics/graphics.h>
#include <Graphics/textures.h>
#include <Graphics/model.h>
#include <Graphics/particles.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/vboringcontainer.h>
#include <Graphics/shaders.h>

#include <Math/vec3.h>
#include <Math/vec2math.h>
#include <Math/vec3math.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Internal/profiler.h>
#include <Internal/timer.h>

#include <Main/scenegraph.h>
#include <Physics/physics.h>
#include <Wrappers/glm.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

extern Timer game_timer;
extern bool g_debug_runtime_disable_blood_surface_pre_draw;

BloodSurface::BloodSurface(TransformedVertexGetter* transformed_vertex_getter):
    drip_update_delay(0),
    dry_delay(0),
    dry_steps(0),
    framebuffer_created(false),
    transformed_vertex_getter_(transformed_vertex_getter),
    simple_2d_id(0),
    simple_2d_stab_id(0),
    character_accumulate_id(0),
    ignite_id(0),
    extinguish_id(0),
	blood_tex_mipmap_dirty(false),
    blood_framebuffer(INVALID_FRAMEBUFFER),
    blood_work_framebuffer(INVALID_FRAMEBUFFER),
    simple_2d("simple_2d"),
    simple_2d_stab_texture("simple_2d #STAB #TEXTURE"),
    character_accumulate_dry("character_accumulate #DRY"),
    character_accumulate_ignite("character_accumulate #IGNITE"),
    character_accumulate_extinguish("character_accumulate #EXTINGUISH")
{
}

BloodSurface::~BloodSurface() {
	LOG_ASSERT(!blood_tex.valid());
	LOG_ASSERT(!blood_work_tex.valid());
	LOG_ASSERT(blood_framebuffer == INVALID_FRAMEBUFFER);
	LOG_ASSERT(blood_work_framebuffer == INVALID_FRAMEBUFFER);
}

void BloodSurface::Dispose()
{
	blood_tex.clear();
	blood_work_tex.clear();
	if(blood_framebuffer != INVALID_FRAMEBUFFER){
		Graphics::Instance()->deleteFramebuffer(&blood_framebuffer);
		blood_framebuffer = -1;
	}
	if(blood_work_framebuffer != INVALID_FRAMEBUFFER){
		Graphics::Instance()->deleteFramebuffer(&blood_work_framebuffer);
		blood_work_framebuffer = -1;
	}
}

void BloodSurface::InitShaders() {
    simple_2d_id = Shaders::Instance()->returnProgram(simple_2d);
    simple_2d_stab_id = Shaders::Instance()->returnProgram(simple_2d_stab_texture);
    character_accumulate_id = Shaders::Instance()->returnProgram(character_accumulate_dry);
    ignite_id = Shaders::Instance()->returnProgram(character_accumulate_ignite);
    extinguish_id = Shaders::Instance()->returnProgram(character_accumulate_extinguish);

    if (!blood_uniforms_inited) {
        InitUniforms();
        blood_uniforms_inited = true;
    }
}

void BloodSurface::InitUniforms() {
    // Pre-warm uniforms
    Shaders::Instance()->setProgram(simple_2d_id);
    Shaders::Instance()->returnShaderVariable("mvp_mat", simple_2d_id);
    Shaders::Instance()->returnShaderVariable("color", simple_2d_id);
    Shaders::Instance()->setProgram(simple_2d_stab_id);
    Shaders::Instance()->returnShaderVariable("mvp_mat", simple_2d_stab_id);
    Shaders::Instance()->returnShaderVariable("color", simple_2d_stab_id);
    Shaders::Instance()->setProgram(character_accumulate_id);
    Shaders::Instance()->returnShaderVariable("tex_width", character_accumulate_id);
    Shaders::Instance()->returnShaderVariable("tex_height", character_accumulate_id);
    Shaders::Instance()->returnShaderVariable("time", character_accumulate_id);
    Shaders::Instance()->setProgram(ignite_id);
    Shaders::Instance()->returnShaderVariable("tex_width", ignite_id);
    Shaders::Instance()->returnShaderVariable("tex_height", ignite_id);
    Shaders::Instance()->returnShaderVariable("time", ignite_id);
    Shaders::Instance()->setProgram(extinguish_id);
    Shaders::Instance()->returnShaderVariable("tex_width", extinguish_id);
    Shaders::Instance()->returnShaderVariable("tex_height", extinguish_id);
    Shaders::Instance()->returnShaderVariable("time", extinguish_id);
}

void BloodSurface::PreDrawFrame(int width, int height) {
    if (g_debug_runtime_disable_blood_surface_pre_draw) {
        return;
    }

    if(!framebuffer_created){
        CHECK_GL_ERROR();
        Textures *textures = Textures::Instance();
        CHECK_GL_ERROR();
        blood_tex = textures->makeTextureColor(width,height,GL_RGBA,GL_RGBA,0.0f,0.0f,0.0f,0.0f,true);
        textures->SetTextureName(blood_tex, "Blood Surface");
        blood_work_tex = textures->makeTextureColor(width,height,GL_RGBA,GL_RGBA,0.0f,0.0f,0.0f,0.0f,true);
        textures->SetTextureName(blood_work_tex, "Blood Surface - Work");
        CHECK_GL_ERROR();
        Graphics *graphics = Graphics::Instance();
        graphics->PushFramebuffer();
        CHECK_GL_ERROR();
        graphics->genFramebuffers(&blood_framebuffer,"blood_framebuffer");
        CHECK_GL_ERROR();
        graphics->bindFramebuffer(blood_framebuffer);
        graphics->framebufferColorTexture2D(blood_tex);
        CHECK_FBO_ERROR();
        CHECK_GL_ERROR();
        graphics->genFramebuffers(&blood_work_framebuffer, "blood_work_framebuffer");
        CHECK_GL_ERROR();
        graphics->bindFramebuffer(blood_work_framebuffer);
        graphics->framebufferColorTexture2D(blood_work_tex);
        CHECK_FBO_ERROR();
        CHECK_GL_ERROR();
        graphics->PopFramebuffer();
        framebuffer_created = true;
        CHECK_GL_ERROR();

        InitShaders();
    }
}

void BloodSurface::AttachToModel( Model * attach_model ) { 
    model = attach_model;
    model_surface_walker.AttachTo(attach_model->vertices, attach_model->faces);
}

vec2 GetTexCoords(Model *model, int tri, vec3 weights){
    unsigned tri_index = tri*3;
    const std::vector<GLuint> &faces = model->faces;
    const std::vector<GLfloat> &tc = model->tex_coords;
    vec2 tex_coords[3]; 
    for(int i=0; i<3; ++i){
        int index = faces[tri_index+i]*2;
        for(int j=0; j<2; ++j){
            tex_coords[i][j] = tc[index+j];
        }
    }
    return tex_coords[0] * weights[0] + tex_coords[1] * weights[1] + tex_coords[2] * weights[2];
}

const int DRIP_DELAY = 4;

void BloodSurface::Update(SceneGraph *scenegraph, float timestep, bool force_update) {
    std::vector<WalkLine> trace[3]; // Trails left since the last blood update
    if(drip_update_delay <= 0){ // Update blood if it is time
        PROFILER_ZONE(g_profiler_ctx, "Update drips");
        vec3 points[3];
        for(SurfaceWalkerList::iterator iter = surface_walkers.begin(); iter != surface_walkers.end();) {
            if(iter->delay > 0.0f){ // This walker does not need to be updated yet
                iter->delay -= timestep * DRIP_DELAY;
                ++iter;
                continue;
            }
            if((*iter).amount < 0.0f){ // This walker ran out of liquid, so delete it
                iter = surface_walkers.erase(iter);
                continue;
            }
            (*iter).amount -= timestep;
            // Move walker across surface
            SurfaceWalker& sw = (*iter); 
            float drip_dist = 0.005f;
            float total_moved = 0.0f;
            int max_triangles = 10;
            int num_triangles = 0; 
            int at_drip_point = -1;
            while(total_moved < 1.0f && num_triangles < max_triangles){
                LOG_ASSERT_LT(sw.tri, (int)model->faces.size()/3);
                for(int tri_vert=0; tri_vert<3; ++tri_vert){
                    LOG_ASSERT_LT(sw.tri*3+tri_vert, (int)model->faces.size());
                    LOG_ASSERT_LT(model->faces[sw.tri*3+tri_vert], model->vertices.size()/3);
                    if(model->faces[sw.tri*3+tri_vert] > model->vertices.size()){
                        LOGE << "sw.tri: " << sw.tri << "tri_vert: " << tri_vert << "index: " << sw.tri*3+tri_vert << "model->faces.size()*3: " << model->faces.size() << "model->faces[sw.tri*3+tri_vert]: " << model->faces[sw.tri*3+tri_vert] << std::endl;
                    }
                    points[tri_vert] = transformed_vertex_getter_->GetTransformedVertex(model->faces[sw.tri*3+tri_vert]);
                }
                //DebugDraw::Instance()->AddWireSphere(points[0], 0.05f, vec4(1.0f), _delete_on_update);
                SWresults swr;

                switch(iter->type){
                case SurfaceWalker::FIRE:
                    swr = model_surface_walker.Move(sw, points, vec3(0.0f,1.0f,0.0f), drip_dist, &trace[2]);
                    break;
                case SurfaceWalker::WATER:
                    swr = model_surface_walker.Move(sw, points, vec3(0.0f,-1.0f,0.0f), drip_dist, &trace[1]);
                    break;
                case SurfaceWalker::BLOOD:
                    swr = model_surface_walker.Move(sw, points, vec3(0.0f,-1.0f,0.0f), drip_dist, &trace[0]);
                    break;
                }

                total_moved += swr.dist_moved;
                if(swr.at_point != -1){
                    at_drip_point = swr.at_point;
                    break;
                }
                ++num_triangles;
            }
            if(num_triangles >= max_triangles){
                // We touched too many triangles
                LOGE << "Max reps exceeded" << std::endl;
            }


            if(at_drip_point == -1){
                ++iter;
            } else {
                if(iter->can_drip){
                    for(int tri_vert=0; tri_vert<3; ++tri_vert){
                        points[tri_vert] = transformed_vertex_getter_->GetTransformedVertex(model->faces[sw.tri*3+tri_vert]);
                    }
                    vec3 point = points[0] * sw.pos[0] +
                                 points[1] * sw.pos[1] +
                                 points[2] * sw.pos[2]; 
                    if(iter->type == SurfaceWalker::BLOOD){
                        scenegraph->particle_system->MakeParticle(scenegraph,
                            "Data/Particles/blooddrop.xml",
                            point,
                            vec3(0.0f),
                            Graphics::Instance()->config_.blood_color());
                    } else if(iter->type == SurfaceWalker::WATER){
                        scenegraph->particle_system->MakeParticle(scenegraph,
                            "Data/Particles/waterdrop.xml",
                            point,
                            vec3(0.0f),
                            vec3(1.0f));
                    }
                }
                iter = surface_walkers.erase(iter);
            }
        }
        drip_update_delay = DRIP_DELAY;
    }
    drip_update_delay--; 
    
    glm::mat4 proj = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

    bool needs_update = (!trace[0].empty() ||
                         !trace[1].empty() ||
                         !trace[2].empty() ||
                         !cut_lines.empty());
    if((framebuffer_created && needs_update) || force_update){
        PROFILER_ZONE(g_profiler_ctx, "Update texture with drips and cuts");
        StartDrawingToBloodTexture();        
        Shaders::Instance()->setProgram(simple_2d_id);
        float ts = 1.0f/(float)Textures::Instance()->getWidth(blood_tex);
        Graphics::Instance()->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Shaders::Instance()->SetUniformMat4("mvp_mat", (const GLfloat*)&proj);
        Graphics::Instance()->SetLineWidth(1);
        std::vector<float> blood_points;
        for(int i=0; i<3; ++i){
            if(trace[i].empty() && (i!=0 || cut_lines.empty())){
                continue;
            }
            switch(i){
            case 0:
                Shaders::Instance()->SetUniformVec4("color", vec4(1.0f,253.0f/255.0f,0.0f,1.0f));
                break;
            case 1:
                Shaders::Instance()->SetUniformVec4("color", vec4(0.0f,253.0f/255.0f,0.0f,1.0f));
                break;
            case 2:
                Shaders::Instance()->SetUniformVec4("color", vec4(0.0f,0.0f,1.0f,1.0f));
                break;
            }

            PROFILER_ENTER(g_profiler_ctx, "Fill blood_points in Update texture");
            for(std::vector<WalkLine>::iterator iter = trace[i].begin(); iter != trace[i].end(); ++iter) {
                const int face_index = (*iter).tri*3;
                vec2 tex_coords[3];
                for(int i=0; i<3; ++i){
                    int index = model->faces[face_index+i]*2;
                    for(int j=0; j<2; ++j){
                        tex_coords[i][j] = model->tex_coords[index+j];
                    }
                }

                const vec3 &start = (*iter).start;
                vec2 start_uv = tex_coords[0] * start[0] +
                    tex_coords[1] * start[1] +
                    tex_coords[2] * start[2];

                const vec3 &end = (*iter).end;
                vec2 end_uv = tex_coords[0] * end[0] +
                    tex_coords[1] * end[1] +
                    tex_coords[2] * end[2];
                blood_points.reserve(blood_points.size()+40);
                for(unsigned i=0; i<10; ++i){
                    vec2 test_offset(RangedRandomFloat(-ts,ts),
                        RangedRandomFloat(-ts,ts));
                    blood_points.push_back(start_uv[0]+test_offset[0]);
                    blood_points.push_back(start_uv[1]+test_offset[1]);
                    blood_points.push_back(end_uv[0]+test_offset[0]);
                    blood_points.push_back(end_uv[1]+test_offset[1]);
                }
            }
            if(i==0){
                blood_points.reserve(blood_points.size()+cut_lines.size()*40);
                for(std::vector<CutLine>::iterator iter = cut_lines.begin();
                    iter != cut_lines.end();
                    ++iter)
                {
                    for(unsigned i=0; i<10; ++i){
                        vec2 test_offset(RangedRandomFloat(-ts,ts),
                            RangedRandomFloat(-ts,ts));
                        blood_points.push_back(iter->start[0]+test_offset[0]);
                        blood_points.push_back(iter->start[1]+test_offset[1]);
                        blood_points.push_back(iter->end[0]+test_offset[0]);
                        blood_points.push_back(iter->end[1]+test_offset[1]);
                    }
                }
                cut_lines.clear();
            }
            PROFILER_LEAVE(g_profiler_ctx);

            PROFILER_ENTER(g_profiler_ctx, "Fill VBO in Update texture");
            static VBORingContainer blood_point_vbo(V_MIBIBYTE, kVBODynamic | kVBOFloat);
            blood_point_vbo.Fill(sizeof(float) * blood_points.size(), (void*)&blood_points[0]);
            blood_point_vbo.Bind();
            PROFILER_LEAVE(g_profiler_ctx);

            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw arrays in Update texture");
            int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_coord", simple_2d_id);
            Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
            glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2*sizeof(float), (const void*)blood_point_vbo.offset());
            Graphics::Instance()->DrawArrays(GL_LINES, 0, blood_points.size()/2);
            Graphics::Instance()->ResetVertexAttribArrays();
        }
        dry_steps = 50;

		EndDrawingToBloodTexture();
		blood_tex_mipmap_dirty = true;
    }
    if((framebuffer_created/* && blood_dry_steps > 0 */&& dry_delay <= 0) || force_update){
        PROFILER_GPU_ZONE(g_profiler_ctx, "Update dry");
        std::swap(blood_tex, blood_work_tex);
        std::swap(blood_framebuffer, blood_work_framebuffer);
        StartDrawingToBloodTexture();

        const bool kCheckForWetnessDrips = false;
        if(kCheckForWetnessDrips){
            int dims[2] = {Textures::Instance()->getWidth(blood_tex), Textures::Instance()->getHeight(blood_tex)};
            std::vector<GLubyte> bytes (dims[0] * dims[1] * 4);
            PROFILER_ENTER(g_profiler_ctx, "Read pixels in Update dry");
            glReadPixels(0, 0, dims[0], dims[1], GL_RGBA, GL_UNSIGNED_BYTE, &bytes[0]);
            PROFILER_LEAVE(g_profiler_ctx);
            int total_wet = 0;
            int total_possible = 0;
            for(int i=0, len=bytes.size(); i<len; i+=4){
                if(bytes[i+3] == 255){
                    total_wet += bytes[i+1];
                    total_possible += 255;
                }
            }
            for(int i=0, len=model->faces.size(); i<len; i+=3){
                if(rand()%20 == 0){
                    vec2 center;
                    for(int j=0; j<3; ++j){
                        int vert_index = model->faces[i+j]*2;
                        for(int k=0; k<2; ++k){
                            center[k] += model->tex_coords[vert_index+k];
                        }
                    }
                    center /= 3.0f;
                    int u = (int) (center[0] * dims[0] + 0.5f);
                    int v = (int) (center[1] * dims[1] + 0.5f);
                    if(u >= 0 && u < dims[0] && v >= 0 && v < dims[1]){
                        int index = (u + v * dims[0])*4;
                        int wet = bytes[index+1];
                        if(wet == 254){
                            CreateDripInTri(i/3, vec3(1.0f/3.0f), 1.0f, RangedRandomFloat(0.0f, 4.0f), true, SurfaceWalker::WATER);
                        }
                    }
                }
                /*
                if(rand()%500 == 0){
                    vec2 center;
                    for(int j=0; j<3; ++j){
                        int vert_index = model->faces[i+j]*2;
                        for(int k=0; k<2; ++k){
                            center[k] += model->tex_coords[vert_index+k];
                        }
                    }
                    center /= 3.0f;
                    int u = center[0] * dims[0] + 0.5f;
                    int v = center[1] * dims[1] + 0.5f;
                    if(u >= 0 && u < dims[0] && v >= 0 && v < dims[1]){
                        int index = (u + v * dims[0])*4;
                        int wet = bytes[index+1];
                        if(bytes[index+2] == 255){ // fire
                            CreateDripInTri(i/3, vec3(1.0f/3.0f), 1.0f, RangedRandomFloat(0.0f, 4.0f), true, SurfaceWalker::FIRE);
                        }
                    }
                }*/
            }
            LOGI << "Wetness: " << (int)(total_wet * 100.0f / total_possible) << "%" << std::endl;
        }
		{
			PROFILER_ZONE(g_profiler_ctx, "Setup");
			Graphics::Instance()->setBlend(false);
			Graphics::Instance()->SetBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			Shaders::Instance()->setProgram(character_accumulate_id);
			Shaders::Instance()->SetUniformInt("tex_width", Textures::Instance()->getWidth(blood_work_tex));
			Shaders::Instance()->SetUniformInt("tex_height", Textures::Instance()->getHeight(blood_work_tex));
			Shaders::Instance()->SetUniformFloat("time", game_timer.game_time);
			Textures::Instance()->bindTexture(blood_work_tex);

			static const GLfloat data[] = {0,0, 1,0, 1,1, 0,1};
			static const GLuint index[] = {0,1,2, 0,2,3};
			static VBOContainer square_vbo;
			static VBOContainer index_vbo;
			if(!square_vbo.valid()){
				square_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(data), (void*)data);
				index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(index), (void*)index);
			}
			square_vbo.Bind();
			index_vbo.Bind();

			int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_coord", character_accumulate_id);
			Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
			glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);
		}
		{
			PROFILER_ZONE(g_profiler_ctx, "Draw");
			Graphics::Instance()->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		{
			PROFILER_ZONE(g_profiler_ctx, "End");
			Graphics::Instance()->ResetVertexAttribArrays();

			EndDrawingToBloodTexture();
		}
		blood_tex_mipmap_dirty = true;
        --dry_steps;
        dry_delay = 6;
    }
    dry_delay--; 
}

void BloodSurface::CreateBloodDripNearestPointDirection( const vec3 &bone_pos, 
                                                         const vec3 &direction, 
                                                         float blood_amount, 
                                                         bool can_drip )
{
    if(Graphics::Instance()->config_.blood() != BloodLevel::kFull){
        return;
    }
    int closest_tri = -1;
    float best_val;
    float val;
    vec3 center;
    for(int i=0, len=model->faces.size(); i<len; i+=3){
        vec3 center;
        for(int j=0; j<3; ++j){
            int vert_index = model->faces[i+j]*3;
            for(int k=0; k<3; ++k){
                center[k] += model->vertices[vert_index+k];
            }
        }
        center /= 3.0f;
        val = dot(normalize(center-bone_pos), direction);
        if(closest_tri == -1 || val > best_val){
            best_val = val;
            closest_tri = i/3;
        }
    }



    if(closest_tri == -1){
        return;
    }
    SurfaceWalker new_walker;
    new_walker.tri = closest_tri;
    new_walker.last_tri = -1;
    new_walker.pos = vec3(1.0f/3.0f);
    new_walker.amount = blood_amount;
    new_walker.can_drip = can_drip;
    new_walker.type = SurfaceWalker::BLOOD;
    surface_walkers.push_back(new_walker);
}


void BloodSurface::CreateDripInTri( int tri, 
                                         const vec3 &bary_coords,
                                         float blood_amount,
                                         float delay,
                                         bool can_drip,
                                         SurfaceWalker::Type type)
{
    if(Graphics::Instance()->config_.blood() != BloodLevel::kFull){
        return;
    }
    LOG_ASSERT_LT(tri, (int)model->faces.size()/3);
    SurfaceWalker new_walker;
    new_walker.tri = tri;
    new_walker.last_tri = -1;
    new_walker.pos = bary_coords;
    new_walker.amount = blood_amount;
    new_walker.can_drip = can_drip;
    new_walker.delay = delay;
    new_walker.type = type;
    surface_walkers.push_back(new_walker);
}

void BloodSurface::CreateBloodDripNearestPointTransformed( const vec3 &bone_pos, 
                                                float blood_amount, 
                                                bool can_drip )
{
    if(Graphics::Instance()->config_.blood() != BloodLevel::kFull){
        return;
    }
    int closest_tri = -1;
    float best_val;
    float val;
    for(int i=0, len=model->faces.size(); i<len; i+=3){
        vec3 center;
        for(int j=0; j<3; ++j) {
            center += transformed_vertex_getter_->GetTransformedVertex(model->faces[i+j]);
        }
        center /= 3.0f;
        val = distance_squared(center,bone_pos);
        if(closest_tri == -1 || val < best_val){
            best_val = val;
            closest_tri = i/3;
        }
    }

    if(closest_tri == -1){
        return;
    }
    SurfaceWalker new_walker;
    new_walker.tri = closest_tri;
    new_walker.last_tri = -1;
    new_walker.pos = vec3(1.0f/3.0f);
    new_walker.amount = blood_amount;
    new_walker.can_drip = can_drip;
    new_walker.type = SurfaceWalker::BLOOD;
    surface_walkers.push_back(new_walker);
}

void BloodSurface::CleanBlood() {
    if(framebuffer_created){
        Graphics::Instance()->PushFramebuffer();
        int width = Textures::Instance()->getWidth(blood_tex);
        int height = Textures::Instance()->getHeight(blood_tex);
        Graphics::Instance()->bindFramebuffer(blood_framebuffer);
        Graphics::Instance()->StartTextureSpaceRenderingCustom(0, 0, width, height);
        CHECK_FBO_ERROR();

        glClearColor(0.0f,0.0f,0.0f,0.0f);
        glColorMask(true, false, false, false);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(true, true, true, true);
        surface_walkers.clear();
        dry_steps = 0;

        Graphics::Instance()->EndTextureSpaceRendering();
		Graphics::Instance()->PopFramebuffer();
		blood_tex_mipmap_dirty = true;
    }
}


void BloodSurface::SetFire(float fire) {
    if(framebuffer_created){
        Graphics::Instance()->PushFramebuffer();
        int width = Textures::Instance()->getWidth(blood_tex);
        int height = Textures::Instance()->getHeight(blood_tex);
        Graphics::Instance()->bindFramebuffer(blood_framebuffer);
        Graphics::Instance()->StartTextureSpaceRenderingCustom(0, 0, width, height);
        CHECK_FBO_ERROR();

        glClearColor(0.0f,
                     0.0f,
                     fire,
                     0.0f);
        glColorMask(false, false, true, false);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(true, true, true, true);

        Graphics::Instance()->EndTextureSpaceRendering();
		Graphics::Instance()->PopFramebuffer();
		blood_tex_mipmap_dirty = true;
    }
}

void BloodSurface::SetWet(float wet) {
    if(framebuffer_created){
        Graphics::Instance()->PushFramebuffer();
        int width = Textures::Instance()->getWidth(blood_tex);
        int height = Textures::Instance()->getHeight(blood_tex);
        Graphics::Instance()->bindFramebuffer(blood_framebuffer);
        Graphics::Instance()->StartTextureSpaceRenderingCustom(0, 0, width, height);
        CHECK_FBO_ERROR();

        glClearColor(0.0f,
            wet,
            0.0f,
            0.0f);
        glColorMask(false, true, false, false);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(true, true, true, true);

        Graphics::Instance()->EndTextureSpaceRendering();
		Graphics::Instance()->PopFramebuffer();
		blood_tex_mipmap_dirty = true;
    }
}

void BloodSurface::AddBloodLine( const vec2 &start, const vec2 &end ) {
    if(Graphics::Instance()->config_.blood() == BloodLevel::kNone){
        return;
    }
    cut_lines.resize(cut_lines.size()+1);
    cut_lines.back().start = start;
    cut_lines.back().end = end;
}

struct tri_data {
    vec3 p[3];
    vec2 decal_coord[3];
};

void BloodSurface::AddDecalToTriangles( const std::vector<int> &hit_list, vec3 pos, vec3 dir, float size, const TextureAssetRef& texture_ref ) {
	if(!framebuffer_created){
		return;
	}
    PROFILER_ZONE(g_profiler_ctx, "BloodSurface::AddDecalToTriangles");

    PROFILER_ENTER(g_profiler_ctx, "Get transformed verts");
    std::vector<tri_data> tris(hit_list.size());
    for(unsigned i=0; i<hit_list.size(); ++i){
        tris[i].p[0] = transformed_vertex_getter_->GetTransformedVertex(model->faces[hit_list[i]*3+0]);
        tris[i].p[1] = transformed_vertex_getter_->GetTransformedVertex(model->faces[hit_list[i]*3+1]);
        tris[i].p[2] = transformed_vertex_getter_->GetTransformedVertex(model->faces[hit_list[i]*3+2]);
    }
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Calculate decal coords");
    vec3 proj_z = dir;
    //vec3 proj_y(0.0f,1.0f,0.0f);
    vec3 proj_y(RangedRandomFloat(-1.0f,1.0f),
                RangedRandomFloat(-1.0f,1.0f),
                RangedRandomFloat(-1.0f,1.0f));
    proj_y = normalize(proj_y);
    vec3 proj_x = normalize(cross(proj_z, proj_y));
    proj_y = normalize(cross(proj_x, proj_z));

    {
        vec3 temp;
        for(unsigned i=0; i<hit_list.size(); ++i){
            for(unsigned j=0; j<3; ++j){
                temp = tris[i].p[j] - pos;
                tris[i].decal_coord[j] = vec2(dot(temp, proj_x) / size + 0.5f, 
                                              dot(temp, proj_y) / size + 0.5f);
            }
        }
    }
    PROFILER_LEAVE(g_profiler_ctx);
    
    PROFILER_ENTER(g_profiler_ctx, "Fill data");
    std::vector<GLfloat> data;
    data.reserve(hit_list.size() * 12);
    for(unsigned i=0; i<hit_list.size(); ++i){
        for(unsigned j=0; j<3; ++j){
            data.push_back(tris[i].decal_coord[j][0]);
            data.push_back(tris[i].decal_coord[j][1]);
            int index = model->faces[hit_list[i]*3+j]*2;
            data.push_back(model->tex_coords[index+0]);
            data.push_back(model->tex_coords[index+1]);
        }
    }
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Setup shader");
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();
    StartDrawingToBloodTexture();
    glm::mat4 proj = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
    shaders->setProgram(simple_2d_stab_id);
    graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
    shaders->SetUniformMat4("mvp_mat", (const GLfloat*)&proj);
    shaders->SetUniformVec4("color", vec4(1.0f,1.0f,0.0f,1.0f));
    textures->bindTexture(texture_ref->GetTextureRef());
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Fill VBO");
    static VBORingContainer data_vbo(V_MIBIBYTE,kVBOFloat | kVBODynamic );
    data_vbo.Fill(sizeof(GLfloat) * data.size(), (void*)&data[0]);
    data_vbo.Bind();
    PROFILER_LEAVE(g_profiler_ctx);
   
    PROFILER_ENTER(g_profiler_ctx, "Draw");
    int vert_attrib_id = shaders->returnShaderAttrib("vert_coord", simple_2d_stab_id);
    int tex_attrib_id = shaders->returnShaderAttrib("tex_coord", simple_2d_stab_id);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    graphics->EnableVertexAttribArray(tex_attrib_id);
    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (const void*)(data_vbo.offset()+2*sizeof(GLfloat)));
    glVertexAttribPointer(tex_attrib_id, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (const void*)(data_vbo.offset()));
    graphics->DrawArrays(GL_TRIANGLES, 0, hit_list.size()*3);
    graphics->ResetVertexAttribArrays();
    EndDrawingToBloodTexture();
    PROFILER_LEAVE(g_profiler_ctx);

	blood_tex_mipmap_dirty = true;
    dry_steps = 50;
}

void BloodSurface::StartDrawingToBloodTexture() {
    PROFILER_ZONE(g_profiler_ctx, "BloodSurface::StartDrawingToBloodTexture");
    PROFILER_ENTER(g_profiler_ctx, "Push framebuffer");
    Graphics* graphics = Graphics::Instance();
    graphics->PushFramebuffer();
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_ENTER(g_profiler_ctx, "bindFramebuffer");
        Graphics::Instance()->bindFramebuffer(blood_framebuffer);
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_ENTER(g_profiler_ctx, "StartTextureSpaceRenderingCustom");
    int width = Textures::Instance()->getWidth(blood_tex);
    int height = Textures::Instance()->getHeight(blood_tex);
    graphics->StartTextureSpaceRenderingCustom(0, 0, width, height);
    CHECK_FBO_ERROR();
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "setGLState");
    GLState gl_state;
    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
    graphics->setGLState(gl_state);
    PROFILER_LEAVE(g_profiler_ctx);
}

void BloodSurface::EndDrawingToBloodTexture() {
    CHECK_GL_ERROR();
    Graphics::Instance()->EndTextureSpaceRendering();
    CHECK_GL_ERROR();
    Graphics::Instance()->PopFramebuffer();
}

void BloodSurface::Ignite() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "BloodSurface::Ignite");
    if(framebuffer_created){
        CHECK_GL_ERROR();
        std::swap(blood_tex, blood_work_tex);
        std::swap(blood_framebuffer, blood_work_framebuffer);
        StartDrawingToBloodTexture();
        Graphics::Instance()->setBlend(false);
        Graphics::Instance()->SetBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        Shaders::Instance()->setProgram(ignite_id);
        Shaders::Instance()->SetUniformInt("tex_width", Textures::Instance()->getWidth(blood_work_tex));
        Shaders::Instance()->SetUniformInt("tex_height", Textures::Instance()->getHeight(blood_work_tex));
        Shaders::Instance()->SetUniformFloat("time", game_timer.game_time);
        Textures::Instance()->bindTexture(blood_work_tex);

        static const GLfloat data[] = {0,0, 1,0, 1,1, 0,1};
        static const GLuint index[] = {0,1,2, 0,2,3};
        static VBOContainer square_vbo;
        static VBOContainer index_vbo;
        if(!square_vbo.valid()){
            square_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(data), (void*)data);
            index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(index), (void*)index);
        }
        square_vbo.Bind();
        index_vbo.Bind();
        CHECK_GL_ERROR();

        int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_coord", ignite_id);
        Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
        glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);
        Graphics::Instance()->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        Graphics::Instance()->ResetVertexAttribArrays();

        CHECK_GL_ERROR();
        EndDrawingToBloodTexture();
        CHECK_GL_ERROR();
		blood_tex_mipmap_dirty = true;
    }
}

void BloodSurface::Extinguish() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "BloodSurface::Extinguish");
    if(framebuffer_created){
        std::swap(blood_tex, blood_work_tex);
        std::swap(blood_framebuffer, blood_work_framebuffer);
        StartDrawingToBloodTexture();
        Graphics::Instance()->setBlend(false);
        Graphics::Instance()->SetBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        Shaders::Instance()->setProgram(extinguish_id);
        Shaders::Instance()->SetUniformInt("tex_width", Textures::Instance()->getWidth(blood_work_tex));
        Shaders::Instance()->SetUniformInt("tex_height", Textures::Instance()->getHeight(blood_work_tex));
        Shaders::Instance()->SetUniformFloat("time", game_timer.game_time);
        Textures::Instance()->bindTexture(blood_work_tex);

        static const GLfloat data[] = {0,0, 1,0, 1,1, 0,1};
        static const GLuint index[] = {0,1,2, 0,2,3};
        static VBOContainer square_vbo;
        static VBOContainer index_vbo;
        if(!square_vbo.valid()){
            square_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(data), (void*)data);
            index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(index), (void*)index);
        }
        square_vbo.Bind();
        index_vbo.Bind();

        int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_coord", extinguish_id);
        Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
        glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);
        Graphics::Instance()->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        Graphics::Instance()->ResetVertexAttribArrays();

		EndDrawingToBloodTexture();
		blood_tex_mipmap_dirty = true;
    }
}

void BloodSurface::GetShaderNames(std::map<std::string, int>& shaders) {
    shaders[simple_2d] = 0;
    shaders[simple_2d_stab_texture] = 0;
    shaders[character_accumulate_dry] = 0;
    shaders[character_accumulate_ignite] = 0;
    shaders[character_accumulate_extinguish] = 0;
}
