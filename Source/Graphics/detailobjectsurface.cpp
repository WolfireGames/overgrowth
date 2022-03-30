//-----------------------------------------------------------------------------
//           Name: detailobjectsurface.cpp
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
#include "detailobjectsurface.h"

#include <Internal/timer.h>
#include <Internal/profiler.h>
#include <Internal/common.h>

#include <Graphics/model.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/models.h>
#include <Graphics/sky.h>
#include <Graphics/shaders.h>
#include <Graphics/camera.h>

#include <Math/vec2math.h>
#include <Math/vec3math.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Physics/bulletworld.h>
#include <Utility/assert.h>
#include <Graphics/lightprobecollection.hpp>
#include <Logging/logdata.h>

#include <algorithm>

#define USE_SSE

static const float _tile_size = 10.0f; // How big is each tile? (in meters)

extern char* global_shader_suffix;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_detail_objects_reduced;
extern bool g_detail_objects_reduced_dirty;
extern Timer game_timer;
extern bool g_debug_runtime_disable_detail_object_surface_draw;
extern bool g_debug_runtime_disable_detail_object_surface_pre_draw;
extern bool g_ubo_batch_multiplier_force_1x;

int num_detail_object_draw_calls;

void DetailObjectSurface::AttachTo( const Model& model, mat4 transform ) {
	PROFILER_ZONE(g_profiler_ctx, "DetailObjectSurface::AttachTo");
    base_model_ptr = &model;
    triangle_area.resize(model.faces.size()/3);
    vec3 verts[3];
    int face_index = 0;
    int vert_index;
    for(unsigned i=0; i<triangle_area.size(); ++i){
        for(unsigned j=0; j<3; ++j){
            vert_index = model.faces[face_index+j]*3;
            verts[j] = vec3(model.vertices[vert_index+0],
                            model.vertices[vert_index+1],
                            model.vertices[vert_index+2]);
            verts[j] = transform * verts[j];
        }
        triangle_area[i] = length(cross(verts[2]-verts[0], verts[1]-verts[0]))*0.5f;
        face_index += 3;
        static const bool debug_draw = false;
        if(debug_draw){
            vec3 pos = (verts[0] + verts[1] + verts[2])/3.0f;
            DebugDraw::Instance()->AddPoint(pos+vec3(0.0f,0.2f,0.0f), vec4(1.0f), _persistent, _DD_NO_FLAG);
        }
    }
}

//void DetailObjectSurface::GetTrisInSphere( const vec3 &center, float radius )
//{
//    BulletWorld bw;
//    bw.Init();
//    bw.InitWorld();
//    BulletObject* bo = bw.CreateStaticMesh(model_ptr);
//    TriListResults tlr;
//    TriListCallback cb(tlr);
//    bw.GetSphereCollisions(center, radius, cb);
//    const std::vector<int> &tris = cb.tri_list[bo];
//    const Model& model = *model_ptr;
//    int face_index;
//    int vert_index;
//    int tex_index;
//    vec3 verts[3];
//    vec2 tex_coords[3];
//    vec2 tex_coords2[3];
//    for(unsigned i=0; i<tris.size(); ++i){
//        face_index = tris[i]*3;
//        for(unsigned j=0; j<3; ++j){
//            vert_index = model.faces[face_index+j]*3;
//            tex_index = model.faces[face_index+j]*2;
//            verts[j] = vec3(model.vertices[vert_index+0],
//                            model.vertices[vert_index+1],
//                            model.vertices[vert_index+2]);
//            tex_coords[j] = vec2(model.tex_coords[tex_index+0],
//                                 model.tex_coords[tex_index+1]);
//            tex_coords2[j] = vec2(model.tex_coords2[tex_index+0],
//                                  model.tex_coords2[tex_index+1]);
//        }
//        unsigned num_dots = (unsigned)(triangle_area[tris[i]]*density);
//        for(unsigned j=0; j<num_dots; ++j){
//            vec2 coord(RangedRandomFloat(0.0f,1.0f),
//                       RangedRandomFloat(0.0f,1.0f));
//            if(coord[0] + coord[1] > 1){
//                std::swap(coord[0], coord[1]);
//                coord[0] = 1.0f - coord[0];
//                coord[1] = 1.0f - coord[1];
//            }
//            while(coord[0] + coord[1] > 1){
//                coord[0] = RangedRandomFloat(0.0f,1.0f);
//                coord[1] = RangedRandomFloat(0.0f,1.0f);
//            }
//            vec3 pos = (verts[0] + (verts[1]-verts[0])*coord[0] + (verts[2]-verts[0])*coord[1]);///3.0f;
//            if(distance_squared(pos, center)<radius*radius){
//                mat4 matrix;
//                matrix.SetRotationY(RangedRandomFloat(0.0f,6.28f));
//                matrix.SetTranslationPart(pos+vec3(0.0f,0.1f,0.0f));
//                vec2 tex_coord;
//                vec2 tex_coord2;
//                tex_coord = (tex_coords[0] + (tex_coords[1]-tex_coords[0])*coord[0] + (tex_coords[2]-tex_coords[0])*coord[1]);
//                tex_coord2 = (tex_coords2[0] + (tex_coords2[1]-tex_coords2[0])*coord[0] + (tex_coords2[2]-tex_coords2[0])*coord[1]);
//                DetailInstance di;
//                di.transform = matrix;
//                di.tex_coord[0] = tex_coord;
//                di.tex_coord[1] = tex_coord2;
//                TriInt ti = GetTriInt(pos);
//                DOPatch& current_patch = patches[ti];
//                current_patch.detail_instances.push_back(di);
//                #if defined(USE_SSE)
//                    vec4 centerVec4 = Mat4Vec4SimdMul(di.transform, vec4(model.bounding_sphere_origin));
//                    vec3 center = *(vec3*)&centerVec4;
//                #else
//                    vec3 center = di.transform * model.bounding_sphere_origin;
//                #endif
//                current_patch.detail_instance_origins_x.push_back(center.x());
//                current_patch.detail_instance_origins_y.push_back(center.y());
//                current_patch.detail_instance_origins_z.push_back(center.z());
//                //detail_instances.push_back(di);
//                //DebugDraw::Instance()->AddPoint(pos+vec3(0.0f,0.2f,0.0f), vec4(1.0f), _persistent, 0);
//            }
//        }
//    }
//    PatchesMap::iterator iter;
//    for(iter = patches.begin(); iter != patches.end(); ++iter){
//        DOPatch &patch = iter->second;
//        vec3 sphere_center;
//        for(unsigned j=0; j<patch.detail_instances.size(); ++j){
//            sphere_center += patch.detail_instances[j].transform.GetTranslationPart();
//        }
//        sphere_center /= (float)patch.detail_instances.size();
//        float least_distance = distance_squared(sphere_center, patch.detail_instances[0].transform.GetTranslationPart());
//        for(unsigned j=0; j<patch.detail_instances.size(); ++j){
//            least_distance = max(least_distance,
//                distance_squared(sphere_center, patch.detail_instances[j].transform.GetTranslationPart()));
//        }
//        patch.sphere_center = sphere_center;
//        patch.sphere_radius = sqrtf(least_distance);
//    }
//    /*PatchesMap::iterator iter;
//    for(iter = patches.begin(); iter != patches.end(); ++iter){
//        vec3 patch_color;
//        patch_color[0] = RangedRandomFloat(0.0f,1.0f);
//        patch_color[1] = RangedRandomFloat(0.0f,1.0f);
//        patch_color[2] = RangedRandomFloat(0.0f,1.0f);   
//
//        const std::vector<DetailInstance> &di_vec = iter->second;
//        for(unsigned j=0; j<di_vec.size(); ++j){
//            if(rand()%20 == 0){
//                DebugDraw::Instance()->AddPoint(di_vec[j].transform.GetTranslationPart()+vec3(0.0f,0.2f,0.0f), patch_color, _persistent, 0);
//            }
//        }
//    }*/
//
//    //Cluster();
//}


void DetailObjectSurface::GetTrisInPatches( mat4 transform ) {
	PROFILER_ZONE(g_profiler_ctx, "DetailObjectSurface::GetTrisInPatches");
    const Model& model = *base_model_ptr;
    int face_index;
	int vert_index;
	int tex_index;
	vec3 verts[3];
	vec3 normals[3];
	vec2 tex_coords[3];
	vec2 tex_coords2[3];
    TriInt patch_vert[3];
	TriInt patch_bounds[2];

	bounding_box[0] = INT_MAX;
	bounding_box[1] = INT_MIN;
	bounding_box[2] = INT_MAX;
	bounding_box[3] = INT_MIN;
	bounding_box[4] = INT_MAX;
	bounding_box[5] = INT_MIN;

    for(int i=0, len=base_model_ptr->faces.size()/3; i<len; ++i){
        face_index = i*3;
		for(unsigned j=0; j<3; ++j){
			vert_index = model.faces[face_index+j]*3;
			tex_index = model.faces[face_index+j]*2;
			verts[j] = vec3(model.vertices[vert_index+0],
				model.vertices[vert_index+1],
				model.vertices[vert_index+2]);
			verts[j] = transform * verts[j];
			normals[j] = vec3(model.normals[vert_index+0],
				model.normals[vert_index+1],
				model.normals[vert_index+2]);
			normals[j] = normalize(transform.GetRotatedvec3(normals[j]));
			tex_coords[j] = vec2(model.tex_coords[tex_index+0],
				model.tex_coords[tex_index+1]);
			tex_coords2[j] = vec2(model.tex_coords2[tex_index+0],
				model.tex_coords2[tex_index+1]);
            patch_vert[j] = GetTriInt(verts[j]);
        }
        patch_bounds[0] = patch_vert[0];
        patch_bounds[1] = patch_vert[0];
        for(unsigned k=0; k<3; ++k){
            for(unsigned j=1; j<3; ++j){
                patch_bounds[0].val[k] = min(patch_bounds[0].val[k], patch_vert[j].val[k]);
                patch_bounds[1].val[k] = max(patch_bounds[1].val[k], patch_vert[j].val[k]);    
            }
        }
        for(int x_patch=patch_bounds[0].val[0]; x_patch<=patch_bounds[1].val[0]; ++x_patch){
            for(int y_patch=patch_bounds[0].val[1]; y_patch<=patch_bounds[1].val[1]; ++y_patch){
                for(int z_patch=patch_bounds[0].val[2]; z_patch<=patch_bounds[1].val[2]; ++z_patch){
					bounding_box[0] = min(bounding_box[0], x_patch);
					bounding_box[1] = max(bounding_box[1], x_patch);
					bounding_box[2] = min(bounding_box[2], y_patch);
					bounding_box[3] = max(bounding_box[3], y_patch);
					bounding_box[4] = min(bounding_box[4], z_patch);
					bounding_box[5] = max(bounding_box[5], z_patch);
					TriInt ti(x_patch, y_patch, z_patch);
					DOPatch& patch = patches[ti];
					patch.tris.push_back(i);
					for(int k=0; k<3; ++k){
						patch.verts.push_back(verts[k]);
						patch.normals.push_back(normals[k]);
						patch.tex_coords.push_back(tex_coords[k]);
						patch.tex_coords2.push_back(tex_coords2[k]);
					}
                }
            }
        }
    }
}

void DetailObjectSurface::LoadDetailModel( const std::string &path ) {
    ofr = Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(path);
    double_sided = ofr->double_sided;
    if(Graphics::Instance()->config_.detail_object_lowres()) {
        std::string model_name = ofr->model_name.substr(0, ofr->model_name.rfind('.')) + "_low" + ofr->model_name.substr(ofr->model_name.rfind('.'));
        if(FindFilePath(model_name, kAnyPath, false).isValid())
            detail_model_id = Models::Instance()->loadModel(model_name);
        else
            detail_model_id = Models::Instance()->loadModel(ofr->model_name);
    }
    else
        detail_model_id = Models::Instance()->loadModel(ofr->model_name);
}

#ifdef USE_SSE
static vec4 Mat4Vec4SimdMul(const mat4& lhsMat, const vec4& rhsVec) {
    __m128 simdMat[4];
    simdMat[0] = _mm_load_ps(&lhsMat.entries[0]);
    simdMat[1] = _mm_load_ps(&lhsMat.entries[4]);
    simdMat[2] = _mm_load_ps(&lhsMat.entries[8]);
    simdMat[3] = _mm_load_ps(&lhsMat.entries[12]);

    __m128 simdVec;
    simdVec = _mm_load_ps(&rhsVec.entries[0]);

    __m128 v0 = _mm_shuffle_ps(simdVec, simdVec, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 v1 = _mm_shuffle_ps(simdVec, simdVec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 v2 = _mm_shuffle_ps(simdVec, simdVec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 v3 = _mm_shuffle_ps(simdVec, simdVec, _MM_SHUFFLE(3, 3, 3, 3));

    __m128 m0 = _mm_mul_ps(simdMat[0], v0);
    __m128 m1 = _mm_mul_ps(simdMat[1], v1);
    __m128 m2 = _mm_mul_ps(simdMat[2], v2);
    __m128 m3 = _mm_mul_ps(simdMat[3], v3);

    __m128 a0 = _mm_add_ps(m0, m1);
    __m128 a1 = _mm_add_ps(m2, m3);
    __m128 a2 = _mm_add_ps(a0, a1);

    vec4 result;
    _mm_store_ps(&result.entries[0], a2);

    return result;
}
#endif

void DetailObjectSurface::PreDrawCamera( const mat4 &transform ) {
    if (g_debug_runtime_disable_detail_object_surface_pre_draw) {
        return;
    }

    draw_detail_instances.clear();
    draw_detail_instance_transforms.clear();

    Camera* cam = ActiveCameras::Get();
	float temp_view_dist = view_dist * min(3.0f, 90.0f / cam->GetFOV());
    const int _tile_radius = (int)ceilf((temp_view_dist)/_tile_size); // How many tiles are visible in each direction
    vec3 cam_pos = cam->GetPos();

    TriInt cam_ti = GetTriInt(cam_pos);
    TriInt test_ti(0,0,0);

    const Model& model = Models::Instance()->GetModel(detail_model_id);
    float cull_dist_squared = square(temp_view_dist + model.bounding_sphere_radius);
	
	if(Graphics::Instance()->config_.detail_objects())
    {
        PROFILER_ZONE(g_profiler_ctx, "Calculate visible detail instances");

		int bounds[6];
		bounds[0] = max(bounding_box[0], cam_ti.val[0]-_tile_radius);
		bounds[1] = min(bounding_box[1], cam_ti.val[0]+_tile_radius);
		bounds[2] = max(bounding_box[2], cam_ti.val[1]-_tile_radius);
		bounds[3] = min(bounding_box[3], cam_ti.val[1]+_tile_radius);
		bounds[4] = max(bounding_box[4], cam_ti.val[2]-_tile_radius);
		bounds[5] = min(bounding_box[5], cam_ti.val[2]+_tile_radius);

        const vec4 modelBoundingSphereOrigin = vec4(model.bounding_sphere_origin);

        PatchesMap::iterator iter;
        for(int i=bounds[0]; i<=bounds[1]; ++i){
            for(int j=bounds[2]; j<=bounds[3]; ++j){
                for(int k=bounds[4]; k<=bounds[5]; ++k){
                    test_ti.val[0] = i;
                    test_ti.val[1] = j;
                    test_ti.val[2] = k;
                    iter = patches.find(test_ti);
                    if(iter != patches.end()){
                        if(!iter->second.calculated || g_detail_objects_reduced_dirty){
                            CalcPatchInstances(iter->first, iter->second, transform);
                        }
                        DOPatch& patch = iter->second;
                        if(!patch.detail_instances.empty()){
                            float square_patch_dist = distance_squared(patch.sphere_center, cam_pos);
                            if(square_patch_dist < square(temp_view_dist + patch.sphere_radius)) {
                                int in_frustum = cam->checkSphereInFrustum(patch.sphere_center, patch.sphere_radius);
                                if(in_frustum == 2 && square_patch_dist > square(temp_view_dist - patch.sphere_radius)){
                                    in_frustum = 1;
                                }
                                if(in_frustum == 1){
                                    static std::vector<uint32_t> is_visible;
                                    is_visible.clear();
                                    is_visible.resize(patch.detail_instances.size());
                                    cam->checkSpheresInFrustum(patch.detail_instances.size(), &patch.detail_instance_origins_x[0], &patch.detail_instance_origins_y[0], &patch.detail_instance_origins_z[0], model.bounding_sphere_radius, cull_dist_squared, &is_visible[0]);
                                    for(int l=0, len=patch.detail_instances.size(); l<len; ++l){
                                        if(is_visible[l]){
                                            draw_detail_instances.push_back(patch.detail_instances[l]);
                                        }
                                    }
                                    for(int l=0, len=patch.detail_instance_transforms.size(); l<len; ++l){
                                        if(is_visible[l]){
                                            draw_detail_instance_transforms.push_back(patch.detail_instance_transforms[l]);
                                        }
                                    }
                                } else if(in_frustum == 2){
                                    draw_detail_instances.insert( draw_detail_instances.end(), patch.detail_instances.begin(), patch.detail_instances.end() );
                                    draw_detail_instance_transforms.insert( draw_detail_instance_transforms.end(), patch.detail_instance_transforms.begin(), patch.detail_instance_transforms.end() );
                                }
                                //DebugDraw::Instance()->AddWireSphere(patch.sphere_center, patch.sphere_radius + 0.01f, vec4(1.0), _delete_on_draw);
                            }
                        }
                    }
                }
            }
        }

        g_detail_objects_reduced_dirty = false;
    }

    float cur_time = game_timer.game_time;
    CalculatedPatchesSet::iterator cpi;
    for(cpi = calculated_patches.begin(); cpi!=calculated_patches.end();){
        const TriInt& coords = (*cpi).coords;
        if((coords.val[0] < cam_ti.val[0]-_tile_radius ||
            coords.val[1] < cam_ti.val[1]-_tile_radius ||
            coords.val[2] < cam_ti.val[2]-_tile_radius ||
            coords.val[0] > cam_ti.val[0]+_tile_radius ||
            coords.val[1] > cam_ti.val[1]+_tile_radius ||
            coords.val[2] > cam_ti.val[2]+_tile_radius) &&
            ((*cpi).last_used_time < cur_time - 0.2f ||
            (*cpi).last_used_time > cur_time))
        {
            ClearPatch(patches[coords]);
            calculated_patches.erase(cpi++);
        } else {
            (*cpi).last_used_time = cur_time;
            ++cpi;
        }
    }
}

void DetailObjectSurface::Draw( const mat4 &transform, DetailObjectShaderType shader_type, vec3 color_tint, const TextureRef& light_cube, LightProbeCollection* light_probe_collection, SceneGraph* scenegraph_) {
    if (g_debug_runtime_disable_detail_object_surface_draw) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "Draw detail object surface");
    Shaders* shaders = Shaders::Instance();
	Graphics* graphics = Graphics::Instance();
	Camera* cam = ActiveCameras::Get();

    if(!bm.created){
        bm.Create(Models::Instance()->GetModel(detail_model_id));
    }
    batch.Dispose();

    batch.gl_state.depth_test = true;
    batch.gl_state.cull_face = !double_sided;
    batch.gl_state.depth_write = true;
    batch.gl_state.blend = false;

    batch.AddUniformFloat("fade",0.0f);
    batch.AddUniformFloat("height",bm.height);
    batch.AddUniformFloat("max_distance",view_dist * min(3.0f, 90.0f / cam->GetFOV()));
    batch.AddUniformFloat("overbright",overbright);
    batch.AddUniformVec3("color_tint", color_tint);
    batch.AddUniformFloat("haze_mult", scenegraph_->haze_mult);
    batch.AddUniformFloat("tint_weight", tint_weight);

    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    FormatString(shader_str[0], kShaderStrSize, "envobject #DETAIL_OBJECT");

    if(shader_type == TERRAIN) {
        FormatString(shader_str[1], kShaderStrSize, "%s #TERRAIN", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    if(ofr->shader_name == "plant" || ofr->shader_name == "envobject #TANGENT #ALPHA #PLANT"){
        FormatString(shader_str[1], kShaderStrSize, "%s #PLANT", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
	}
	if(ofr->shader_name == "plant_less_movement"){
		FormatString(shader_str[1], kShaderStrSize, "%s #PLANT #LESS_PLANT_MOVEMENT", shader_str[0]);
		std::swap(shader_str[0], shader_str[1]);
    }

    if(!Graphics::Instance()->config_.detail_object_decals()) {
        FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], "#NO_DECALS");
        std::swap(shader_str[0], shader_str[1]);
    }
    if(!Graphics::Instance()->config_.detail_object_shadows()) {
        FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], "#NO_DETAIL_OBJECT_SHADOWS");
        std::swap(shader_str[0], shader_str[1]);
    }

    FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], global_shader_suffix);
    std::swap(shader_str[0], shader_str[1]);

    batch.shader_id = shaders->returnProgram(shader_str[0]);
    batch.use_cam_pos = true;
    batch.use_light = true;

    bool transparent = shaders->IsProgramTransparent(batch.shader_id);
    if(transparent) {
        batch.use_time = true;        
    }

    if(ofr->clamp_texture){
        Textures::Instance()->setWrap(GL_CLAMP_TO_EDGE);
    } else {
        Textures::Instance()->setWrap(GL_REPEAT);
    }
    batch.texture_ref[0] = ofr->color_map_texture->GetTextureRef();
    batch.texture_ref[1] = ofr->normal_map_texture->GetTextureRef();
    batch.texture_ref[2] = light_cube;
    if(g_simple_shadows || !g_level_shadows){
        batch.texture_ref[4] = graphics->static_shadow_depth_ref;
    } else {
        batch.texture_ref[4] = graphics->cascade_shadow_depth_ref;
    }

    if(!ofr->translucency_map.empty()){
        batch.texture_ref[5] = ofr->translucency_map_texture->GetTextureRef();
    }
    batch.texture_ref[6] = base_color_ref->GetTextureRef();
    batch.texture_ref[7] = base_normal_ref->GetTextureRef();
    if(transparent){
        batch.transparent = true;
    }
    batch.AddUniformVec3("avg_color", vec3(ofr->avg_color_srgb[0]/255.0f,
        ofr->avg_color_srgb[1]/255.0f,
        ofr->avg_color_srgb[2]/255.0f));

    if(!graphics->config_.detail_objects()){
        return;
    }

    CHECK_GL_ERROR();
    batch.SetStartState();

    Textures::Instance()->bindTexture(graphics->screen_depth_tex, 18);

    if(light_probe_collection->light_probe_buffer_object_id != -1){
        shaders->SetUniformInt("num_tetrahedra", light_probe_collection->ShaderNumTetrahedra());
        shaders->SetUniformInt("num_light_probes", light_probe_collection->ShaderNumLightProbes());
        shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
        glBindBuffer(GL_TEXTURE_BUFFER, light_probe_collection->light_probe_buffer_object_id);
    }    

    scenegraph_->BindLights(batch.shader_id);

    CHECK_GL_ERROR();
    std::vector<mat4> shadow_matrix;
    shadow_matrix.resize(4);
    for(int i=0; i<4; ++i){
        shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
    }
    if(g_simple_shadows || !g_level_shadows){
        shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
    }
    shaders->SetUniformMat4("projection_view_mat", cam->GetProjMatrix() * cam->GetViewMatrix());
    shaders->SetUniformMat4Array("shadow_matrix",shadow_matrix);

    static const int kNumAttribs = 5;
    static const char* attrib_labels[] = {
        "vertex_attrib", "tex_coord_attrib", "tangent_attrib", "bitangent_attrib", "normal_attrib"
    };
    static const int attrib_offset[] = {
        0, 12, 6, 9, 3
    };
    static const int attrib_size[] = {
        3, 2, 3, 3, 3
    };

    CHECK_GL_ERROR();

    bm.vboc.Bind();
    bm.vboc_el.Bind();
    CHECK_GL_ERROR();
    int shader_id = shaders->bound_program;
    int attrib_ids[kNumAttribs];
    for(int i=0; i<kNumAttribs; ++i){
        CHECK_GL_ERROR();
        attrib_ids[i] = shaders->returnShaderAttrib(attrib_labels[i], shader_id);
        CHECK_GL_ERROR();
        if(attrib_ids[i] != -1){
            graphics->EnableVertexAttribArray(attrib_ids[i]);
            CHECK_GL_ERROR();
            glVertexAttribPointer(attrib_ids[i], attrib_size[i], GL_FLOAT, false, 
                sizeof(BatchVertex), (void*)(sizeof(GLfloat)*attrib_offset[i]));
            CHECK_GL_ERROR();
        }
    }
    CHECK_GL_ERROR();
    shaders->SetUniformMat4("model_mat", transform);
    CHECK_GL_ERROR();
    shaders->SetUniformMat3("normal_matrix", mat3(transform.GetInverseTranspose()));

    CHECK_GL_ERROR();

    int instance_block_index = shaders->GetUBOBindIndex(batch.shader_id, "InstanceInfo");
    if ((unsigned)instance_block_index != GL_INVALID_INDEX)
    {
        const GLchar *names[] = { 
            "transforms[0]",
            "texcoords2[0]",
            // These long names were necessary on a Mac OS 10.7 ATI card
            "InstanceInfo.transforms[0]",
            "InstanceInfo.texcoords2[0]",
        };

        GLuint indices[2];
        for(int i=0; i<2; ++i){
            indices[i] = shaders->returnShaderVariableIndex(names[i], shader_id);
            if(indices[i] == GL_INVALID_INDEX){
                indices[i] = shaders->returnShaderVariableIndex(names[i+2], shader_id);
            }
        }
        GLint offset[2];
        for(int i=0; i<2; ++i){
            if(indices[i] != GL_INVALID_INDEX){
                offset[i] = shaders->returnShaderVariableOffset(indices[i], shader_id);
            }
        }
        GLint block_size = shaders->returnShaderBlockSize(instance_block_index, shader_id);

        static GLubyte blockBuffer[131072];  // Big enough for 8x the OpenGL guaranteed minimum size. Max supported by shader flags currently. 16x or higher could be added if new platforms end up having > 128k frequently

        static UniformRingBuffer detail_object_instance_buffer;
        if(detail_object_instance_buffer.gl_id == -1){
            detail_object_instance_buffer.Create(2 * 1024 * 1024);
        }

        mat4* transforms = (mat4*)((uintptr_t)blockBuffer + offset[0]);
        vec4* texcoords2 = (vec4*)((uintptr_t)blockBuffer + offset[1]);

        static int ubo_batch_size_multiplier = 1;
        static GLint max_ubo_size = -1;
        if(max_ubo_size == -1) {
            glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_ubo_size);

            if(max_ubo_size >= 131072) {
                ubo_batch_size_multiplier = 8;
            }
            else if(max_ubo_size >= 65536) {
                ubo_batch_size_multiplier = 4;
            }
            else if(max_ubo_size >= 32768) {
                ubo_batch_size_multiplier = 2;
            }
        }

        int kBatchSize = 200 * (!g_ubo_batch_multiplier_force_1x ? ubo_batch_size_multiplier : 1);

        {
            PROFILER_ZONE(g_profiler_ctx, "Issue draw calls");
            for(int i=0, len=draw_detail_instances.size(); i<len; i+=kBatchSize){
                glUniformBlockBinding(shaders->programs[shader_id].gl_program, instance_block_index, 0);
                int to_draw = min(kBatchSize, len-i);
                if(indices[0] != GL_INVALID_INDEX){
                    memcpy(&transforms[0], &draw_detail_instance_transforms[i], to_draw * sizeof(mat4));
                }
                if(indices[1] != GL_INVALID_INDEX){
                    memcpy(&texcoords2[0], &draw_detail_instances[i], to_draw * sizeof(vec4));
                }
                {
                    // Copying over the whole block because fields aren't contiguous (struct of arrays), so at best can only save the final field's gap until the end of the buffer
                    detail_object_instance_buffer.Fill(block_size, blockBuffer);
                    glBindBufferRange(GL_UNIFORM_BUFFER, 0, detail_object_instance_buffer.gl_id, detail_object_instance_buffer.offset, detail_object_instance_buffer.next_offset - detail_object_instance_buffer.offset);
                }
                CHECK_GL_ERROR();
                graphics->DrawElementsInstanced(GL_TRIANGLES, bm.instance_num_faces, GL_UNSIGNED_INT, 0, to_draw);
                CHECK_GL_ERROR();
                ++num_detail_object_draw_calls;
            }
        }
    }

    graphics->ResetVertexAttribArrays();

    CHECK_GL_ERROR();

    batch.SetEndState();
    CHECK_GL_ERROR();
}

void DetailObjectSurface::SetBaseTextures( const TextureAssetRef& color_ref, 
                                           const TextureAssetRef& normal_ref ) 
{
    base_color_ref = color_ref;
    base_normal_ref = normal_ref;
}

TriInt DetailObjectSurface::GetTriInt( const vec3 &pos ) {
    TriInt ti((int)ceil(pos[0]/_tile_size), 
              (int)ceil(pos[1]/_tile_size), 
              (int)ceil(pos[2]/_tile_size));
    return ti;
}

void DetailObjectSurface::CalcPatchInstances( const TriInt &coords, DOPatch &patch, mat4 transform ) {
	PROFILER_ZONE(g_profiler_ctx, "CalcPatchInstances");
    patch.detail_instances.clear();
    patch.detail_instance_transforms.clear();
    patch.detail_instance_origins_x.clear();
    patch.detail_instance_origins_y.clear();
    patch.detail_instance_origins_z.clear();
    const Model& model = Models::Instance()->GetModel(detail_model_id);
    const vec4 modelBoundingSphereOrigin = vec4(model.bounding_sphere_origin);
    //int face_index;
    int index=0;
    vec3 verts[3];
    vec3 normals[3];
    vec2 tex_coords[3];
    vec2 tex_coords2[3];
	for(unsigned i=0; i<patch.tris.size(); ++i){
		double int_part;
		float float_part = (float)modf(triangle_area[patch.tris[i]]*density, &int_part);
		int num_dots = int(int_part) + (RangedRandomFloat(0.0f, 1.0f) < float_part);
		if(num_dots == 0){
			continue;
		}
        if(g_detail_objects_reduced && i % 2 == 0) {
            continue;
        }
        //face_index = patch.tris[i]*3;
		for(unsigned j=0; j<3; ++j){
            verts[j] = patch.verts[index];
			normals[j] = patch.normals[index];
			tex_coords[j] = patch.tex_coords[index];
			tex_coords2[j] = patch.tex_coords2[index];
			++index;
        }
		PROFILER_ZONE(g_profiler_ctx, "Fill dots");
        for(int j=0; j<num_dots; ++j){
            vec2 coord(RangedRandomFloat(0.0f,1.0f),
                RangedRandomFloat(0.0f,1.0f));
            if(coord[0] + coord[1] > 1){
                std::swap(coord[0], coord[1]);
                coord[0] = 1.0f - coord[0];
                coord[1] = 1.0f - coord[1];
            }
            while(coord[0] + coord[1] > 1){
                coord[0] = RangedRandomFloat(0.0f,1.0f);
                coord[1] = RangedRandomFloat(0.0f,1.0f);
			}
			vec2 tex_coord;
			tex_coord = (tex_coords[0] + (tex_coords[1]-tex_coords[0])*coord[0] + (tex_coords[2]-tex_coords[0])*coord[1]);
			vec4 color = weight_map->GetInterpolatedColorUV(tex_coord[0], tex_coord[1]);
			float grass_val = RangedRandomFloat(0.0f,1.0f);
			if(color[0] <= grass_val){
				continue;
			}
			vec3 pos = (verts[0] + (verts[1]-verts[0])*coord[0] + (verts[2]-verts[0])*coord[1]);///3.0f;
			vec3 normal = (normals[0] + (normals[1]-normals[0])*coord[0] + (normals[2]-normals[0])*coord[1]);
            //pos[1] -= (1.0f - normal[1]) * bm.height;
            quaternion rand_rot(true, 
                vec3(RangedRandomFloat(-jitter_degrees, jitter_degrees),
                     RangedRandomFloat(0.0f,360.0f),
					 RangedRandomFloat(-jitter_degrees, jitter_degrees)));
			mat4 matrix = Mat4FromQuaternion(rand_rot);
            if(normal_conform > 0.0f){
                mat4 normal_conform_mat;
                vec3 x_axis = normalize(cross(normal, vec3(0.0f,0.0f,1.0f)));
                vec3 z_axis = normalize(cross(x_axis, normal));
                normal_conform_mat.SetColumn(0, x_axis);
                normal_conform_mat.SetColumn(1, normal);
                normal_conform_mat.SetColumn(2, z_axis);
                quaternion normal_conform_quat = QuaternionFromMat4(normal_conform_mat);
                normal_conform_quat = normal_conform_quat * normal_conform;
                normal_conform_mat = Mat4FromQuaternion(normal_conform_quat);
                matrix = normal_conform_mat * matrix;
			}
            float scale_val = RangedRandomFloat(min_scale, max_scale);
            mat4 scale_matrix;
            scale_matrix.SetUniformScale(scale_val);
            matrix = scale_matrix * matrix;
            matrix.SetTranslationPart(pos);
            //vec2 tex_coord2 = (tex_coords2[0] + (tex_coords2[1]-tex_coords2[0])*coord[0] + (tex_coords2[2]-tex_coords2[0])*coord[1]);  // Not setting this because it's currently not used in drawing
			DetailInstance di;
            di.embed = RangedRandomFloat(min_embed,max_embed);
            di.tex_coord0 = tex_coord;
            di.height_scale = scale_val;
            TriInt ti = GetTriInt(pos);
            if(ti == coords){
                DOPatch& current_patch = patches[ti];
                current_patch.detail_instances.push_back(di);
                current_patch.detail_instance_transforms.push_back(matrix);
#if defined(USE_SSE)
                vec4 centerVec4 = Mat4Vec4SimdMul(matrix, modelBoundingSphereOrigin);
                vec3 center = *(vec3*)&centerVec4;
#else
                vec3 center = di.transform * model.bounding_sphere_origin;
#endif
                current_patch.detail_instance_origins_x.push_back(center.x());
                current_patch.detail_instance_origins_y.push_back(center.y());
                current_patch.detail_instance_origins_z.push_back(center.z());
            }
        }
    }
	if(!patch.detail_instance_transforms.empty()){
		PROFILER_ZONE(g_profiler_ctx, "Get bounding sphere");
        vec3 sphere_center;
        for(unsigned j=0; j<patch.detail_instance_transforms.size(); ++j){
            sphere_center += patch.detail_instance_transforms[j].GetTranslationPart();
        }
        sphere_center /= (float)patch.detail_instance_transforms.size();
        float least_distance = distance_squared(sphere_center, patch.detail_instance_transforms[0].GetTranslationPart());
        for(unsigned j=0; j<patch.detail_instance_transforms.size(); ++j){
            least_distance = max(least_distance,
                distance_squared(sphere_center, patch.detail_instance_transforms[j].GetTranslationPart()));
        }
        patch.sphere_center = sphere_center;
        patch.sphere_radius = sqrtf(least_distance);
    }
    patch.calculated = true;
    CalculatedPatch cp;
    cp.coords = coords;
    cp.last_used_time = game_timer.game_time;
    calculated_patches.insert(cp);
}

void DetailObjectSurface::ClearPatch( DOPatch &patch ) {
    LOGD << "Clearing patch..." << std::endl;
    patch.detail_instances.clear();
    patch.detail_instance_transforms.clear();
    patch.detail_instance_origins_x.clear();
    patch.detail_instance_origins_y.clear();
    patch.detail_instance_origins_z.clear();
    patch.calculated = false;
}

void DetailObjectSurface::SetDensity( float _density ) {
    density = _density;
}

void DetailObjectSurface::LoadWeightMap( const std::string &weight_path ) {
    //weight_map = ImageSamplers::Instance()->ReturnRef(weight_path);
    weight_map = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(weight_path);
}

void DetailObjectSurface::SetMinEmbed( float _min_embed ) {
    min_embed = _min_embed;
}

void DetailObjectSurface::SetMaxEmbed( float _max_embed ) {
    max_embed = _max_embed;
}

void DetailObjectSurface::SetMinScale( float _min_scale ) {
    min_scale = _min_scale;
}

void DetailObjectSurface::SetMaxScale( float _max_scale ) {
    max_scale = _max_scale;
}

void DetailObjectSurface::SetViewDist( float _view_dist ) {
    view_dist = _view_dist;
}

void DetailObjectSurface::SetCollisionType( CollisionType _collision ) {
    collision_type = _collision;
}

void DetailObjectSurface::SetNormalConform( float _normal_conform ) {
    normal_conform = _normal_conform;
}

void DetailObjectSurface::SetOverbright( float _overbright ) {
    overbright = _overbright;
}

void DetailObjectSurface::SetJitterDegrees( float _jitter_degrees ) {
    jitter_degrees = _jitter_degrees;
}

void DetailObjectSurface::SetColorTint(const vec3& tint) {
    batch.SetUniformVec3("color_tint", tint);
}

TriInt::TriInt( int a, int b, int c ) {
    val[0] = a;
    val[1] = b;
    val[2] = c;
}

bool TriInt::operator<( const TriInt &other ) const {
    return (val[0] < other.val[0] ||
            (val[0] == other.val[0] && 
             (val[1] < other.val[1] ||
              (val[1] == other.val[1] &&
               val[2] < other.val[2]))));
}

bool TriInt::operator==( const TriInt &other ) const {
    return val[0] == other.val[0] && 
           val[1] == other.val[1] && 
           val[2] == other.val[2];
}

DOPatch::DOPatch():
    calculated(false) 
{
}

void BatchModel::Create( const Model& detail_model ) {
    if(created){
        return;   
    }
    created = true;
    vec3 vec;
    int bvi = 0; // Batch vertex index
    int bti = 0; // Batch tex index
    int bfi = 0; // Batch face index
    int start_faces;
    std::vector<GLuint> indices;
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLuint> faces;
    std::vector<GLfloat> tangents;
    std::vector<GLfloat> bitangents;
    std::vector<GLfloat> tex_coords;
    unsigned num_vertices = 0;
    unsigned num_faces = 0;

    float vert_bounds[2];
    vert_bounds[0] = detail_model.vertices[1];
    vert_bounds[1] = detail_model.vertices[1];
    for(unsigned j=3; j<detail_model.vertices.size(); j+=3){
        vert_bounds[0] = min(vert_bounds[0], detail_model.vertices[j+1]);
        vert_bounds[1] = max(vert_bounds[1], detail_model.vertices[j+1]);
    }
    height = vert_bounds[1] - vert_bounds[0];

    for(unsigned i=0; i<1; ++i){
        start_faces = num_vertices;
        num_faces += detail_model.faces.size()/3;
        num_vertices += detail_model.vertices.size()/3;
        vertices.resize(vertices.size() + detail_model.vertices.size());
        normals.resize(normals.size() + detail_model.normals.size());
        tangents.resize(tangents.size() + detail_model.tangents.size());
        bitangents.resize(bitangents.size() + detail_model.bitangents.size());
        tex_coords.resize(tex_coords.size() + detail_model.tex_coords.size());
        faces.resize(faces.size() + detail_model.faces.size());
        indices.resize(indices.size() + detail_model.vertices.size()/3, i);
        for(unsigned j=0; j<detail_model.vertices.size(); j+=3){
            vertices[bvi+0] = detail_model.vertices[j];
            vertices[bvi+1] = detail_model.vertices[j+1]-vert_bounds[0];
            vertices[bvi+2] = detail_model.vertices[j+2];
            normals[bvi+0] = detail_model.normals[j];
            normals[bvi+1] = detail_model.normals[j+1];
            normals[bvi+2] = detail_model.normals[j+2];
            tangents[bvi+0] = detail_model.tangents[j];
            tangents[bvi+1] = detail_model.tangents[j+1];
            tangents[bvi+2] = detail_model.tangents[j+2];
            bitangents[bvi+0] = detail_model.bitangents[j];
            bitangents[bvi+1] = detail_model.bitangents[j+1];
            bitangents[bvi+2] = detail_model.bitangents[j+2];
            bvi += 3;
        }
        for(unsigned j=0; j<detail_model.tex_coords.size(); j+=2){
            tex_coords[bti+0] = detail_model.tex_coords[j];
            tex_coords[bti+1] = detail_model.tex_coords[j+1];
            bti += 2;
        }
        for(unsigned j=0; j<detail_model.faces.size(); j+=3){
            faces[bfi+0] = detail_model.faces[j]+start_faces;
            faces[bfi+1] = detail_model.faces[j+1]+start_faces;
            faces[bfi+2] = detail_model.faces[j+2]+start_faces;
            bfi += 3;
        }
    }

    std::vector<BatchVertex> bvs(num_vertices);
    unsigned vert_index = 0;
    unsigned tex_index = 0;
    for(unsigned i=0; i<num_vertices; ++i){
        BatchVertex &bv = bvs[i];
        bv.px = vertices[vert_index+0];
        bv.py = vertices[vert_index+1];
        bv.pz = vertices[vert_index+2];
        bv.nx = normals[vert_index+0];
        bv.ny = normals[vert_index+1];
        bv.nz = normals[vert_index+2];
        bv.tx = tangents[vert_index+0];
        bv.ty = tangents[vert_index+1];
        bv.tz = tangents[vert_index+2];
        bv.bx = bitangents[vert_index+0];
        bv.by = bitangents[vert_index+1];
        bv.bz = bitangents[vert_index+2];
        bv.tu = tex_coords[tex_index+0];
        bv.tv = tex_coords[tex_index+1];
        vert_index += 3;
        tex_index += 2;
    }
    vboc.Fill(kVBOFloat | kVBOStatic, sizeof(BatchVertex)*num_vertices, &bvs[0]);
    vboc_el.Fill(kVBOElement | kVBOStatic, sizeof(GLuint)*num_faces*3, &faces[0]);
    instance_num_faces = detail_model.faces.size();
}

void BatchModel::StopDraw() {
    Graphics *graphics = Graphics::Instance();
    graphics->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
    graphics->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
    GLuint index_attrib = Shaders::Instance()->returnShaderAttrib("index", Shaders::Instance()->bound_program);
    graphics->ResetVertexAttribArrays();

    graphics->SetClientActiveTexture(0);
    graphics->BindArrayVBO(0);
    graphics->BindElementVBO(0);
}

void BatchModel::Draw( const std::vector<DetailInstance> &di_vec, const std::vector<mat4> &di_vec_transforms ) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "BatchModel::Draw");
    if(di_vec.empty()){
        return;
    }

    unsigned pass_size;
    unsigned index = 0;
    std::vector<mat4> transforms(batch_size);
    std::vector<vec4> texcoords2(batch_size);
    unsigned num_passes = (di_vec.size()-1) / batch_size + 1;
    
    vboc_el.Bind();
    for(unsigned i=0; i<num_passes; ++i){
        pass_size = batch_size;
        int temp_index_mat = index;  // Need to work off a copy of index because loop was split up
        for(unsigned j=0; j<batch_size; ++j){
            if(temp_index_mat >= di_vec.size()){
                pass_size = j;
                break;
            }
            transforms[j] = di_vec_transforms[temp_index_mat];
            ++temp_index_mat;
        }
        for(unsigned j=0; j<batch_size; ++j){
            if(index >= di_vec.size()){
                pass_size = j;
                break;
            }
            texcoords2[j][0] = di_vec[index].tex_coord0[0];
            texcoords2[j][1] = di_vec[index].tex_coord0[1];
            texcoords2[j][2] = di_vec[index].embed;
            texcoords2[j][3] = di_vec[index].height_scale;
            ++index;
        }
        Shaders::Instance()->SetUniformMat4Array("transforms",transforms);
        Shaders::Instance()->SetUniformVec4Array("texcoords2",texcoords2);
        Graphics::Instance()->DrawElementsInstanced(GL_TRIANGLES, instance_num_faces, GL_UNSIGNED_INT, 0, pass_size);
        ++num_detail_object_draw_calls;
    }
}

BatchModel::BatchModel():
    created(false) 
{
}

bool CalculatedPatch::operator<( const CalculatedPatch &other ) const {
    return coords < other.coords;
}

bool CalculatedPatch::operator==( const CalculatedPatch &other ) const {
    return coords == other.coords;
}
