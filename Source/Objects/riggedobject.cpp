//-----------------------------------------------------------------------------
//           Name: riggedobject.cpp
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
#include "riggedobject.h"

#include <Graphics/graphics.h>
#include <Graphics/particles.h>
#include <Graphics/camera.h>
#include <Graphics/shaders.h>
#include <Graphics/textures.h>
#include <Graphics/models.h>
#include <Graphics/particles.h>
#include <Graphics/sky.h>
#include <Graphics/palette.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/simplify.hpp>

#include <Internal/timer.h>
#include <Internal/datemodified.h>
#include <Internal/collisiondetection.h>

#include <Objects/group.h>
#include <Objects/movementobject.h>
#include <Objects/itemobject.h>
#include <Objects/envobject.h>
#include <Objects/decalobject.h>
#include <Objects/lightvolume.h>

#include <XML/xml_helper.h>
#include <XML/level_loader.h>

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <Physics/bulletobject.h>
#include <Physics/bulletworld.h>
#include <Physics/physics.h>

#include <Asset/Asset/skeletonasset.h>
#include <Asset/Asset/fzx_file.h>
#include <Asset/Asset/material.h>
#include <Asset/Asset/attacks.h>

#include <GUI/gui.h>
#include <GUI/IMUI/imui.h>

#include <Internal/varstring.h>
#include <Internal/checksum.h>
#include <Internal/memwrite.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Main/engine.h>
#include <Main/scenegraph.h>

#include <Online/online.h>
#include <Online/online_datastructures.h>

#include <Compat/fileio.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Threading/thread_sanity.h>
#include <Sound/sound.h>
#include <Editors/map_editor.h>
#include <Memory/allocation.h>

#include <tinyxml.h>

#include <forward_list>
#include <cmath>
#include <sstream>
#include <cmath>

#ifdef PLATFORM_LINUX
    #include <malloc.h>
#endif
const bool _draw_char_collision_world = false;
bool GPU_skinning = false;

extern AnimationConfig animation_config;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern char* global_shader_suffix;
extern bool g_no_decals;
extern bool g_no_reflection_capture;
extern bool g_character_decals_enabled;

extern bool g_debug_runtime_disable_morph_target_pre_draw_camera;
extern bool g_debug_runtime_disable_rigged_object_draw;
extern bool g_debug_runtime_disable_rigged_object_pre_draw_camera;
extern bool g_debug_runtime_disable_rigged_object_pre_draw_frame;

const int kMaxBones = 200;

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

struct RiggedObjectGLState {
    GLState gl_state;
    RiggedObjectGLState() {
        gl_state.depth_test = true;
        gl_state.cull_face = true;
        gl_state.depth_write = true;
        gl_state.blend = false;
    }
};
static const RiggedObjectGLState rigged_object_gl_state;



float RiggedObject::GetRootBoneVelocity() {
	return velocity;
}

const std::vector<mat4>& RiggedObject::GetViewBones() const
{
	return display_bone_matrices;
}

RiggedObject::RiggedObject() :
	static_char(false),
	primary_weapon_id(-1),
	total_rotation(0.0f),
	character_script_getter(NULL),
	as_context(NULL),
	char_id(-1),
	lod_level(0),
	shadow_group_id(-1),
	char_scale(1.0f),
	transformed_vertex_getter_(this),
	blood_surface(&transformed_vertex_getter_),
	vel_vbo(V_MIBIBYTE, kVBOFloat | kVBODynamic),
	fur_transform_vec_vbo0(V_MIBIBYTE, kVBOFloat | kVBODynamic),
	fur_transform_vec_vbo1(V_MIBIBYTE, kVBOFloat | kVBODynamic),
	fur_transform_vec_vbo2(V_MIBIBYTE, kVBOFloat | kVBODynamic),
	fur_tex_transform_vbo(V_MIBIBYTE, kVBOFloat | kVBODynamic),
	last_draw_time(0.0f),
	shader("envobject #CHARACTER"),
	water_cube("character_accumulate #WATER_CUBE"),
	water_cube_expand("character_accumulate #EXPAND"),
	update_camera_pos(true)
{
    for( int i = 0; i < kLodLevels; i++ ) {
        transform_vec_vbo0[i] = new VBORingContainer(V_MIBIBYTE, kVBOFloat | kVBODynamic);
        transform_vec_vbo1[i] = new VBORingContainer(V_MIBIBYTE, kVBOFloat | kVBODynamic);
        transform_vec_vbo2[i] = new VBORingContainer(V_MIBIBYTE, kVBOFloat | kVBODynamic);
        tex_transform_vbo[i]  = new VBORingContainer(V_MIBIBYTE, kVBOFloat | kVBODynamic);
        need_vbo_update[i] = true;
    }                                                                

    palette_colors.resize(max_palette_elements, vec3(1.0f));
    palette_colors_srgb.resize(max_palette_elements, vec3(1.0f));
    for(int & i : model_id){
        i = -1;
    }

    anim_update_period = 2;
    max_time_until_next_anim_update = anim_update_period;

    time_until_next_anim_update = 0;
    curr_anim_update_time = -1;
    prev_anim_update_time = -1;

    collidable = false;
    
    animated = false;

    stab_texture = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/stabdecal.tga");
}

RiggedObject::~RiggedObject() {
    #ifdef USE_SSE
        #ifdef _WIN32
            _aligned_free(simd_bone_mats);
        #else
            free(simd_bone_mats);
        #endif
    #endif
    for(int i : model_id){
        Models::Instance()->DeleteModel(i);
    }
    for(auto & morph_target : morph_targets){
        morph_target.Dispose();
    }
    
    // TODO: why do we get a black screen when returning to the main menu on Mac
    // if the velocity vbo gets disposed?
    // Maybe something to do with motion blur being enabled or not?
    //vel_vbo.is_valid = false;

    for( int i = 0; i < kLodLevels; i++ ) {
        delete transform_vec_vbo0[i];
        delete transform_vec_vbo1[i];
        delete transform_vec_vbo2[i];
        delete tex_transform_vbo[i];
	}
	blood_surface.Dispose();
}

mat4 ASGetBindMatrix(Skeleton* skeleton, int which_bone){
    mat4 mat;
    const PhysicsBone& p_bone = skeleton->physics_bones[which_bone];
    mat.SetTranslationPart(p_bone.initial_position * -1.0f);
    mat = transpose(p_bone.initial_rotation)*mat;
    return mat;
}

int RiggedObject::GetTimeBetweenLastTwoAnimUpdates(){
    return max_time_until_next_anim_update;
    //return curr_anim_update_time - prev_anim_update_time;
}

int RiggedObject::GetTimeSinceLastAnimUpdate(){
    return -curr_anim_update_time;
}

extern Timer game_timer;

void RiggedObject::CalcRootBoneVelocity() {
	vec3 pos(0.0f);
	vec3 old_pos(0.0f); 
	pos = display_bone_matrices[0] * pos;
	old_pos = old_root_bone * old_pos;

	old_root_bone = display_bone_matrices[0];
	
	velocity = (length(pos - old_pos)) / (game_timer.GetWallTime() - old_time);
	old_time = game_timer.GetWallTime();
}

void RiggedObject::GetCubicSplineWeights(float interp, float *weights) {
    float interp_squared = interp*interp;
    float interp_cubed = interp_squared*interp;
    weights[0] = 0.5f * (-interp_cubed + 2.0f * interp_squared - interp);
    weights[1] = 0.5f * (3.0f * interp_cubed - 5.0f * interp_squared + 2.0f);
    weights[2] = 0.5f * (-3.0f * interp_cubed + 4.0f * interp_squared + interp);
    weights[3] = 0.5f * (interp_cubed - interp_squared);
}

const mat4 RiggedObject::InterpolateBetweenFourBonesQuadratic(const NetworkBone* bones, float inter_step) {
    float weights[4]; 

    GetCubicSplineWeights(inter_step, weights);

    quaternion quaternions[4];

    quaternions[0] = bones[0].rotation0;
    quaternions[1] = bones[1].rotation0;
    quaternions[2] = bones[2].rotation0;
    quaternions[3] = bones[3].rotation0;

    mat4 rotation0 = Mat4FromQuaternion(normalize(BlendFour(quaternions, weights)));

    quaternions[0] = bones[0].rotation1;
    quaternions[1] = bones[1].rotation1;
    quaternions[2] = bones[2].rotation1;
    quaternions[3] = bones[3].rotation1;

    mat4 rotation1 = Mat4FromQuaternion(normalize(BlendFour(quaternions, weights)));

    vec3 vectors[4];

    vectors[0] = bones[0].translation0;
    vectors[1] = bones[1].translation0;
    vectors[2] = bones[2].translation0;
    vectors[3] = bones[3].translation0;

	vec3 translation0 = BlendFour(vectors, weights); 

    vectors[0] = bones[0].translation1;
    vectors[1] = bones[1].translation1;
    vectors[2] = bones[2].translation1;
    vectors[3] = bones[3].translation1;

	vec3 translation1 = BlendFour(vectors, weights); 

    float floats[4];

    floats[0] = bones[0].scale;
    floats[1] = bones[1].scale;
    floats[2] = bones[2].scale;
    floats[3] = bones[3].scale;

	float scale = BlendFour(floats, weights);

    floats[0] = bones[0].model_char_scale;
    floats[1] = bones[1].model_char_scale;
    floats[2] = bones[2].model_char_scale;
    floats[3] = bones[3].model_char_scale;

	float model_char_scale = BlendFour(floats, weights);

	//'mat4 &mat = display_bone_matrices[index];
    mat4 mat;
	mat.LoadIdentity();
	mat.SetTranslationPart(translation0);
	mat = rotation0 * mat;
	mat4 scale_mat;
	scale_mat.SetScale(vec3(scale, scale, 1.0f) * model_char_scale);
	mat = scale_mat * mat;
	mat = rotation1 * mat;
	mat.AddTranslation(translation1);
    return mat;
}

const mat4 RiggedObject::InterpolateBetweenTwoBones(const NetworkBone& current, const NetworkBone& next, float interp_step) {

    mat4 rotation0 = Mat4FromQuaternion(normalize(Slerp(current.rotation0, next.rotation0, interp_step)));

    mat4 rotation1 = Mat4FromQuaternion(normalize(Slerp(current.rotation1, next.rotation1, interp_step)));

	vec3 translation0 = mix(current.translation0, next.translation0, interp_step);
	vec3 translation1 = mix(current.translation1, next.translation1, interp_step);

	float scale = (1.0f - interp_step) * current.scale
		+ interp_step * next.scale;

	float model_char_scale = (1.0f - interp_step) * current.model_char_scale
		+ interp_step * next.model_char_scale;

	//'mat4 &mat = display_bone_matrices[index];
    mat4 mat;
	mat.LoadIdentity();
	mat.SetTranslationPart(translation0);
	mat = rotation0 * mat;
	mat4 scale_mat;
	scale_mat.SetScale(vec3(scale, scale, 1.0f) * model_char_scale);
	mat = scale_mat * mat;
	mat = rotation1 * mat;
	mat.AddTranslation(translation1);
    return mat;
}

void RiggedObject::PreDrawFrame(float curr_game_time) {
    Online* online = Online::Instance();

    if (g_debug_runtime_disable_rigged_object_pre_draw_frame) {
        return;
    }

    
	CalcRootBoneVelocity(); 
	if (!online->IsClient()) {
		PROFILER_ZONE(g_profiler_ctx, "RiggedObject::PreDrawFrame");
		{
			PROFILER_ZONE(g_profiler_ctx, "needs_matrix_update");
			float interp_weight;
			if (animated) {
				interp_weight = game_timer.GetInterpWeightX(GetTimeBetweenLastTwoAnimUpdates(), GetTimeSinceLastAnimUpdate());
			}
			else {
				interp_weight = game_timer.GetInterpWeight();
			} 

			int num_bones = skeleton_.physics_bones.size();
			assert(num_bones < kMaxBones);
			for (int j = 0; j < num_bones; ++j) {
				const PhysicsBone& p_bone = skeleton_.physics_bones[j];
				BulletObject* b_o = p_bone.bullet_object;
				if (!b_o) {
					continue;
				}  

				mat4 &mat = display_bone_matrices[j];
				mat.LoadIdentity();
				mat.SetTranslationPart(p_bone.initial_position * -1.0f / model_char_scale);
				mat = transpose(p_bone.initial_rotation)*mat;
				mat = b_o->GetInterpWeightRotation(interp_weight) * mat;
				mat.AddTranslation(b_o->GetInterpWeightPosition(interp_weight));
			} 
			{
				PROFILER_ZONE(g_profiler_ctx, "Angelscript DisplayMatrixUpdate");
				as_context->CallScriptFunction(as_funcs.display_matrix_update);
			}
			
            //Apply scaling of character bones. Do this after the initial posing of the character to be able to retrieve
            //relative connection points of armor and other attachables.
			for (int j = 0; j < num_bones; ++j) {
				const PhysicsBone& p_bone = skeleton_.physics_bones[j];
				BulletObject* b_o = p_bone.bullet_object;
				if (!b_o) {
					continue;
				}
				mat4 &mat = display_bone_matrices[j];
                //Store attachment point
				display_bone_transforms[j] = mat;
                //Store scale for network bone.
				mat4 scale_mat;
				scale_mat.SetScale(vec3(skeleton_.physics_bones[j].display_scale, skeleton_.physics_bones[j].display_scale, 1.0f) * model_char_scale);
				
                //Shift back the bone to origo
				mat.AddTranslation(-b_o->GetInterpWeightPosition(interp_weight));
                //Rotate back to identity rotation.
				mat = invert(b_o->GetInterpWeightRotation(interp_weight)) * mat;
                //Apply scaling
				mat = scale_mat * mat;
                //Re-apply rotation
				mat = b_o->GetInterpWeightRotation(interp_weight) * mat;
                //Re-apply translation
				mat.AddTranslation(b_o->GetInterpWeightPosition(interp_weight));
			} 
		}
	}

	blood_surface.PreDrawFrame(
		Textures::Instance()->getWidth(texture_ref) / 4,
		Textures::Instance()->getHeight(texture_ref) / 4);

	{
		PROFILER_ZONE(g_profiler_ctx, "Update attached objects");
		int num_attached_env_objects = children.size();
		for (int i = 0; i < num_attached_env_objects; ++i) {
			AttachedEnvObject& attached_env_object = children[i];
			Object *obj = attached_env_object.direct_ptr;
			if (obj) {
				if (IsBeingMoved(scenegraph_->map_editor, obj) && obj->Selected()) {
				}
				else {
					int total_connections = 0;
					for (auto & bone_connect : attached_env_object.bone_connects) {
						total_connections += bone_connect.num_connections;
					}

					mat4 total_mat(0.0f);
					for (const auto & bone_connect : attached_env_object.bone_connects) {
							if (bone_connect.num_connections) {
							mat4 mat = display_bone_transforms[bone_connect.bone_id].GetMat4() * bone_connect.rel_mat;
							total_mat += mat * ((float)bone_connect.num_connections / (float)total_connections);
						}
					}

                    if (!online->IsClient()) {
                        obj->SetTranslation(total_mat.GetTranslationPart());
                        obj->SetRotation(QuaternionFromMat4(total_mat.GetRotationPart()));
                    }
				}
			}
		}
	}
}

void RiggedObject::StoreNetworkBones() {
    int num_bones = skeleton_.physics_bones.size();
    assert(num_bones < kMaxBones);
    network_bones.resize(num_bones);
    network_bones_host_walltime = game_timer.GetWallTime();
    for (int j = 0; j < num_bones; ++j) {
        const PhysicsBone& p_bone = skeleton_.physics_bones[j];
        BulletObject* b_o = p_bone.bullet_object;
        if (!b_o) {
            continue;
        }  

        NetworkBone network_bone; 
        network_bone.translation0 = p_bone.initial_position * -1.0f / model_char_scale; 
        network_bone.model_char_scale = model_char_scale;
        network_bone.rotation0 = QuaternionFromMat4(transpose(p_bone.initial_rotation));
        network_bone.rotation1 = QuaternionFromMat4(b_o->transform.GetRotationPart());
        network_bone.translation1 = b_o->transform.GetTranslationPart();//b_o->GetInterpWeightPosition(interp_weight);
        network_bone.scale = skeleton_.physics_bones[j].display_scale;  

        network_bones[j] = (network_bone); 
    } 
}

void RiggedObject::StoreNetworkMorphTargets() { 
	// first check if not present 
	if (morph_targets.size() != network_morphs.size()) {
		for (auto & morph_target : morph_targets) {
			bool present = false;

			for (auto & network_morph : network_morphs) {
				if (morph_target.name == network_morph.name) {
					present = true;
					break;
				}

			}

			if (present == false) {
				MorphTargetStateStorage missing_morph;
				missing_morph.disp_weight = morph_target.disp_weight;
				missing_morph.name = morph_target.name;
				missing_morph.dirty = true;
				missing_morph.mod_weight = morph_target.mod_weight;
				network_morphs.push_back(missing_morph);
			}
		}
	}

	for (auto & morph_target : morph_targets) {
		for (auto & network_morph : network_morphs) {
			if (morph_target.name == network_morph.name) {
				float a = morph_target.disp_weight;
				float b = network_morph.disp_weight;


				float c = network_morph.mod_weight;
				float d = morph_target.mod_weight;

				if (std::abs(a - b) > 1e-4 ||
					std::abs(c - d) > 1e-4) {

					network_morph.mod_weight = morph_target.mod_weight;
					network_morph.disp_weight = morph_target.disp_weight;
					network_morph.dirty = true;
				} 
			}

		}
	}

}

void RiggedObject::PreDrawCamera(float curr_game_time) {
    Online* online = Online::Instance();

    if (g_debug_runtime_disable_rigged_object_pre_draw_camera) {
        return;
    }

    PROFILER_ZONE(g_profiler_ctx, "RiggedObject::PreDraw");
    for(bool & i : need_vbo_update){
        i = true;
    }
    Graphics::Instance()->setGLState(rigged_object_gl_state.gl_state);

    shadow_group_id = -1;
    
    lod_level = 0;
    const vec3 &avg_pos = GetAvgPosition();
    const vec3 &cam_pos = ActiveCameras::Get()->GetPos();
    const float cam_zoom = 90.0f/ActiveCameras::Get()->GetFOV();
	const float _lod_switch_threshold = 20.0f * cam_zoom * cam_zoom * char_scale * char_scale;
	if (!online->IsActive()) {
		if (distance_squared(avg_pos, cam_pos) > _lod_switch_threshold * 16.0f) {
			lod_level = 3;
		}
		else if (distance_squared(avg_pos, cam_pos) > _lod_switch_threshold * 4.0f) {
			lod_level = 2;
		}
		else if (distance_squared(avg_pos, cam_pos) > _lod_switch_threshold) {
			lod_level = 1;
		}
	}

    // Use base LOD if lods are not loaded
    if(model_id[lod_level] == -1){
        lod_level = 0;
    }

    {
        PROFILER_ZONE(g_profiler_ctx, "ApplyBoneMatricesToModel");
        ApplyBoneMatricesToModel(false, lod_level);
    }
}

// Find the closest points between line segments ab and cd 
// Based on the theory described at: http://stackoverflow.com/questions/627563/calculating-the-shortest-distance-between-two-lines-line-segments-in-3d/702174#702174
void ClosestPointsBetweenTwoLineSegments(const vec3 &a, const vec3 &b, const vec3 &c, const vec3 &d, vec3 *ab_closest, vec3 *cd_closest){
    // Create two vectors perpendicular to AB
    float ab_len = distance(a,b);
    vec3 ab_dir = b-a;
    if(ab_len != 0.0f){
        ab_dir /= ab_len;
    }
    vec3 ab_perp[2];
    ab_perp[0] = vec3(-ab_dir[1], ab_dir[0], 0.0f); 
    ab_perp[1] = cross(ab_dir, ab_perp[0]);
    // Flatten start and end onto plane defined by AB
    vec3 rel_c = c - a;
    vec3 rel_d = d - a;
    vec2 flat_c(dot(ab_perp[0],rel_c), dot(ab_perp[1], rel_c));
    vec2 flat_d(dot(ab_perp[0],rel_d), dot(ab_perp[1], rel_d));
    // Find vector perpendicular to flat CD
    vec2 flat_cd_dir = normalize(flat_d - flat_c);
    vec2 perp_dir(-flat_cd_dir[1], flat_cd_dir[0]);
    // Move flat line so it intersects origin
    vec2 shift = perp_dir * dot(flat_c, perp_dir);
    flat_c -= shift;
    flat_d -= shift;
    // Find intersection point with origin in the space of flat CD
    float t = 0.0f;
    vec2 flat_dir = flat_d - flat_c;
    if(flat_dir[0] != 0.0f){
        t = -flat_c[0]/flat_dir[0];
    } else if(flat_dir[1] != 0.0f){
        t= -flat_c[1]/flat_dir[1];
    }
    // Clamp point to bounds of line segment CD
    t = min(1.0f, max(0.0f, t));
    // Find CD point in world space
    *cd_closest = c + (d - c) * t;
    // Find the point on AB nearest to the closest point on CD
    float t2 = 0.0f;
    if(ab_len != 0.0f){
        t2 = dot(ab_dir, *cd_closest-a)/ab_len;
    }
    // Clamp point to bounds of line segment AB
    t2 = min(1.0f, max(0.0f, t2));
    // Find AB point in world space
    *ab_closest = a + (b - a) * t2;
}

const int kOcclusionRadius = 2;

void RiggedObject::ClientBeforeDraw() {
    if(Online::Instance()->IsClient()) {
        /*
        std::list<RiggedObjectFrame>& bones = mp->bones[char_id];
        while(bones.size() > 1) {
            network_time_interpolator.timestamps.Clear();
            for(const RiggedObjectFrame& bone : bones) {
                network_time_interpolator.timestamps.PushValue(bone.host_walltime);
            }

            int interpolation_return = network_time_interpolator.Update();

            if(interpolation_return == 1) {
                continue;
            } else if(interpolation_return == 2) {
                delete[] bones.front().bones;
                bones.pop_front();
                continue;
            } else if(interpolation_return == 3) {
                break;
            }

            RiggedObjectFrame& current_bones = *bones.begin();
            RiggedObjectFrame& next_bones = *std::next(bones.begin());

            float prev_root_z = GetDisplayBonePosition(0).z();

            for(uint32_t i = 0; i < skeleton_.physics_bones.size(); i++) {
                InterpolateBetweenTwoBones(current_bones.bones[i], next_bones.bones[i], network_time_interpolator.interpolation_step, i);
            }

            break;
        }
        */

        if(network_display_bone_matrices.size() == display_bone_matrices.size()) {
            //Replace this with a better routine in the future, where we just use the network bone matrices on a client when rendering.
            display_bone_matrices = network_display_bone_matrices;
        }
    }
}

void RiggedObject::Draw(const mat4& proj_view_matrix, Object::DrawType type) {
    if (g_debug_runtime_disable_rigged_object_draw) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "RiggedObject::Draw()");
	if(type == Object::kFullDraw){
		last_draw_time = game_timer.game_time;
	}

    const bool _draw_animation_bones_xray = false;
    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();

    CHECK_GL_ERROR();
    if(_draw_char_collision_world && animated){
        //skeleton.DrawLocalBulletWorld();
        skeleton_.col_bullet_world->Draw(scenegraph_->sky->GetSpecularCubeMapTexture());
        return;
    }
    /*if(!animated){
    return;
    }*/

    if(model_id[0]==-1) {
        return;
    }

    graphics->setGLState(rigged_object_gl_state.gl_state);
    CHECK_GL_ERROR();

    // Get active shader variation (based on shadow catching and alpha to coverage)

    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    FormatString(shader_str[0], kShaderStrSize, "%s %s", shader, global_shader_suffix);
    if(GPU_skinning){
        FormatString(shader_str[1], kShaderStrSize, "%s #GPU_SKINNING", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }

    int the_shader = shaders->returnProgram(shader_str[0]);

    if(the_shader!=-1) {
        shaders->setProgram(the_shader);

        CHECK_GL_ERROR();

        shaders->SetUniformMat4("mvp",proj_view_matrix);
        shaders->SetUniformMat4("projection_view_mat", proj_view_matrix);
        if(GPU_skinning){
            shaders->SetUniformMat4Array("bone_mats", display_bone_matrices);
        }

        CHECK_GL_ERROR();
    }
    else {
        shaders->noProgram();
    }
    CHECK_GL_ERROR();

    if(model_id[0]!=-1){
        //int LOD = 0;
        Model* model = &Models::Instance()->GetModel(model_id[lod_level]);
        graphics->DebugTracePrint(model->path.c_str());
		textures->bindBlankTexture(7);
        if(type == RiggedObject::kFullDraw){
            PROFILER_GPU_ZONE(g_profiler_ctx, "Set up full draw state");
			if(blood_surface.blood_tex_mipmap_dirty){
				Textures::Instance()->GenerateMipmap(blood_surface.blood_tex);
				blood_surface.blood_tex_mipmap_dirty = false;
			}

            std::vector<mat4> shadow_matrix;
            shadow_matrix.resize(4);
            for(int i=0; i<4; ++i){
                shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
            }
            if(g_simple_shadows || !g_level_shadows){
                shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
            }
            shaders->SetUniformMat4Array("shadow_matrix",shadow_matrix);
            shaders->SetUniformVec3("cam_pos",ActiveCameras::Get()->GetPos());
            shaders->SetUniformVec3("ws_light",scenegraph_->primary_light.pos);
            shaders->SetUniformVec4("primary_light_color",vec4(scenegraph_->primary_light.color, scenegraph_->primary_light.intensity));
            shaders->SetUniformFloat("time",game_timer.GetRenderTime());
            shaders->SetUniformVec3("blood_tint", vec3(-1,-1,-1)); // Presumably this is here to force refreshing this uniform?
            shaders->SetUniformVec3("blood_tint", Graphics::Instance()->config_.blood_color());
            std::vector<vec3> ambient_cube_color_vec;
            ambient_cube_color_vec.reserve(ambient_cube_color.size());
            for (auto& i : ambient_cube_color) {
                ambient_cube_color_vec.push_back(ambient_cube_color[i]);
            }
            shaders->SetUniformVec3Array("ambient_cube_color", ambient_cube_color_vec);        
            shaders->SetUniformVec3Array("tint_palette", palette_colors_srgb);
            if(scenegraph_->light_probe_collection.probe_lighting_enabled && scenegraph_->light_probe_collection.light_probe_buffer_object_id != -1){
                shaders->SetUniformInt("num_tetrahedra", scenegraph_->light_probe_collection.ShaderNumTetrahedra());
                shaders->SetUniformInt("num_light_probes", scenegraph_->light_probe_collection.ShaderNumLightProbes());
                shaders->SetUniformVec3("grid_bounds_min", scenegraph_->light_probe_collection.grid_lookup.bounds[0]);
                shaders->SetUniformVec3("grid_bounds_max", scenegraph_->light_probe_collection.grid_lookup.bounds[1]);
                shaders->SetUniformInt("subdivisions_x", scenegraph_->light_probe_collection.grid_lookup.subdivisions[0]);
                shaders->SetUniformInt("subdivisions_y", scenegraph_->light_probe_collection.grid_lookup.subdivisions[1]);
                shaders->SetUniformInt("subdivisions_z", scenegraph_->light_probe_collection.grid_lookup.subdivisions[2]);
                shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
                shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
                glBindBuffer(GL_TEXTURE_BUFFER, scenegraph_->light_probe_collection.light_probe_buffer_object_id);
            }
            shaders->SetUniformInt("reflection_capture_num", scenegraph_->ref_cap_matrix.size());
            if(!scenegraph_->ref_cap_matrix.empty()){
                assert(!scenegraph_->ref_cap_matrix_inverse.empty());
                shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph_->ref_cap_matrix);
                shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph_->ref_cap_matrix_inverse);
            }
            shaders->SetUniformFloat("haze_mult", scenegraph_->haze_mult);

            std::vector<mat4> light_volume_matrix;
            std::vector<mat4> light_volume_matrix_inverse;
            for(auto obj : scenegraph_->light_volume_objects_){
                const mat4 &mat = obj->GetTransform();
                light_volume_matrix.push_back(mat);
                light_volume_matrix_inverse.push_back(invert(mat));
            }
            shaders->SetUniformInt("light_volume_num", light_volume_matrix.size());
            if(!light_volume_matrix.empty()){
                assert(!light_volume_matrix_inverse.empty());
                shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
                shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
            }

            if( g_no_reflection_capture == false) {
                textures->bindTexture(scenegraph_->cubemaps, 19);
            }
            if( g_character_decals_enabled ) {
                scenegraph_->BindDecals(the_shader);
            }
            scenegraph_->BindLights(the_shader);

            if(texture_ref.valid()){
                textures->bindTexture(texture_ref, 0);
            }
            if(normal_texture_ref.valid()){
                textures->bindTexture(normal_texture_ref, 1);
            }
            TextureAssetRef catcher_ref;
            TextureAssetRef caster_ref;
            if(!graphics->drawing_shadow){
                if(g_simple_shadows || !g_level_shadows){
                    textures->bindTexture(graphics->static_shadow_depth_ref, 4);
                } else {
                    textures->bindTexture(graphics->cascade_shadow_depth_ref, 4);
                }
                textures->bindBlankTexture(5);
            } else {
                textures->bindBlankTexture(4);
                textures->bindBlankTexture(5);
            }
            if(cube_map.valid()){
                textures->bindTexture(cube_map, 2);
            } else {
                textures->bindTexture(scenegraph_->sky->GetSpecularCubeMapTexture(), 2);
            }
            TextureRef blood_tex = blood_surface.blood_tex;
            if(blood_tex.valid()){
                textures->bindTexture(blood_tex, 6);
            }
            if(palette_texture_ref.valid()){
                textures->bindTexture(palette_texture_ref, 8);
            } else {
                textures->bindBlankTexture(8);
            }
            if(scenegraph_->light_probe_collection.light_volume_enabled && scenegraph_->light_probe_collection.ambient_3d_tex.valid()){
                textures->bindTexture(scenegraph_->light_probe_collection.ambient_3d_tex, 16);
            }
            textures->bindTexture(graphics->screen_color_tex, 17);
            textures->bindTexture(graphics->screen_depth_tex, 18);
        }

        if(need_vbo_update[lod_level]) {
            need_vbo_update[lod_level] = false;
            PROFILER_GPU_ZONE(g_profiler_ctx, "Fill dynamic vbos");
            int model_num_verts = model->vertices.size()/3;
            int fur_model_num_verts = fur_model.vertices.size()/3;
            if(GPU_skinning){
                PROFILER_ZONE(g_profiler_ctx, "GPU_skinning");
                if(!transform_vec_vbo0[lod_level]->valid()){
                    transform_vec_vbo0[lod_level]->SetHint(model_num_verts*4*sizeof(GLfloat),kVBOStatic);
                    transform_vec_vbo0[lod_level]->Fill(model_num_verts*4*sizeof(GLfloat), &model->bone_ids[0]);

                    transform_vec_vbo1[lod_level]->SetHint(model_num_verts*4*sizeof(GLfloat),kVBOStatic);
                    transform_vec_vbo1[lod_level]->Fill(model_num_verts*4*sizeof(GLfloat), &model->bone_weights[0]);
                }
                if(fur_model.faces.size() > 0 && !fur_transform_vec_vbo0.valid()){
                    fur_transform_vec_vbo0.SetHint(fur_model_num_verts*4*sizeof(GLfloat),kVBOStatic);
                    fur_transform_vec_vbo0.Fill(fur_model_num_verts*4*sizeof(GLfloat), &fur_model.bone_ids[0]);

                    fur_transform_vec_vbo1.SetHint(fur_model_num_verts*4*sizeof(GLfloat),kVBOStatic);
                    fur_transform_vec_vbo1.Fill(fur_model_num_verts*4*sizeof(GLfloat), &fur_model.bone_weights[0]);
                }
                transform_vec_vbo2[lod_level]->SetHint(model_num_verts*3*sizeof(GLfloat),kVBODynamic);
                transform_vec_vbo2[lod_level]->Fill(model_num_verts*3*sizeof(GLfloat), &model->morph_transform_vec[0]);
            } else {
                {
                    PROFILER_ZONE(g_profiler_ctx, "transform_vec_vbos");
                    transform_vec_vbo0[lod_level]->SetHint(model_num_verts*4*sizeof(GLfloat),kVBODynamic);
                    transform_vec_vbo0[lod_level]->Fill(model_num_verts*4*sizeof(GLfloat), &model->transform_vec[0][0]);

                    transform_vec_vbo1[lod_level]->SetHint(model_num_verts*4*sizeof(GLfloat),kVBODynamic);
                    transform_vec_vbo1[lod_level]->Fill(model_num_verts*4*sizeof(GLfloat), &model->transform_vec[1][0]);

                    transform_vec_vbo2[lod_level]->SetHint(model_num_verts*4*sizeof(GLfloat),kVBODynamic);
                    transform_vec_vbo2[lod_level]->Fill(model_num_verts*4*sizeof(GLfloat), &model->transform_vec[2][0]);
                }
            }

            static std::vector<float> vel;
            vel.resize(model_num_verts * 3);

            if(!animated){ // If ragdolled, propagate velocity to fixed bones as well
                for(int j=0, len=skeleton_.physics_bones.size(); j<len; j++){
                    if(skeleton_.fixed_obj[j] && skeleton_.physics_bones[j].bullet_object){
                        int bone = j;
                        while(skeleton_.parents[bone] != -1 && skeleton_.fixed_obj[bone]){
                            bone = skeleton_.parents[bone];
                        }
                        skeleton_.physics_bones[j].bullet_object->SetLinearVelocity(skeleton_.physics_bones[bone].bullet_object->GetLinearVelocity());
                    }
                }
            }

            if(graphics->config_.motion_blur_amount_ > 0.01f){
                PROFILER_ZONE(g_profiler_ctx, "Calculate vertex velocities");
                for(int j=0; j<model_num_verts; j++){
                    vec3 vert_vel = skeleton_.physics_bones[(unsigned int)model->bone_ids[j][0]].bullet_object->GetLinearVelocity() * model->bone_weights[j][0];
                    for(int k=1; k<4; k++){
                        if(model->bone_weights[j][k] > 0.0f) {
                            vert_vel += skeleton_.physics_bones[(unsigned int)model->bone_ids[j][k]].bullet_object->GetLinearVelocity() * model->bone_weights[j][k];
                        }
                    } 
                    vel[j*3+0] = vert_vel[0];
                    vel[j*3+1] = vert_vel[1];
                    vel[j*3+2] = vert_vel[2];
                }
                vel_vbo.SetHint(model_num_verts*3*sizeof(GLfloat),kVBODynamic);
                vel_vbo.Fill(model_num_verts*3*sizeof(GLfloat), &vel[0]);
            } else {
                PROFILER_ZONE(g_profiler_ctx, "Calculate null vertex velocities");
                for(float & i : vel){
                    i = 0.0f;
                }
                vel_vbo.SetHint(model_num_verts*3*sizeof(GLfloat),kVBODynamic);
                vel_vbo.Fill(model_num_verts*3*sizeof(GLfloat), &vel[0]);
            }

            if(lod_level == 0 || !tex_transform_vbo[lod_level]->valid()){
                PROFILER_ZONE(g_profiler_ctx, "tex_transform_vbo");
                if(lod_level == 0) {
                    tex_transform_vbo[lod_level]->SetHint(model_num_verts*2*sizeof(GLfloat),kVBODynamic);
                    tex_transform_vbo[lod_level]->Fill(model_num_verts*2*sizeof(GLfloat), &model->tex_transform_vec[0]);
                } else {
                    //Was static
                    tex_transform_vbo[lod_level]->SetHint(model_num_verts*2*sizeof(GLfloat),kVBOStatic);
                    tex_transform_vbo[lod_level]->Fill(model_num_verts*2*sizeof(GLfloat), &model->tex_transform_vec[0]);
                }
            }
            if(lod_level == 0 && fur_model.faces.size() != 0){
                PROFILER_ZONE(g_profiler_ctx, "fur_transform_vec_vbo");
                if(GPU_skinning){
                    fur_transform_vec_vbo2.SetHint(fur_model_num_verts*3*sizeof(GLfloat),kVBODynamic);
                    fur_transform_vec_vbo2.Fill(fur_model_num_verts*3*sizeof(GLfloat), &fur_model.morph_transform_vec[0]);
                } else {
                    fur_transform_vec_vbo0.SetHint(fur_model_num_verts*4*sizeof(GLfloat),kVBODynamic);
                    fur_transform_vec_vbo0.Fill(fur_model_num_verts*4*sizeof(GLfloat), &fur_model.transform_vec[0][0]);

                    fur_transform_vec_vbo1.SetHint(fur_model_num_verts*4*sizeof(GLfloat),kVBODynamic);
                    fur_transform_vec_vbo1.Fill(fur_model_num_verts*4*sizeof(GLfloat), &fur_model.transform_vec[1][0]);

                    fur_transform_vec_vbo2.SetHint(fur_model_num_verts*4*sizeof(GLfloat),kVBODynamic);
                    fur_transform_vec_vbo2.Fill(fur_model_num_verts*4*sizeof(GLfloat), &fur_model.transform_vec[2][0]);
                }
                if(!fur_tex_transform_vbo.valid()){
                    fur_tex_transform_vbo.SetHint(fur_model_num_verts*2*sizeof(GLfloat),kVBOStatic);
                    fur_tex_transform_vbo.Fill(fur_model_num_verts*2*sizeof(GLfloat), &fur_model.tex_transform_vec[0]);
                }
            }
        }

        DrawModel(model, lod_level);
        /*ApplyBoneMatricesToModel(true);
        transform_vec_vbo0.FillDynamic(model->num_vertices*4*sizeof(GLfloat), &model->transform_vec[0][0]);
        transform_vec_vbo1.FillDynamic(model->num_vertices*4*sizeof(GLfloat), &model->transform_vec[1][0]);
        transform_vec_vbo2.FillDynamic(model->num_vertices*4*sizeof(GLfloat), &model->transform_vec[2][0]);
        tex_transform_vbo.FillDynamic(model->num_vertices*2*sizeof(GLfloat), &model->tex_transform_vec[0]);
        DrawModel(model);*/
    }

    if(_draw_animation_bones_xray){
        float closest_dist;
        int closest_bone = -1;
        for(unsigned i=0; i<display_bone_matrices.size(); ++i){
            vec3 start = display_bone_matrices[i] * skeleton_.points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[0]];
            vec3 end = display_bone_matrices[i] * skeleton_.points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[1]];
            vec3 closest_point, closest_point_mouseray;
            ClosestPointsBetweenTwoLineSegments(
                ActiveCameras::Get()->GetPos(), 
                ActiveCameras::Get()->GetPos()+ActiveCameras::Get()->GetMouseRay()*200.0f,
                start,
                end,
                &closest_point_mouseray,
                &closest_point); 
            float dist = distance_squared(closest_point, closest_point_mouseray);
            if(closest_bone == -1 || dist < closest_dist){
                closest_dist = dist;
                closest_bone = i;
            }
        }
        const float kDistThreshold = 0.01f;
        for(unsigned i=0; i<display_bone_matrices.size(); ++i){
            vec3 start = display_bone_matrices[i] * skeleton_.points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[0]];
            vec3 end = display_bone_matrices[i] * skeleton_.points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[1]];
            vec4 color(1.0f);
            if(closest_bone == (int)i && closest_dist < kDistThreshold){
                color = vec4(1.0f,0.0f,0.0f,1.0f);
            }
            DebugDraw::Instance()->AddLine(start, end, color, _delete_on_draw, _DD_XRAY);
        }
    }
    CHECK_GL_ERROR();
}

bool RiggedObject::DrawBoneConnectUI(Object* objects[], int num_obj_ids, IMUIContext &imui_context, EditorTypes::Tool tool, int id) {
    Online* online = Online::Instance();
    bool did_something = false;
    float closest_dist;
    int closest_bone = -1;
    const float kDistThreshold = 0.01f;
    const std::vector<vec3> &points = skeleton_.skeleton_asset_ref->GetData().points;        
    for(unsigned i=0; i<display_bone_matrices.size(); ++i){
        if(!skeleton_.has_verts_assigned[skeleton_.physics_bones[i].bone]){
            continue;
        }
        vec3 start = display_bone_matrices[i] * points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[0]];
        vec3 end = display_bone_matrices[i] * points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[1]];
        vec3 closest_point, closest_point_mouseray;
        ClosestPointsBetweenTwoLineSegments(
            ActiveCameras::Get()->GetPos(), 
            ActiveCameras::Get()->GetPos()+ActiveCameras::Get()->GetMouseRay()*200.0f,
            start,
            end,
            &closest_point_mouseray,
            &closest_point); 
        float dist = distance_squared(closest_point, closest_point_mouseray);
        if(dist < kDistThreshold && (closest_bone == -1 || dist < closest_dist)){
            closest_dist = dist;
            closest_bone = i;
        }
    }

    for(unsigned i=0; i<display_bone_matrices.size(); ++i){
        if(!skeleton_.has_verts_assigned[skeleton_.physics_bones[i].bone]){
            continue;
        }
        IMUIContext::UIState ui_state;
        if(imui_context.DoButtonMouseOver(i + id * 100, closest_bone == (int)i,ui_state)){
            did_something = true;
            if(tool == EditorTypes::CONNECT){ 
                for(int obj_id_iter = 0; obj_id_iter < num_obj_ids; ++obj_id_iter){
                    Object* obj = objects[obj_id_iter];
                    // Check if we already have a connection between this object and this character
                    bool already_attached = false;
                    for(auto & attached_env_object : children){
                        if(attached_env_object.direct_ptr == obj){
                            // If this object is already attached to the selected bone, increment the number of connections
                            attached_env_object.bone_connection_dirty = true;
                            already_attached = true;
                            bool already_attached_to_bone = false;
                            for(auto & bone_connect : attached_env_object.bone_connects){
                                if(bone_connect.bone_id == (int)i){
                                    ++bone_connect.num_connections;
                                    already_attached_to_bone = true;
                                    break;
                                }
                            }
                            // If this object is not already attached to the bone, try to attach it with a spare slot
                            if(!already_attached_to_bone){
                                for(auto & bone_connect : attached_env_object.bone_connects){
                                    if(bone_connect.num_connections == 0){
                                        bone_connect.bone_id = i;
                                        bone_connect.num_connections = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    // If object is not attached to character, then we need a new attachment
                    if(!already_attached){
						// sync_attach_1 här sker själa ändringen, detta bör syncas
						// Vi vill översätta detta till obj id's och replikeras hos clienten
						//  set parent och skapa ett attachedenvobj
                        children.resize(children.size() + 1);
                        AttachedEnvObject &attached_env_object = children.back();
                        attached_env_object.bone_connection_dirty = true;
                        attached_env_object.direct_ptr = obj;
                        attached_env_object.bone_connects[0].bone_id = i;
                        attached_env_object.bone_connects[0].num_connections = 1;
						Object* obj = attached_env_object.direct_ptr;
						obj->SetParent(scenegraph_->GetObjectFromID(char_id));

						if (Online::Instance()->IsActive()) {
							online->Send<OnlineMessages::AttachToMessage>(char_id, obj->GetID(), i, true, false);
						}
                        for(unsigned j=1; j<kMaxBoneConnects; ++j){
                            BoneConnect &bone_connect = attached_env_object.bone_connects[j];
                            bone_connect.bone_id = -1;
                            bone_connect.num_connections = 0;
                        }
                    }
                }
            } else if(tool == EditorTypes::DISCONNECT){ 
                for(int obj_id_iter = 0; obj_id_iter < num_obj_ids; ++obj_id_iter){
                    Object* obj = objects[obj_id_iter];
                    for(int j=children.size()-1; j>=0; --j){
                        AttachedEnvObject& attached_obj = children[j];
                        if(attached_obj.direct_ptr == obj){
                            int total_connections = 0; //NOTE(David): Number of connections remaining between object and character
                            for(auto & bone_connect : attached_obj.bone_connects){
                                if(bone_connect.bone_id == (int)i && bone_connect.num_connections > 0){
                                    --bone_connect.num_connections;
                                }
                                total_connections += bone_connect.num_connections;
                            }
                            if(total_connections == 0){
                                Object* obj = attached_obj.direct_ptr;
                                obj->SetParent(NULL);
                                break;
                            }
                        }
                    }
                }
            }
        } 
        vec4 bone_color(1.0f);
        if(ui_state == IMUIContext::kActive){
            bone_color = vec4(1.0f,0.0f,0.0f,1.0f);
        }
        if(ui_state == IMUIContext::kHot){
            bone_color = vec4(1.0f,0.5f,0.5f,1.0f);
        }
        vec3 start = display_bone_matrices[i] * points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[0]];
        vec3 end = display_bone_matrices[i] * points[skeleton_.bones[skeleton_.physics_bones[i].bone].points[1]];
        DebugDraw::Instance()->AddLine(start, end, bone_color, _delete_on_draw);
        bone_color[3] *= 0.3f;
        DebugDraw::Instance()->AddLine(start, end, bone_color, _delete_on_draw, _DD_XRAY);
    }

    for(auto & i : children){
        const AttachedEnvObject& attached_env_object = i;
        Object* obj = i.direct_ptr;
        if(obj && obj->Selected()){
            for(const auto & bone_connect : attached_env_object.bone_connects){
                if(bone_connect.num_connections > 0){
                    int bone_id = bone_connect.bone_id;
                    const Bone& bone = skeleton_.bones[skeleton_.physics_bones[bone_id].bone];
                    vec3 start = obj->GetTranslation();
                    vec3 a = display_bone_matrices[bone_id] * skeleton_.points[bone.points[0]];
                    vec3 b = display_bone_matrices[bone_id] * skeleton_.points[bone.points[1]];
                    vec3 a_b = normalize(b-a);
                    vec3 end = display_bone_matrices[bone_id] * ((points[bone.points[0]]+skeleton_.points[bone.points[1]])*0.5f);
                    for(int k=0; k<bone_connect.num_connections; ++k){
                        float offset = ((float)k-(float)bone_connect.num_connections/2.0f)*0.03f;
                        DebugDraw::Instance()->AddLine(start+a_b*offset, end+a_b*offset, vec4(0.0f,1.0f,0.0f,0.8f), _delete_on_draw);
                        DebugDraw::Instance()->AddLine(start+a_b*offset, end+a_b*offset, vec4(0.0f,1.0f,0.0f,0.3f), _delete_on_draw, _DD_XRAY);
                    }
                }
            }
        }
    }
    return did_something;
}

bool RiggedObject::Initialize() {
    return true;
}

vec3 RiggedObject::GetTransformedVertex(int vert_id){
    Model *model = &Models::Instance()->GetModel(model_id[0]);
    LOG_ASSERT_LT(vert_id, (int)model->vertices.size()/3);
    vec3 vert = vec3(model->vertices[vert_id*3+0],
                     model->vertices[vert_id*3+1],
                     model->vertices[vert_id*3+2]);
    vec3 transformed;
    for(unsigned i=0; i<4; ++i){
        if(model->bone_weights[vert_id][i] > 0.0f){
            transformed += display_bone_matrices[(unsigned int)model->bone_ids[vert_id][i]] * vert *
                           model->bone_weights[vert_id][i];
        }
    }
    return transformed;
}

void RiggedObject::GetTransformedTri(int id, vec3* points){
    Model *model = &Models::Instance()->GetModel(model_id[0]);
    points[0] = GetTransformedVertex(model->faces[id*3+0]);
    points[1] = GetTransformedVertex(model->faces[id*3+1]);
    points[2] = GetTransformedVertex(model->faces[id*3+2]); 
}

int GetIKAttachBone( const Skeleton &skeleton, const std::string & ik_attach ) {
    Skeleton::SimpleIKBoneMap::const_iterator iter =
        skeleton.simple_ik_bones.find(ik_attach);
    int bone_id = -1;
    if(iter != skeleton.simple_ik_bones.end()){
        bone_id = iter->second.bone_id;
    } else {
        if(ik_attach == "hip"){
            iter = skeleton.simple_ik_bones.find("torso");
            bone_id = iter->second.bone_id;
            bone_id = skeleton.parents[bone_id];
            bone_id = skeleton.parents[bone_id];
        }
    }
    return bone_id;
}

int GetAttachmentBone( const Skeleton &skeleton, const std::string &label, int bone ) {
    Skeleton::SimpleIKBoneMap::const_iterator iter = 
        skeleton.simple_ik_bones.find(label);
    int bone_id = -1;
    if(iter != skeleton.simple_ik_bones.end()){
        bone_id = iter->second.bone_id;
    } 

    for(int i=0; i<bone; ++i){
        bone_id = skeleton.parents[bone_id];
    }
    return bone_id;
}

void UpdateAttachedItemRagdoll(AttachedItem &stuck_item, const Skeleton& skeleton, const std::map<int, mat4> &weapon_offset) {
    ItemObjectScriptReader& item = stuck_item.item;
    //AttachmentType type = _at_grip;
    if(item->state() == ItemObject::kSheathed){
        //type = _at_sheathe;
    }
    int bone_id = stuck_item.bone_id;
    mat4 bone_mat = skeleton.physics_bones[bone_id].bullet_object->GetTransform();
    mat4 the_weap_mat = bone_mat * weapon_offset.find(item->GetID())->second;
    mat4 temp_weap_mat = the_weap_mat;
    temp_weap_mat.SetColumn(2, the_weap_mat.GetColumn(1)*-1.0f);
    temp_weap_mat.SetColumn(1, the_weap_mat.GetColumn(2));
    item.SetPhysicsTransform(temp_weap_mat);
    if(item.just_created){
        item.SetPhysicsTransform(temp_weap_mat);
        item.just_created = false;
    }
    item.SetInterpInfo(1,1);
}

void RiggedObject::Update(float timestep) {
    PROFILER_ZONE(g_profiler_ctx, "RiggedObject Update()");

    Model* model = &Models::Instance()->GetModel(model_id[0]);
    //blood_surface.CreateDripInTri(RangedRandomInt(0, model->faces.size()/3-1), vec3(1.0f/3.0f), 1.0f, 0.0f, true, SurfaceWalker::WATER);
    {
        if(game_timer.game_time < blood_surface.sleep_time){
            PROFILER_ZONE(g_profiler_ctx, "Update blood surface");
            blood_surface.Update(scenegraph_, timestep);
        }
    }
    {
        PROFILER_ZONE(g_profiler_ctx, "Update anim client");
        anim_client.Update(timestep);
    }
    {
        PROFILER_ZONE(g_profiler_ctx, "HandleAnimationEvents");
        HandleAnimationEvents(anim_client);
    }
    if(!animated) {
        PROFILER_ZONE(g_profiler_ctx, "Update ragdoll");
        if(!first_ragdoll_frame) {
            skeleton_.UpdateTwistBones(true);
        }
        first_ragdoll_frame = false;
        // Updated attached items
        for(auto & item : attached_items.items) {
            UpdateAttachedItemRagdoll(item, skeleton_, weapon_offset);
        }
        // Make sure attached items are not asleep if skeleton is awake
        for(auto & item : stuck_items.items) {
            if(skeleton_.physics_bones[0].bullet_object->IsActive()){
                item.item->WakeUpPhysics();
            }
        }
    }

    --time_until_next_anim_update;
    --curr_anim_update_time;
    --prev_anim_update_time;

    if(time_until_next_anim_update <= 0) {
        for(auto & morph_target : morph_targets){
            morph_target.UpdateForInterpolation();
        }
        if(!animated){
            PROFILER_ZONE(g_profiler_ctx, "Update ragdoll animation");
            // Get results from animation blend tree
            AnimOutput anim_output;
            anim_client.GetMatrices(anim_output, &skeleton_.parents);
            if(!anim_output.matrices.empty()){
                // Store animation frame
                for(int i=0, len=animation_frame_bone_matrices.size(); i<len; i++){
                    anim_output.matrices[i].origin *= model_char_scale;
                    animation_frame_bone_matrices[i] = anim_output.matrices[i];
                }
                // Apply physics weights for active ragdoll
                std::map<BulletObject*, int> boid;
                for(unsigned i=0; i<skeleton_.physics_bones.size(); ++i){
                    boid[skeleton_.physics_bones[i].bullet_object] = i;
                }
                if(anim_output.physics_weights.size() >= animation_frame_bone_matrices.size()){
                    for(unsigned i=0; i<skeleton_.physics_joints.size(); ++i){
                        PhysicsJoint &joint = skeleton_.physics_joints[i];
                        int id[2];
                        id[0] = boid[joint.bt_bone[0]];
                        id[1] = boid[joint.bt_bone[1]];
                        float weight = max(anim_output.physics_weights[id[0]],
                                           anim_output.physics_weights[id[1]]);
                        skeleton_.SetGFStrength(joint, weight * ragdoll_strength);
                    }
                } else {
                    for(unsigned i=0; i<skeleton_.physics_joints.size(); ++i){
                        PhysicsJoint &joint = skeleton_.physics_joints[i];
                        skeleton_.SetGFStrength(joint, 1.0f * ragdoll_strength);
                    }
                }
                skeleton_.RefreshFixedJoints(animation_frame_bone_matrices);
                // Apply morph weights from animation output
                std::map<std::string, MorphTarget*> morph_target_names;
                for(auto & morph_target : morph_targets){
                    morph_target_names[morph_target.name] = &morph_target;
                }
                for(auto & skb : anim_output.shape_keys){
                    const std::string &label = skb.label;
                    if(morph_target_names.find(label) != morph_target_names.end()) {
                        morph_target_names[label]->anim_weight = 
                            mix(morph_target_names[label]->anim_weight, skb.weight*skb.weight_weight, ragdoll_strength);
                    }
                }
            }
        } else if(animated){
            PROFILER_ZONE(g_profiler_ctx, "Update animation");
			HandleLipSyncMorphTargets(timestep);
			AnimOutput anim_output;
            // Get results from animation blend tree
			bool drawn_recently = last_draw_time > game_timer.game_time - 1.0f;
			if(cached_animation_frame_bone_matrices.empty() || drawn_recently || !static_char){
            {
                PROFILER_ZONE(g_profiler_ctx, "anim_client.GetMatrices()");
                anim_client.GetMatrices(anim_output, &skeleton_.parents);
            }
            for(auto & matrice : anim_output.matrices){
                matrice.origin *= model_char_scale;
            }
            for(auto & ik_bone : anim_output.ik_bones){
                ik_bone.transform.origin *= model_char_scale;
            }
            weap_anim_info_map = anim_output.weap_anim_info_map;
            // Copy bone transforms for further manipulation
            std::vector<BoneTransform> transforms = anim_output.matrices;
            // Get info for moving and rotating entire character based on animation
            total_center_offset += vec3(anim_output.delta_offset[0],
                                        anim_output.delta_offset[1],
                                        anim_output.delta_offset[2]) * model_char_scale;
            total_rotation += anim_output.delta_rotation;

            unmodified_transforms = anim_output.unmodified_matrices;

            // Apply full-character rotation to bone and weapon transforms
            vec4 angle_axis(0.0f,1.0f,0.0f,total_rotation);
            quaternion rot(angle_axis);
            mat4 rotmat4 = Mat4FromQuaternion(rot);
            for(auto & transform : transforms){
                transform = rotmat4 * transform;
            }
            for(auto & ik_bone : anim_output.ik_bones){
                ik_bone.transform = rotmat4 * ik_bone.transform;
            }
            for(auto & iter : weap_anim_info_map) {
                WeapAnimInfo &weap_anim_info = iter.second;
                if(weap_anim_info.relative_id == -1){
                    weap_anim_info.bone_transform = rotmat4 * weap_anim_info.bone_transform;
                }
            }

            // Apply full-character translation to bone and weapon transforms
            for(unsigned i=0; i<transforms.size(); i++){
                transforms[i].origin += total_center_offset;
                animation_frame_bone_matrices[i] = transforms[i];
            }
            for(auto & ik_bone : anim_output.ik_bones){
                ik_bone.transform.origin += total_center_offset;
            }
            for(auto & iter : weap_anim_info_map) {
                WeapAnimInfo &weap_anim_info = iter.second;
                if(weap_anim_info.relative_id == -1){
                    weap_anim_info.bone_transform.origin += total_center_offset;
                } else {
                    weap_anim_info.matrix = weap_anim_info.bone_transform.GetMat4();
                }
            }
            
            // Apply animation morphs
            std::map<std::string, MorphTarget*> morph_target_names;
            for(auto & morph_target : morph_targets){
                morph_target_names[morph_target.name] = &morph_target;
            }
            for(auto & skb : anim_output.shape_keys){
                const std::string &label = skb.label;
                if(morph_target_names.find(label) != morph_target_names.end()) {
                    morph_target_names[label]->anim_weight = skb.weight*skb.weight_weight;
                }
            }

            // Apply status keys
            status_keys.clear();
            for(auto key : anim_output.status_keys){
                status_keys[key.label] = key.weight*key.weight_weight;
            }

            blended_bone_paths = anim_output.ik_bones;
			cached_animation_frame_bone_matrices = animation_frame_bone_matrices;
			} else {
				animation_frame_bone_matrices = cached_animation_frame_bone_matrices;
			}

            ASArglist args;
			if(drawn_recently){
				args.Add(anim_update_period);
			} else {
				args.Add(-anim_update_period);
			}
			{
                PROFILER_ZONE(g_profiler_ctx, "Angelscript FinalAnimationMatrixUpdate()");
                as_context->CallScriptFunction(as_funcs.final_animation_matrix_update, &args);
            }
            // Apply transform to physics objects
            for(unsigned i=0; i<animation_frame_bone_matrices.size(); i++){
                BulletObject* b_object = skeleton_.physics_bones[i].bullet_object;
                if(!b_object){
                    continue;
                }
                b_object->SetRotationAndVel(animation_frame_bone_matrices[i].rotation, -curr_anim_update_time);
                b_object->SetPositionAndVel(animation_frame_bone_matrices[i].origin, -curr_anim_update_time);
                b_object->SetRotation(animation_frame_bone_matrices[i].rotation);
                b_object->SetPosition(animation_frame_bone_matrices[i].origin);
				if(animation_config.kDisablePhysicsInterpolation){
					b_object->FixDiscontinuity();
					b_object->SetPosition(animation_frame_bone_matrices[i].origin);
				}
                b_object->UpdateTransform();
            }

            UpdateAttachedItems();

            {
                PROFILER_ZONE(g_profiler_ctx, "Angelscript FinalAttachedItemUpdate()");
                as_context->CallScriptFunction(as_funcs.final_attached_item_update, &args);
            }
        }
        max_time_until_next_anim_update = anim_update_period;
        time_until_next_anim_update = anim_update_period;
        prev_anim_update_time = curr_anim_update_time;
        curr_anim_update_time = 0;

        UpdateCollisionObjects();

        //Store the current animation frame bones for the next network character update.
        StoreNetworkBones(); 
    }

    if(animated){
        for(auto & iter : attached_items.items) {
            ItemObjectScriptReader& item = iter.item;
            item.SetInterpInfo(GetTimeSinceLastAnimUpdate(), GetTimeBetweenLastTwoAnimUpdates());
        }
        for(auto & item : stuck_items.items) {
            item.item.SetInterpInfo(GetTimeSinceLastAnimUpdate(), GetTimeBetweenLastTwoAnimUpdates());
        }
    }
}


namespace {
    struct BoneInfo {
        int id;
        float weight;
    };
    bool BoneInfoIDSort (const BoneInfo &a, const BoneInfo &b){
        return a.id < b.id;
    }
    bool BoneInfoWeightSort (const BoneInfo &a, const BoneInfo &b){
        return a.weight > b.weight;
    }
    void ApplyNewEdgeCollapseToModel(const Model &base_model, 
                                     Model &model, 
                                     const WOLFIRE_SIMPLIFY::SimplifyModel &sm, 
                                     const WOLFIRE_SIMPLIFY::ParentRecordListVec &vert_parent, 
                                     const WOLFIRE_SIMPLIFY::ParentRecordListVec &tex_parent) 
    {
        std::vector<float> sm_vertices(sm.vertices.size());
        std::vector<float> sm_tex_coords(sm.tex_coords.size());
        for(auto parents : vert_parent){
            float total[3];
            for(float & j : total){
                j = 0.0f;
            }
            for(auto & parent : parents){
                WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                if(pr.weight > 0.0f){
                    int parent_vert = sm.old_vert_id[pr.id];
                    int parent_vert_index = parent_vert*3;
                    for(int j=0; j<3; ++j){
                        total[j] += base_model.vertices[parent_vert_index+j] * pr.weight;
                    }
                }
            }
            for(auto & parent : parents){
                WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                int parent_vert_index = pr.id*3;
                for(int j=0; j<3; ++j){
                    sm_vertices[parent_vert_index+j] = total[j];
                }
            }
        }
        for(auto parents : tex_parent){
            float total[2];
            for(float & j : total){
                j = 0.0f;
            }
            for(auto & parent : parents){
                WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                int parent_tex_index = sm.old_tex_id[pr.id]*2;
                for(int j=0; j<2; ++j){
                    total[j] += base_model.tex_coords[parent_tex_index+j] * pr.weight;
                }
            }
            for(auto & parent : parents){
                WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                int parent_tex_index = pr.id*2;
                for(int j=0; j<2; ++j){
                    sm_tex_coords[parent_tex_index+j] = total[j];
                }
            }
        }
        int num_indices = sm.vert_indices.size();
        for(int i=0; i<num_indices; ++i){
            int vert_index = sm.vert_indices[i]*3;
            for(int j=0; j<3; ++j){
                model.vertices.push_back(sm_vertices[vert_index+j]);
            }
            int tex_index = sm.tex_indices[i]*2;
            for(int j=0; j<2; ++j){
                model.tex_coords.push_back(sm_tex_coords[tex_index+j]);
            }
        }
    }
} // namespace ""

void RiggedObject::Load(const std::string& character_path, vec3 pos, SceneGraph* scenegraph_ptr, OGPalette &palette) {
    //CharacterRef char_ref = Characters::Instance()->ReturnRef(character_path);
    CharacterRef char_ref = Engine::Instance()->GetAssetManager()->LoadSync<Character>(character_path);
    scenegraph_ = scenegraph_ptr;

    //ofc = ObjectFiles::Instance()->ReturnRef(char_ref->GetObjPath());
    ofc = Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(char_ref->GetObjPath());
    
    Models* mi = Models::Instance();
    int the_model_id = mi->loadModel(ofc->model_name, _MDL_CENTER);
    model_id[0] = the_model_id;
    
    Textures *ti = Textures::Instance();
    texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofc->color_map, PX_SRGB, 0x0);
    normal_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofc->normal_map);

    if(!ofc->palette_map_path.empty()) {
        palette_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofc->palette_map_path);
        for(int i=0; i<max_palette_elements; ++i){
            if(!ofc->palette_label[i].empty()){
                LabeledColor lc;
                lc.label = ofc->palette_label[i];
                lc.channel = i;
                lc.color = vec3(1.0f);
                palette.push_back(lc);
            }
        }
    }
            
    model_id[0] = Models::Instance()->CopyModel(the_model_id);
    Model *model = &Models::Instance()->GetModel(model_id[0]);
    floor_height = model->min_coords[1]; 
    
    UpdateGPUSkinning();
    SetTranslation(pos);

    skeleton_.SetBulletWorld(scenegraph_->bullet_world_);
    
    const std::string& skeleton_path = char_ref->GetSkeletonPath();

    FZXAssetRef fzx_ref;
    std::string fzx_path = ofc->model_name.substr(0, ofc->model_name.size()-3)+"fzx";
    if(FileExists(fzx_path.c_str(), kDataPaths|kModPaths)){
        //fzx_ref = FZXAssets::Instance()->ReturnRef(fzx_path);
        fzx_ref = Engine::Instance()->GetAssetManager()->LoadSync<FZXAsset>(fzx_path);
    }
    skeleton_.Read(skeleton_path, model_char_scale, char_scale, fzx_ref);
    
    int num_verts = model->vertices.size()/3;
    model->transform_vec.resize(4);
    model->transform_vec[0].resize(num_verts);
    model->transform_vec[1].resize(num_verts);
    model->transform_vec[2].resize(num_verts);
    model->transform_vec[3].resize(num_verts);
    model->tex_transform_vec.resize(num_verts);
    model->morph_transform_vec.resize(num_verts);

    //SkeletonAssetRef skeleton_asset_ref = SkeletonAssets::Instance()->ReturnRef(skeleton_path);
    SkeletonAssetRef skeleton_asset_ref = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(skeleton_path);
    const SkeletonFileData &data = skeleton_asset_ref->GetData();
    model->bone_weights = data.model_bone_weights;
    model->bone_ids = data.model_bone_ids;
    
    skeleton_.SetGravity(true);
    skeleton_.ReduceROM(0.5f);
    skeleton_.UnlinkFromBulletWorld();

    anim_client.SetBoneMasses(skeleton_.GetBoneMasses());
    anim_client.SetSkeleton(&skeleton_);
    anim_client.SetRetargeting(skeleton_path);

    const mat4 &transform = GetTransform();
    for(auto & physics_bone : skeleton_.physics_bones){
        if(!physics_bone.bullet_object){
            continue;
        }
        physics_bone.bullet_object->ApplyTransform(transform);
    }

    ik_offset.resize(skeleton_.physics_bones.size());
    total_ik_offset = 0.0f;
    ik_enabled = true;
    ragdoll_strength = 1.0f;

    lipsync_reader.SetVisemeMorphs(char_ref->GetVisemeMorphs());

    if(skeleton_.simple_ik_bones.find("head")!=skeleton_.simple_ik_bones.end()){
        const std::vector<int> &parents = skeleton_.parents;
        const SimpleIKBone &head_sib = skeleton_.simple_ik_bones["head"];
        int the_bone = head_sib.bone_id;
        for(int i=0; i<head_sib.chain_length; ++i){
            if(the_bone != -1){
                head_chain.insert(skeleton_.physics_bones[the_bone].bullet_object);
                the_bone = parents[the_bone];
            }
        }
    }

    const std::vector<MorphInfo> &morphs = char_ref->GetMorphs();
    for(const auto & morph : morphs){
        MorphTarget mt;
        mt.Load(ofc->model_name, the_model_id, morph.label, morph.num_steps);
        morph_targets.push_back(mt);
    }
    

    const std::string &morph_path = char_ref->GetPermanentMorphPath();
    if(!morph_path.empty() && FileExists(morph_path.c_str(), kDataPaths|kModPaths)){
        MorphTarget mt;
        mt.Load(ofc->model_name, the_model_id, morph_path, 1, true);
        mt.name = "permanent";
        mt.script_weight_weight = 1.0f;
        mt.script_weight = 1.0f;
        morph_targets.push_back(mt);
    }
    if(!LoadSimplificationCache(char_ref->path_)){   
        const Model &base_model = Models::Instance()->GetModel(model_id[0]);
        WOLFIRE_SIMPLIFY::SimplifyModelInput smi;
        // Prepare model to pass to simplify algorithm
        for(int i=0, len=base_model.faces.size(); i<len; ++i){
            int vert_index = base_model.faces[i]*3;
            for(int j=0; j<3; ++j){
                smi.vertices.push_back(base_model.vertices[vert_index+j]);
            }
            smi.old_vert_id.push_back(base_model.faces[i]);
            int tex_index = base_model.faces[i]*2;
            for(int j=0; j<2; ++j){
                smi.tex_coords.push_back(base_model.tex_coords[tex_index+j]);
            }
            smi.old_tex_id.push_back(base_model.faces[i]);
        }

        const int LOD_LEVELS = 5;
        WOLFIRE_SIMPLIFY::MorphModel lod[LOD_LEVELS];
        WOLFIRE_SIMPLIFY::ParentRecordListVec vert_parents[LOD_LEVELS];
        WOLFIRE_SIMPLIFY::ParentRecordListVec tex_parents[LOD_LEVELS];
        if(WOLFIRE_SIMPLIFY::SimplifyMorphLOD(smi, lod, vert_parents, tex_parents, LOD_LEVELS)){
            std::vector<BoneInfo> bone_info_accumulate;
            std::vector<vec4> bone_id;
            std::vector<vec4> bone_weight;

            for(int lod_index=1; lod_index<4; ++lod_index){
                model_id[lod_index] = Models::Instance()->AddModel();
                Model& model = Models::Instance()->GetModel(model_id[lod_index]);
                WOLFIRE_SIMPLIFY::SimplifyModel sm = lod[lod_index].model;
                WOLFIRE_SIMPLIFY::ParentRecordListVec vert_parent = vert_parents[lod_index-1];
                WOLFIRE_SIMPLIFY::ParentRecordListVec tex_parent = tex_parents[lod_index-1];
                bone_id.clear();
                bone_weight.clear();
                bone_id.resize(sm.vertices.size()/3, 0.0f);
                bone_weight.resize(sm.vertices.size()/3, 0.0f);
                for(auto parents : vert_parent){
                    float total[3];
                    bone_info_accumulate.clear();
                    for(float & j : total){
                        j = 0.0f;
                    }
                    for(auto & parent : parents){
                        WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                        if(pr.weight > 0.0f){
                            int parent_vert = sm.old_vert_id[pr.id];
                            int parent_vert_index = parent_vert*3;
                            for(int j=0; j<3; ++j){
                                total[j] += base_model.vertices[parent_vert_index+j] * pr.weight;
                            }
                            for(int j=0; j<4; ++j){
                                BoneInfo bi;
                                bi.id = (int)base_model.bone_ids[parent_vert][j];
                                bi.weight = base_model.bone_weights[parent_vert][j] * pr.weight;
                                if(bi.weight > 0.0f){
                                    bone_info_accumulate.push_back(bi);
                                }
                            }
                        }
                    }
                    // Merge duplicates
                    std::sort(bone_info_accumulate.begin(), bone_info_accumulate.end(), BoneInfoIDSort);
                    if(!bone_info_accumulate.empty()){
                        int last_unique = 0;
                        for(int i=1,len=bone_info_accumulate.size(); i<len; ++i){
                            BoneInfo &bi = bone_info_accumulate[i];
                            BoneInfo &unique_bi = bone_info_accumulate[last_unique];
                            if(bi.id == unique_bi.id){
                                unique_bi.weight += bi.weight;
                            } else {
                                ++last_unique;
                                bone_info_accumulate[last_unique] = bi;
                            }
                        }
                        bone_info_accumulate.resize(last_unique+1);
                    }
                    // Only use most-weighted 4 bones
                    std::sort(bone_info_accumulate.begin(), bone_info_accumulate.end(), BoneInfoWeightSort);
                    if(bone_info_accumulate.size() > 4){
                        bone_info_accumulate.resize(4);
                    }
                    // Normalize weights to add up to 1.0
                    float total_weight = 0.0f;
                    for(auto & bi : bone_info_accumulate){
                        total_weight += bi.weight; 
                    }
                    if(total_weight != 0.0f){
                        for(auto & bi : bone_info_accumulate){
                            bi.weight /= total_weight; 
                        }
                    }
                    for(auto & parent : parents){
                        WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                        int parent_vert_index = pr.id*3;
                        for(int j=0; j<3; ++j){
                            sm.vertices[parent_vert_index+j] = total[j];
                        }
                        for(int j=0,len=bone_info_accumulate.size(); j<len; ++j){
                            bone_id[pr.id][j] = (float)bone_info_accumulate[j].id;
                            bone_weight[pr.id][j] = bone_info_accumulate[j].weight;
                        }
                    }
                }
                for(auto parents : tex_parent){
                    float total[2];
                    for(float & j : total){
                        j = 0.0f;
                    }
                    for(auto & parent : parents){
                        WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                        int parent_tex_index = sm.old_tex_id[pr.id]*2;
                        for(int j=0; j<2; ++j){
                            total[j] += base_model.tex_coords[parent_tex_index+j] * pr.weight;
                        }
                    }
                    for(auto & parent : parents){
                        WOLFIRE_SIMPLIFY::ParentRecord pr = parent;
                        int parent_tex_index = pr.id*2;
                        for(int j=0; j<2; ++j){
                            sm.tex_coords[parent_tex_index+j] = total[j];
                        }
                    }
                }
                int num_indices = sm.vert_indices.size();
                for(int i=0; i<num_indices; ++i){
                    int vert_index = sm.vert_indices[i]*3;
                    for(int j=0; j<3; ++j){
                        model.vertices.push_back(sm.vertices[vert_index+j]);
                    }
                    model.bone_ids.push_back(bone_id[sm.vert_indices[i]]);
                    model.bone_weights.push_back(bone_weight[sm.vert_indices[i]]);
                    int tex_index = sm.tex_indices[i]*2;
                    for(int j=0; j<2; ++j){
                        model.tex_coords.push_back(sm.tex_coords[tex_index+j]);
                    }
                    model.faces.push_back(i);
                }

                model.RemoveDuplicatedVerts();
                model.RemoveDegenerateTriangles();
                model.OptimizeTriangleOrder();
                model.OptimizeVertexOrder();

                int model_num_verts = model.vertices.size()/3;
                model.transform_vec.resize(4);
                for(int j=0; j<4; ++j){   
                    model.transform_vec[j].resize(model_num_verts);
                }
                model.tex_transform_vec.resize(model_num_verts);
                model.morph_transform_vec.resize(model_num_verts);
                
                std::queue<MorphTarget*> queue;
                for(auto & morph_target : morph_targets){
                    queue.push(&morph_target);
                }
                while(!queue.empty()){
                    MorphTarget& morph = (*queue.front());
                    queue.pop();
                    for(auto & morph_target : morph.morph_targets){
                        queue.push(&morph_target);
                    }
                    if(morph.model_id[0] != -1) {
                        morph.model_id[lod_index] = Models::Instance()->AddModel();
                        morph.model_copy[lod_index] = true;
                        Model& morph_model = Models::Instance()->GetModel(morph.model_id[lod_index]);
                        Model& base_morph_model = Models::Instance()->GetModel(morph.model_id[0]);
                        ApplyNewEdgeCollapseToModel(base_morph_model, morph_model, sm, vert_parent, tex_parent);
                        morph_model.CopyVertCollapse(model);
                    }
                }
                for(auto & morph_target : morph_targets){
                    morph_target.CalcVertsModified(model_id[lod_index],lod_index);
                }
            }
            SaveSimplificationCache(char_ref->path_);
        } else {
            LOGI << "Simplification failed!" << std::endl;
        }
    }   

    Model *attach_model = &Models::Instance()->GetModel(model_id[0]);
    blood_surface.AttachToModel(attach_model);
    blood_surface.sleep_time = game_timer.game_time;

    if (!blood_inited && IsMainThread()) {
        blood_surface.PreDrawFrame(
            Textures::Instance()->getWidth(texture_ref) / 4,
            Textures::Instance()->getHeight(texture_ref) / 4);
        blood_surface.Update(scenegraph_, 0.001f, true);
        blood_inited = true;
    }

    const std::string &fur_path = char_ref->GetFurPath();
    int fur_model_id = -1;
    if(!fur_path.empty() && FileExists(fur_path.c_str(), kDataPaths|kModPaths)){
        fur_model_id = mi->loadModel(fur_path.c_str(), 0);
        CreateFurModel(model_id[0], fur_model_id);
    }

    Textures::Instance()->setWrap(GL_REPEAT, GL_CLAMP_TO_EDGE);
    fur_tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/fur_fin.tga");
    Textures::Instance()->setWrap(GL_REPEAT, GL_REPEAT);
    
    skeleton_.AlternateHull(ofc->model_name, Models::Instance()->GetModel(model_id[0]).old_center, model_char_scale);
    display_bone_matrices.resize(skeleton_.physics_bones.size());
    display_bone_transforms.resize(skeleton_.physics_bones.size());
    animation_frame_bone_matrices.resize(skeleton_.physics_bones.size());
    
    // Allocate aligned memory for SIMD matrix palette skinning
    #ifdef USE_SSE
	int bytes_needed = display_bone_matrices.size() * sizeof(simd_mat4);
        #ifdef _WIN32
            simd_bone_mats = (simd_mat4*)_aligned_malloc(bytes_needed, 16);
        #else
	    #ifdef __APPLE__
                simd_bone_mats = (simd_mat4*)malloc(bytes_needed);
            #else
                simd_bone_mats = (simd_mat4*)memalign(16, bytes_needed);
            #endif
        #endif
    #endif

    cached_skeleton_info_.bind_matrices.resize(skeleton_.physics_bones.size());
    for(int i=0, len=skeleton_.physics_bones.size(); i<len; ++i){
        cached_skeleton_info_.bind_matrices[i] = ASGetBindMatrix(&skeleton_, i);
    }
}

bool RiggedObject::InHeadChain(BulletObject* obj) {
    if(head_chain.find(obj) != head_chain.end()) {
        return true;
    }
    return false;
}

void RiggedObject::InvalidatedItem(ItemObjectScriptReader *invalidated) {
    int weap_id = -1;
    for(auto & item : attached_items.items) {
        if(&(item.item) == invalidated){
            weap_id = (item.item)->GetID();
            break;
        } 
    }
    attached_items.InvalidatedItem(invalidated);
    if(as_context){
        ASArglist args;
        args.Add(weap_id);
        as_context->CallScriptFunction(as_funcs.notify_item_detach, &args);
    }
    char_anim.clear();
}

mat4 LoadAttachPose(const std::string &anim_path, const std::string &ik_attach, const Skeleton &skeleton, int* bone_id_ptr, bool mirror, bool blade_orient, int chain_depth){
    int& bone_id = *bone_id_ptr;
    //AnimationRef anim_ref = Animations::Instance()->ReturnRef(anim_path);
    AnimationRef anim_ref = Engine::Instance()->GetAssetManager()->LoadSync<Animation>(anim_path);
    if( anim_ref.valid() ) {
        AnimOutput anim_output;
        BlendMap blend_map;
        AnimInput anim_input(blend_map, NULL);
        anim_ref->GetMatrices(0.0f, anim_output, anim_input);

        BoneTransform weapon_bt = anim_output.weapon_matrices[0];
        weapon_bt.origin += anim_output.center_offset;
        
        bone_id = GetIKAttachBone(skeleton, ik_attach);
        for(int i=0; i<chain_depth; ++i){
            bone_id = skeleton.parents[bone_id];
        }
        if(mirror){
            bone_id = skeleton.skeleton_asset_ref->GetData().symmetry[bone_id];
            anim_input.mirrored = mirror;
            anim_ref->GetMatrices(0.0f, anim_output, anim_input);
        }
        BoneTransform hand_bt = anim_output.matrices[bone_id];
        hand_bt.origin += anim_output.center_offset;
        if(mirror){
            weapon_bt.origin[0] *= -1.0f;
            {
                quaternion old_rotation = weapon_bt.rotation;
                vec4 aa = Quat_2_AA(old_rotation);
                aa[0] *= -1.0f;
                aa[3] *= -1.0f;
                weapon_bt.rotation = quaternion(aa);
                if(blade_orient){
                    quaternion new_rotation = weapon_bt.rotation;
                    vec3 tip_dir;
                    vec3 blade_dir;
                    if(ik_attach == "hip"){
                        tip_dir = normalize(vec3(0.0f,-0.5f,1.0f));
                        blade_dir = vec3(0.0f,-1.0f,0.0f);
                    } else {
                        tip_dir = vec3(0.0f,0.0f,-1.0f);
                        blade_dir = normalize(vec3(-0.25f, -0.165f, 0.0f));
                    }
                    vec3 new_blade_dir = new_rotation * invert(old_rotation) * blade_dir;
                    vec3 targ_blade_dir;
                    if(ik_attach == "hip"){
                        targ_blade_dir = vec3(0.0f,-1.0f,0.0f);
                    } else {
                        targ_blade_dir = vec3(0.25f, -0.165f, 0.0f);
                    }
                    if(dot(new_blade_dir, targ_blade_dir) < 0.0f){
                        //quaternion quat(vec4(0.0f,0.0f,1.0f,3.1415f));
                        //weapon_bt.rotation = quat * weapon_bt.rotation;
                        vec3 axis_candidate[3];
                        axis_candidate[0] = new_rotation * vec3(1.0f,0.0f,0.0f);
                        axis_candidate[1] = new_rotation * vec3(0.0f,1.0f,0.0f);
                        axis_candidate[2] = new_rotation * vec3(0.0f,0.0f,1.0f);
                        int which = 0;
                        if(fabs(axis_candidate[1][2]) > fabs(axis_candidate[0][2]) &&
                           fabs(axis_candidate[1][2]) > fabs(axis_candidate[2][2]))
                        {
                            which = 1;
                        } else if(fabs(axis_candidate[2][2]) > fabs(axis_candidate[0][2]))
                        {
                            which = 2;
                        }
                        quaternion quat(vec4(axis_candidate[which],3.1415f));
                        weapon_bt.rotation = quat * weapon_bt.rotation;
                    }
                }
            }
        }
        mat4 hand_mat = hand_bt.GetMat4();
        mat4 weapon_mat = weapon_bt.GetMat4();
        return invert(hand_mat) * weapon_mat;
    } else {
        return mat4(1.0f);
    }
}

void RiggedObject::SetWeaponOffset(AttachedItem &stuck_item, AttachmentType type, bool mirror){
    ItemObjectScriptReader& item = stuck_item.item;
    mat4& wo = weapon_offset[item->GetID()];
    mat4& wor = weapon_offset_retarget[item->GetID()];
    
    int bone_id;
    const ItemRef& ir = item.GetAttached()->item_ref();
    std::string ir_attachanimpath = ir->GetAttachAnimPath(type);
    if( ir_attachanimpath.empty() == false ) {
        wo = LoadAttachPose(ir_attachanimpath, ir->GetIKAttach(type), skeleton_, &bone_id, mirror, true, 0);
        if(!ir->GetAttachAnimBasePath(type).empty()){
            wor = LoadAttachPose(ir->GetAttachAnimBasePath(type), ir->GetIKAttach(type), skeleton_, &bone_id, mirror, true, 0);
            wor = invert(wor) * wo;
        } else {
            wor.LoadIdentity();
        }
    } else {
        LOGE << "No AttachAnimPath for type " << type << " in " << GetName() << std::endl;
    }
    
    stuck_item.rel_mat = wo;
    stuck_item.bone_id = bone_id;
}

void RiggedObject::SetItemAttachment(AttachedItem &stuck_item, AttachmentRef attachment_ref, bool mirror){
    ItemObjectScriptReader& item = stuck_item.item;
    mat4& wo = weapon_offset[item->GetID()];
    mat4& wor = weapon_offset_retarget[item->GetID()];

    int bone_id;
    wo = LoadAttachPose(attachment_ref->anim, attachment_ref->ik_chain_label, skeleton_, &bone_id, mirror, false, attachment_ref->ik_chain_bone);
    
    /*AnimationRef anim_ref = Animations::Instance()->ReturnRef(attachment_ref->anim);
    AnimOutput anim_output;
    BlendMap blend_map;
    AnimInput anim_input(blend_map, NULL);
    anim_ref->GetMatrices(0.0f, anim_output, anim_input);

    int bone_id = GetAttachmentBone(skeleton, attachment_ref->ik_chain_label, attachment_ref->ik_chain_bone);
    mat4 bone_mat = anim_output.matrices[bone_id].GetMat4();
    mat4 weapon_mat = anim_output.weapon_matrices[0].GetMat4();
    wo = invert(bone_mat) * weapon_mat;*/

    wor.LoadIdentity();
    stuck_item.rel_mat = wo;
    stuck_item.bone_id = bone_id;
}

void RiggedObject::SheatheItem(int id, bool on_right){
    AttachedItem *item_ptr = NULL;
    for(auto & iter : attached_items.items)
    {
        ItemObjectScriptReader& item = iter.item;
        if(item->GetID() == id){
            item_ptr = &iter;
        }
    }
    if(!item_ptr){
        return;
    }
    ItemObjectScriptReader &item = item_ptr->item;
    item->SetState(ItemObject::kSheathed);
    bool mirror = on_right;
    SetWeaponOffset(*item_ptr, _at_sheathe, mirror);
    char_anim.clear();
    character_script_getter->ItemsChanged(GetWieldedItemRefs());
}

void RiggedObject::UnSheatheItem(int id, bool right_hand){
    AttachedItem *item_ptr = NULL;
    for(auto & iter : attached_items.items)
    {
        ItemObjectScriptReader& item = iter.item;
        if(item->GetID() == id){
            item_ptr = &iter;
        }
    }
    if(!item_ptr){
        return;
    }
    ItemObjectScriptReader &item = item_ptr->item;
    item->SetState(ItemObject::kWielded);
    bool mirror = !right_hand;
    SetWeaponOffset(*item_ptr, _at_grip, mirror);
    char_anim.clear();
    character_script_getter->ItemsChanged(GetWieldedItemRefs());
}

void RiggedObject::DetachItem(ItemObject* item_object){
    int item_id = item_object->GetID();
    attached_items.Remove(item_id);
    char_anim.clear();
}

void RiggedObject::DetachAllItems(){
    attached_items.RemoveAll();
    char_anim.clear();

    for(auto & item : stuck_items.items)
    {
        item.item->ActivatePhysics();
    }
    stuck_items.items.clear();
}


void RiggedObject::InvalidatedItemCallback(ItemObjectScriptReader *invalidated, void* this_ptr) {
    ((RiggedObject*)this_ptr)->InvalidatedItem(invalidated);
}

vec3 CalcVertexOffset (const vec3 &world_pos, float wind_amount) {
    const float time = game_timer.game_time*2.0f;
    vec3 vertex_offset = vec3(0.0f);

    float wind_shake_amount = 0.02f;
    float wind_time_scale = 8.0f;
    float wind_shake_detail = 60.0f;
    float wind_shake_offset = (world_pos[0]+world_pos[1])*wind_shake_detail;
    /*wind_shake_amount *= max(0.0f,sinf((world_pos[0]+world_pos[1])+time*0.3f));
    wind_shake_amount *= sinf((world_pos[0]*0.1f+world_pos[2])*0.3f+time*0.6f)+1.0f;
    wind_shake_amount = max(0.002f,wind_shake_amount);
    wind_shake_amount *= wind_amount;*/

    wind_shake_amount = 0.05f*wind_amount;

    vertex_offset[0] += sinf(time*wind_time_scale+wind_shake_offset);
    vertex_offset[2] += cosf(time*wind_time_scale*1.2f+wind_shake_offset);
    vertex_offset[1] += cosf(time*wind_time_scale*1.4f+wind_shake_offset);

    vertex_offset *= wind_shake_amount;

    return vertex_offset;
}

bool RiggedObject::GetOnlineIncomingMorphTargetState(MorphTargetStateStorage& dest, const char* name) {
    if (incoming_network_morphs.find(name) == incoming_network_morphs.end()) {
        return false;
    }

    dest = incoming_network_morphs[name];
    return true;
}

void RiggedObject::ApplyBoneMatricesToModel(bool old, int lod_level) {
	Model* model = &Models::Instance()->GetModel(model_id[lod_level]);
	Online* online = Online::Instance();

    std::vector<MorphTarget*> active_morph_targets;
    const bool kMorphTargetsEnabled = true;
    if(kMorphTargetsEnabled){
        // Determine which morphs are active (have effective weight > 0)
        for(auto & m_t : morph_targets){
            if(online->IsClient() || m_t.weight > 0.0f ||
              (m_t.anim_weight > 0.0f && m_t.script_weight_weight < 1.0f) || 
              (m_t.script_weight > 0.0f && m_t.script_weight_weight > 0.0f))
            {
                m_t.PreDrawCamera(GetTimeBetweenLastTwoAnimUpdates(), GetTimeSinceLastAnimUpdate(), lod_level, char_id);
                if(online->IsClient() || m_t.weight > 0.0f ){
                    active_morph_targets.push_back(&m_t);
                }
            }
        }

        // Initialize per-vertex morph accumulators
        for(int j=0, len=model->vertices.size()/3; j<len; j++){
            model->tex_transform_vec[j] = 0.0f;
            model->morph_transform_vec[j] = 0.0f;
        }

        // Accumulate the effect of all active morphs
        Models *mi = Models::Instance();
        int vert_id, vert_id_index;
        for(auto morph : active_morph_targets){
            const Model *morph_model = &mi->GetModel(morph->model_id[lod_level]);
            const std::vector<int> &modified_tc = morph->modified_tc[lod_level];
            for(int j : modified_tc){
                vert_id = j;
                vert_id_index = vert_id*2;
                model->tex_transform_vec[vert_id] += 
                    vec2(morph_model->tex_coords[vert_id_index+0],
                         morph_model->tex_coords[vert_id_index+1]) *
                    morph->disp_weight;
            }
            const std::vector<int> &modified_vert = morph->modified_verts[lod_level];

            for(int j : modified_vert){
                vert_id = j;
                vert_id_index = vert_id*3;

				if (online->IsClient()) {
					MorphTargetStateStorage state;
					if (GetOnlineIncomingMorphTargetState(state, morph->name.c_str())) {
						morph->disp_weight = state.disp_weight;   
					}
				}
                model->morph_transform_vec[vert_id][0] += morph_model->vertices[vert_id_index+0] * morph->disp_weight;
                model->morph_transform_vec[vert_id][1] += morph_model->vertices[vert_id_index+1] * morph->disp_weight;
                model->morph_transform_vec[vert_id][2] += morph_model->vertices[vert_id_index+2] * morph->disp_weight;
            }
        } 

		if (online->IsHosting()) {
			StoreNetworkMorphTargets();
		}
    }
    if(lod_level == 0 && !fur_model.faces.empty()){
        for(int j=0, len=fur_model.vertices.size()/3; j<len; j++){
            fur_model.morph_transform_vec[j] = model->morph_transform_vec[fur_base_vertex_ids[j]];
        }
    }
    if(GPU_skinning){
        return;
    }

#ifndef USE_SSE
    #pragma omp parallel for
    for(int j=0; j<model->num_vertices; j++){
        mat4 transform = display_bone_matrices[(unsigned int)model->bone_ids[j][0]] * model->bone_weights[j][0];
        for(int k=1; k<4; k++){
            if(model->bone_weights[j][k] > 0.0f) {
                transform += display_bone_matrices[(unsigned int)model->bone_ids[j][k]] * model->bone_weights[j][k];
            }
        } 
        transform.SetTranslationPart(transform.GetTranslationPart() +
            transform.GetRotatedvec3(model->morph_transform_vec[j]));
        transform.ToVec4(&model->transform_vec[0][j],
                         &model->transform_vec[1][j],
                         &model->transform_vec[2][j],
                         &model->transform_vec[3][j]);
    }
#else
#pragma omp parallel for
    for(int j=0; j<(int)display_bone_matrices.size(); j++){
        simd_bone_mats[j].assign(display_bone_matrices[j].entries);
    }

#pragma omp parallel for
    for(int j=0,len=model->vertices.size()/3; j<len; j++){
        simd_mat4 transform = simd_bone_mats[(unsigned int)model->bone_ids[j][0]] * model->bone_weights[j][0];
        for(int k=1; k<4; k++){
            if(model->bone_weights[j][k] > 0.0f) {
                transform += simd_bone_mats[(unsigned int)model->bone_ids[j][k]] * model->bone_weights[j][k];
            }
        } 
        transform.AddRotatedVec3AndSave(model->morph_transform_vec[j].entries,
            model->transform_vec[0][j].entries,
            model->transform_vec[1][j].entries,
            model->transform_vec[2][j].entries,
            model->transform_vec[3][j].entries);
    }
        
    if(lod_level == 0 && !fur_model.faces.empty()){
    #pragma omp parallel for
        for(int j=0, len=fur_model.vertices.size()/3; j<len; j++){
            simd_mat4 transform = simd_bone_mats[(unsigned int)fur_model.bone_ids[j][0]] * fur_model.bone_weights[j][0];
            for(int k=1; k<4; k++){
                if(fur_model.bone_weights[j][k] > 0.0f) {
                    transform += simd_bone_mats[(unsigned int)fur_model.bone_ids[j][k]] * fur_model.bone_weights[j][k];
                }
            } 
            transform.AddRotatedVec3AndSave(fur_model.morph_transform_vec[j].entries,
                fur_model.transform_vec[0][j].entries,
                fur_model.transform_vec[1][j].entries,
                fur_model.transform_vec[2][j].entries,
                fur_model.transform_vec[3][j].entries);
        }
    }
#endif
}

void MorphTarget::SetScriptWeight(float weight, float weight_weight) {
    script_weight = weight;
    script_weight_weight = weight_weight;
}

void RiggedObject::SetMTTargetWeight( const std::string &target_name, float weight, float weight_weight ) {
    for(auto & morph_target : morph_targets){
        if(morph_target.name == target_name){
            morph_target.SetScriptWeight(weight, weight_weight);
            return;
        }
    }    
}

vec3 RiggedObject::FetchCenterOffset() {
    vec3 offset = total_center_offset;
    total_center_offset = vec3(0.0f);
    return offset;
}

float RiggedObject::FetchRotation() {
    float rot = total_rotation;
    total_rotation = 0.0f;
    return rot;
}

void RiggedObject::DrawModel( Model* model, int lod_level ) {
    PROFILER_ZONE(g_profiler_ctx, "RiggedObject::DrawModel()");
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();

    if(!model->vbo_loaded){
        PROFILER_ENTER(g_profiler_ctx, "Create VBO");
        model->createVBO();
        PROFILER_LEAVE(g_profiler_ctx);
    }

    PROFILER_ENTER(g_profiler_ctx, "Get shader attributes");
    CHECK_GL_ERROR();
    int shader = shaders->bound_program;
    int vertex_attrib_id = shaders->returnShaderAttrib("vertex_attrib", shader);
    int tex_coord_attrib_id = shaders->returnShaderAttrib("tex_coord_attrib", shader);
    int morph_tex_offset_attrib_id = shaders->returnShaderAttrib("morph_tex_offset_attrib", shader);
    int fur_tex_coord_attrib_id = shaders->returnShaderAttrib("fur_tex_coord_attrib", shader);
    int bone_ids_id = shaders->returnShaderAttrib("bone_ids", shader);
    int bone_weights_id = shaders->returnShaderAttrib("bone_weights", shader);
    int morph_offsets_id = shaders->returnShaderAttrib("morph_offsets", shader);
    int transform_mat_column_a_id = shaders->returnShaderAttrib("transform_mat_column_a", shader);
    int transform_mat_column_b_id = shaders->returnShaderAttrib("transform_mat_column_b", shader);
    int transform_mat_column_c_id = shaders->returnShaderAttrib("transform_mat_column_c", shader);
    int vel_id = shaders->returnShaderAttrib("vel_attrib", shader);
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Enable shader attributes");
    CHECK_GL_ERROR();
    if(vertex_attrib_id != -1){
        graphics->EnableVertexAttribArray(vertex_attrib_id);
        model->VBO_vertices.Bind();
        glVertexAttribPointer(vertex_attrib_id, 3, GL_FLOAT, false, 0, 0);
    }
    if(tex_coord_attrib_id != -1){
        graphics->EnableVertexAttribArray(tex_coord_attrib_id);
        model->VBO_tex_coords.Bind();
        glVertexAttribPointer(tex_coord_attrib_id, 2, GL_FLOAT, false, 0, 0);
    }

    if (GPU_skinning) {
        if(bone_ids_id != -1){
            graphics->EnableVertexAttribArray(bone_ids_id);
            transform_vec_vbo0[lod_level]->Bind();
            glVertexAttribPointer(bone_ids_id, 4, GL_FLOAT, false, 0, (const void*)transform_vec_vbo0[lod_level]->offset());
        }
        if(bone_weights_id != -1){
            graphics->EnableVertexAttribArray(bone_weights_id);
            transform_vec_vbo1[lod_level]->Bind();
            glVertexAttribPointer(bone_weights_id, 4, GL_FLOAT, false, 0, (const void*)transform_vec_vbo1[lod_level]->offset());
        }
        if(morph_offsets_id != -1){
            graphics->EnableVertexAttribArray(morph_offsets_id);
            transform_vec_vbo2[lod_level]->Bind();
            glVertexAttribPointer(morph_offsets_id, 3, GL_FLOAT, false, 0, (const void*)transform_vec_vbo2[lod_level]->offset());
        }
    } else {
        if(transform_mat_column_a_id != -1){
            graphics->EnableVertexAttribArray(transform_mat_column_a_id);
            transform_vec_vbo0[lod_level]->Bind();
            glVertexAttribPointer(transform_mat_column_a_id, 4, GL_FLOAT, false, 0, (const void*)transform_vec_vbo0[lod_level]->offset());
        }
        if(transform_mat_column_b_id != -1){
            graphics->EnableVertexAttribArray(transform_mat_column_b_id);
            transform_vec_vbo1[lod_level]->Bind();
            glVertexAttribPointer(transform_mat_column_b_id, 4, GL_FLOAT, false, 0, (const void*)transform_vec_vbo1[lod_level]->offset());
        }
        if(transform_mat_column_c_id != -1){
            graphics->EnableVertexAttribArray(transform_mat_column_c_id);
            transform_vec_vbo2[lod_level]->Bind();
            glVertexAttribPointer(transform_mat_column_c_id, 4, GL_FLOAT, false, 0, (const void*)transform_vec_vbo2[lod_level]->offset());
        }
    }
    if(morph_tex_offset_attrib_id != -1){
        graphics->EnableVertexAttribArray(morph_tex_offset_attrib_id);
        tex_transform_vbo[lod_level]->Bind();
        glVertexAttribPointer(morph_tex_offset_attrib_id, 2, GL_FLOAT, false, 0, (const void*)tex_transform_vbo[lod_level]->offset());
    }
	if(fur_tex_coord_attrib_id != -1){
		graphics->EnableVertexAttribArray(fur_tex_coord_attrib_id);
		model->VBO_tex_coords.Bind();
		glVertexAttribPointer(fur_tex_coord_attrib_id, 2, GL_FLOAT, false, 0, 0);
	}
    if(vel_id != -1){
        graphics->EnableVertexAttribArray(vel_id);
        vel_vbo.Bind();
        glVertexAttribPointer(vel_id, 3, GL_FLOAT, false, 0, (const void*)vel_vbo.offset());
    }
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Bind faces VBO");
    model->VBO_faces.Bind();
    PROFILER_LEAVE(g_profiler_ctx);
    GL_PERF_START();
    PROFILER_ENTER(g_profiler_ctx, "glDrawElements");
    graphics->DrawElements(GL_TRIANGLES, model->faces.size(), GL_UNSIGNED_INT, 0);
    PROFILER_LEAVE(g_profiler_ctx);
    GL_PERF_END();
    CHECK_GL_ERROR();

    if(lod_level == 0 && !fur_model.faces.empty() && !graphics->drawing_shadow){
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw fur");
        CHECK_GL_ERROR();
        Textures::Instance()->bindTexture(fur_tex, 7);
        graphics->setCullFace(false);
        graphics->SetBlendFunc(GL_ONE,GL_ZERO,GL_ONE,GL_ONE);
        graphics->setBlend(true);

        if(graphics->use_sample_alpha_to_coverage){
            glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
        }
        if(!fur_model.vbo_loaded){
            fur_model.createVBO();
        }

        if(vertex_attrib_id != -1){
            fur_model.VBO_vertices.Bind();
            glVertexAttribPointer(vertex_attrib_id, 3, GL_FLOAT, false, 0, 0);
        }
        if(tex_coord_attrib_id != -1){
            fur_model.VBO_tex_coords.Bind();
            glVertexAttribPointer(tex_coord_attrib_id, 2, GL_FLOAT, false, 0, 0);
        }
        if (GPU_skinning) {
            if(bone_ids_id != -1){
                graphics->EnableVertexAttribArray(bone_ids_id);
                fur_transform_vec_vbo0.Bind();
                glVertexAttribPointer(bone_ids_id, 4, GL_FLOAT, false, 0, (const void*)fur_transform_vec_vbo0.offset());
            }
            if(bone_weights_id != -1){
                graphics->EnableVertexAttribArray(bone_weights_id);
                fur_transform_vec_vbo1.Bind();
                glVertexAttribPointer(bone_weights_id, 4, GL_FLOAT, false, 0, (const void*)fur_transform_vec_vbo1.offset());
            }
            if(morph_offsets_id != -1){
                graphics->EnableVertexAttribArray(morph_offsets_id);
                fur_transform_vec_vbo2.Bind();
                glVertexAttribPointer(morph_offsets_id, 3, GL_FLOAT, false, 0, (const void*)fur_transform_vec_vbo2.offset());
            }
        } else {
            if(transform_mat_column_a_id != -1){
                fur_transform_vec_vbo0.Bind();
                glVertexAttribPointer(transform_mat_column_a_id, 4, GL_FLOAT, false, 0, (const void*)fur_transform_vec_vbo0.offset());
            }
            if(transform_mat_column_b_id != -1){
                fur_transform_vec_vbo1.Bind();
                glVertexAttribPointer(transform_mat_column_b_id, 4, GL_FLOAT, false, 0, (const void*)fur_transform_vec_vbo1.offset());
            }
            if(transform_mat_column_c_id != -1){
                fur_transform_vec_vbo2.Bind();
                glVertexAttribPointer(transform_mat_column_c_id, 4, GL_FLOAT, false, 0, (const void*)fur_transform_vec_vbo2.offset());
            }
        }
        if(morph_tex_offset_attrib_id != -1){
            fur_tex_transform_vbo.Bind();
            glVertexAttribPointer(morph_tex_offset_attrib_id, 2, GL_FLOAT, false, 0, (const void*)fur_tex_transform_vbo.offset());
        }
        if(fur_tex_coord_attrib_id != -1){
            graphics->EnableVertexAttribArray(fur_tex_coord_attrib_id);
            fur_model.VBO_tex_coords2.Bind();
            glVertexAttribPointer(fur_tex_coord_attrib_id, 2, GL_FLOAT, false, 0, 0);
        }
        fur_model.VBO_faces.Bind();
        graphics->DrawElements(GL_TRIANGLES, fur_model.faces.size(), GL_UNSIGNED_INT, 0);
        graphics->setCullFace(true);

        if(graphics->use_sample_alpha_to_coverage){
            glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
        }
    }

    PROFILER_ENTER(g_profiler_ctx, "Disable shader attributes");
    CHECK_GL_ERROR();
    graphics->ResetVertexAttribArrays();

    CHECK_GL_ERROR();
    PROFILER_LEAVE(g_profiler_ctx);
}

void RiggedObject::AddAnimation( std::string path, float weight_coord )
{
    //AnimationRef new_anim = Animations::Instance()->ReturnRef(path);
    //(animation_group.AddAnimation(new_anim, weight_coord);
}

void RiggedObject::FixDiscontinuity() {
	time_until_next_anim_update = 0;
	Update(game_timer.timestep);
	time_until_next_anim_update = 0;
	Update(game_timer.timestep);
    for(auto & physics_bone : skeleton_.physics_bones){
		if(physics_bone.bullet_object){
	        physics_bone.bullet_object->FixDiscontinuity();
		}
    }    
}

void RiggedObject::RefreshRagdoll()
{
    if(!animated){
        for(auto & physics_bone : skeleton_.physics_bones){
            BulletObject* b_object = physics_bone.bullet_object;
            if(!b_object){
                continue;
            }
            b_object->Activate();
        }    
    }
}


void RiggedObject::Ragdoll(const vec3 &velocity) {
    if(animated){
        animated = false;
        first_ragdoll_frame = true;
    
        for(auto & physics_bone : skeleton_.physics_bones){
            BulletObject* b_object = physics_bone.bullet_object;
            if(!b_object){
                continue;
            }
            b_object->Activate();
            b_object->old_transform.SetTranslationPart(b_object->GetInterpPositionX(GetTimeBetweenLastTwoAnimUpdates(), GetTimeSinceLastAnimUpdate()+1));
            b_object->old_transform.SetRotationPart(b_object->GetInterpRotationX(GetTimeBetweenLastTwoAnimUpdates(), GetTimeSinceLastAnimUpdate()+1));
        }    

        skeleton_.LinkToBulletWorld();
        for(unsigned i=0; i<skeleton_.physics_joints.size(); ++i){
            skeleton_.SetGFStrength(skeleton_.physics_joints[i], 0.0f);
        }

        for(auto & item : stuck_items.items)
        {
            item.item.ActivatePhysics();
            item.item.AddConstraint(skeleton_.physics_bones[item.bone_id].bullet_object);
        }
    }
}

void RiggedObject::SetDamping(float amount){
    for(auto & physics_bone : skeleton_.physics_bones){
        BulletObject* b_object = physics_bone.bullet_object;
        if(!b_object){
            continue;
        }
        b_object->SetDamping(amount);
        if(amount >= 1.0f){
            b_object->Sleep();
        }
    }   
    if(amount >= 1.0f){
        for(auto & item : stuck_items.items)
        {
            item.item->SleepPhysics();
        }
    }
}

void RiggedObject::UnRagdoll() {
    //mat4 inv_transform = invert(GetTransformationMatrix());
    if(!animated){
        for(auto & item : stuck_items.items)
        {
            item.item.RemoveConstraint();
        }

        anim_client.Reset();

        animated = true;

        skeleton_.UnlinkFromBulletWorld();
        skeleton_.UpdateTwistBones(true);

        time_until_next_anim_update = rand()%anim_update_period-anim_update_period;
    }
}

vec3 RiggedObject::GetAvgPosition()
{
    vec3 total(0.0f);
    int num_total = 0;
    for(auto & physics_bone : skeleton_.physics_bones){
        if(!physics_bone.bullet_object){
            continue;
        }
        num_total++;
        total += physics_bone.bullet_object->GetPosition();
    }    

    total /= (float)num_total;

    return total;
}

vec3 RiggedObject::GetAvgVelocity()
{
    vec3 total(0.0f);
    float divisor = 0.0f;
    for(auto & physics_bone : skeleton_.physics_bones){
        if(!physics_bone.bullet_object ||
           !physics_bone.bullet_object->linked){
            continue;
        }
        const float mass = physics_bone.bullet_object->GetMass();
        divisor += mass;
        total += physics_bone.bullet_object->GetLinearVelocity() * mass;
    }    

    total /= divisor;

    return total;
}

vec3 RiggedObject::GetAvgAngularVelocity() {
    vec3 total(0.0f);
    float divisor = 0.0f;
    for(auto & physics_bone : skeleton_.physics_bones){
        if(!physics_bone.bullet_object ||
           !physics_bone.bullet_object->linked){
                continue;
        }
        total += physics_bone.bullet_object->GetAngularVelocity() *
                 physics_bone.bullet_object->GetMass();
        divisor += physics_bone.bullet_object->GetMass();
    }    

    total /= divisor;

    return total;
}


quaternion RiggedObject::GetAvgRotation()
{
    quaternion total;
    for(auto & physics_bone : skeleton_.physics_bones){
        if(!physics_bone.bullet_object){
            continue;
        }
        const PhysicsBone& bone = physics_bone;
        mat4 rotation = bone.bullet_object->GetRotation() * 
                        invert(bone.initial_rotation);
        total += QuaternionFromMat4(rotation) * bone.bullet_object->GetMass();
    }    

    total = normalize(total);

    return total;
}

vec3 RiggedObject::GetIKTargetPosition( const std::string &target_name )
{
    if(animation_frame_bone_matrices.empty()){
        return vec3(0.0f);
    }
    Skeleton::SimpleIKBoneMap::iterator iter;
    iter = skeleton_.simple_ik_bones.find(target_name);
    if(iter != skeleton_.simple_ik_bones.end()){
        return animation_frame_bone_matrices[iter->second.bone_id].origin;
    }
    return vec3(0.0f);
}

float RiggedObject::GetIKWeight( const std::string &target_name ) {
    for(auto & blended_bone_path : blended_bone_paths){
        if(blended_bone_path.ik_bone.label == target_name){
            return blended_bone_path.weight;
        }
    }
    return 0.0f;
}

BoneTransform GetIKTransform( RiggedObject* rigged_object, const std::string &target_name ) {
    for(auto & blended_bone_path : rigged_object->blended_bone_paths){
        if(blended_bone_path.ik_bone.label == target_name){
            return blended_bone_path.transform;
        }
    }
    BoneTransform identity;
    return identity;
}

mat4 GetUnmodifiedIKTransform( RiggedObject* rigged_object, const std::string &target_name ) {
    for(auto & blended_bone_path : rigged_object->blended_bone_paths){
        if(blended_bone_path.ik_bone.label == target_name){
            return blended_bone_path.unmodified_transform.GetMat4();
        }
    }
    mat4 identity;
    return identity;
}

vec3 RiggedObject::GetIKTargetAnimPosition( const std::string &target_name ) {
    for(auto & blended_bone_path : blended_bone_paths){
        if(blended_bone_path.ik_bone.label == target_name){
            int target_bone = blended_bone_path.ik_bone.bone_path.back();
            return unmodified_transforms[target_bone].origin;
        }
    }
    return vec3(0.0f);
}

void RiggedObject::SetASContext( ASContext* _as_context ) {
    as_context = _as_context;

    as_funcs.handle_animation_event         = as_context->RegisterExpectedFunction("void HandleAnimationEvent(string,vec3)", true);
    as_funcs.display_matrix_update          = as_context->RegisterExpectedFunction("void DisplayMatrixUpdate()", true);
    as_funcs.final_animation_matrix_update  = as_context->RegisterExpectedFunction("void FinalAnimationMatrixUpdate(int)", true);
    as_funcs.final_attached_item_update     = as_context->RegisterExpectedFunction("void FinalAttachedItemUpdate(int)", true);
    as_funcs.notify_item_detach             = as_context->RegisterExpectedFunction("void NotifyItemDetach(int)", true);

    as_context->LoadExpectedFunctions();

    anim_client.SetASContext(_as_context);
}

void MorphTarget::CalcVertsModified(int base_model_id, int lod_level) {
    Model& model = Models::Instance()->GetModel(model_id[lod_level]);
    int model_num_verts = model.vertices.size()/3;
    if(morph_targets.empty()){
        verts_modified[lod_level].resize(model_num_verts, false);
        tc_modified[lod_level].resize(model_num_verts, false);
        for(int i=0; i<model_num_verts; ++i){
            if(fabsf(model.vertices[i*3+0]) > EPSILON ||
               fabsf(model.vertices[i*3+1]) > EPSILON ||
               fabsf(model.vertices[i*3+2]) > EPSILON)
            {
                modified_verts[lod_level].push_back(i);
                verts_modified[lod_level][i] = true;
            }
            if(fabsf(model.tex_coords[i*2+0]) > EPSILON ||
               fabsf(model.tex_coords[i*2+1]) > EPSILON)
            {
                modified_tc[lod_level].push_back(i);
                tc_modified[lod_level][i] = true;
            }
        }
    } else {
        verts_modified[lod_level].resize(model_num_verts, false);
        tc_modified[lod_level].resize(model_num_verts, false);
        for(auto & mt : morph_targets){
            mt.CalcVertsModified(base_model_id, lod_level);
            for(int j=0; j<model_num_verts; ++j){
                if(mt.verts_modified[lod_level][j]){
                    verts_modified[lod_level][j] = true;
                }
                if(mt.tc_modified[lod_level][j]){
                    tc_modified[lod_level][j] = true;
                }
            }
        }
        for(int i=0; i<model_num_verts; ++i){
            if(verts_modified[lod_level][i]){
                modified_verts[lod_level].push_back(i);
            }
            if(tc_modified[lod_level][i]){
                modified_tc[lod_level].push_back(i);
            }
        }
    }
}

void MorphTarget::Load(const std::string &base_model_name,
                       int base_model_id,
                       const std::string &_name,
                       int parts,
                       bool absolute_path) 
{
    static const int kBufSize = 256;
    char buf[kBufSize];
    FormatString(buf, kBufSize, "Loading morph target: %s", _name.c_str());
    PROFILER_ZONE_DYNAMIC_STRING(g_profiler_ctx, buf);

    name = _name;
    old_weight = 0.0f;
    weight = 0.0f;
    anim_weight = 0.0f;
    script_weight = 0.0f;
    script_weight_weight = 0.0f;
    checksum = 0;

    for(int & i : model_id){
        i = -1;
    }

    if(parts == 1){       
        std::string path;
        if(absolute_path){
            path = _name;
        } else {
            path = GetMorphPath(base_model_name, _name);
        }

        model_id[0] = Models::Instance()->LoadModelAsMorph(path.c_str(), base_model_id, base_model_name);
        char abs_path[kPathSize];
        if(FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1){
            FatalError("Error", "Could not find %s", path.c_str());
        }
        checksum = Checksum(abs_path);
        model_copy[0] = false;
        if(model_id[0] == -1){
            return;
        }
    } else {
        model_id[0] = Models::Instance()->CopyModel(base_model_id);
        Model& model = Models::Instance()->GetModel(model_id[0]);
        for(float & vertex : model.vertices){
            vertex = 0.0f;
        }
        for(float & tex_coord : model.tex_coords){
            tex_coord = 0.0f;
        }
        model_copy[0] = true;
        for(int i=0; i<parts; ++i){
            MorphTarget mt;
            std::stringstream oss;
            oss << _name << i;
            mt.Load(base_model_name, base_model_id, oss.str());
            checksum += mt.checksum;
            morph_targets.push_back(mt);
        }
    }
    CalcVertsModified(base_model_id, 0);
}

void MorphTarget::PreDrawCamera(int num, int progress, int lod_level, uint32_t char_id) {    
    if (g_debug_runtime_disable_morph_target_pre_draw_camera) {
        return;
    }

    Online* online = Online::Instance();
    MovementObject* mov = (MovementObject*)Engine::Instance()->GetSceneGraph()->GetObjectFromID(char_id);
    RiggedObject* rigged_object = nullptr; 

    if(mov != nullptr) {
        rigged_object = mov->rigged_object();
    }

    float interp_weight = mix(old_weight, weight, game_timer.GetInterpWeightX(num, progress));
    if(!morph_targets.empty()){
        Model& model = Models::Instance()->GetModel(model_id[lod_level]);        
        int num_parts = morph_targets.size();
        mod_weight = max(0.0f,interp_weight*(float)(num_parts)-1.0f);
        int start = max(0, min((int)mod_weight, num_parts-1));
        int end = max(0, min(start+1, num_parts-1));
        float new_weight = mod_weight - (float)start;

		if (online->IsClient()) {
			MorphTargetStateStorage state;
            if(rigged_object != nullptr) {
                if(rigged_object->GetOnlineIncomingMorphTargetState(state, name.c_str())) {
                    mod_weight = state.mod_weight;
                    start = max(0, min((int)mod_weight, num_parts - 1));
                    end = max(0, min(start + 1, num_parts - 1));
                }
            }
		} 

        Model& morph_model_start = Models::Instance()->GetModel(morph_targets[start].model_id[lod_level]);
        Model& morph_model_end = Models::Instance()->GetModel(morph_targets[end].model_id[lod_level]);
        
        if(new_weight == 0.0f || new_weight == 1.0f){
            Model *the_model = (new_weight == 0.0f)?&morph_model_start:&morph_model_end;
            for(int j : modified_verts[lod_level]){
                const int index = j*3;
                model.vertices[index+0] = the_model->vertices[index+0]; 
                model.vertices[index+1] = the_model->vertices[index+1]; 
                model.vertices[index+2] = the_model->vertices[index+2];
            }
            for(int j : modified_tc[lod_level]){
                const int tc_index = j*2;
                model.tex_coords[tc_index+0] = the_model->tex_coords[tc_index+0]; 
                model.tex_coords[tc_index+1] = the_model->tex_coords[tc_index+1];
            }
        } else {
            for(int j : modified_verts[lod_level]){
                const int index = j*3;
                model.vertices[index+0] = morph_model_start.vertices[index+0]*(1.0f-new_weight); 
                model.vertices[index+1] = morph_model_start.vertices[index+1]*(1.0f-new_weight); 
                model.vertices[index+2] = morph_model_start.vertices[index+2]*(1.0f-new_weight); 
                model.vertices[index+0] += morph_model_end.vertices[index+0]*new_weight; 
                model.vertices[index+1] += morph_model_end.vertices[index+1]*new_weight; 
                model.vertices[index+2] += morph_model_end.vertices[index+2]*new_weight; 
            }
            for(int j : modified_tc[lod_level]){
                const int tc_index = j*2;
                model.tex_coords[tc_index+0] = morph_model_start.tex_coords[tc_index+0]*(1.0f-new_weight); 
                model.tex_coords[tc_index+1] = morph_model_start.tex_coords[tc_index+1]*(1.0f-new_weight);
                model.tex_coords[tc_index+0] += morph_model_end.tex_coords[tc_index+0]*new_weight; 
                model.tex_coords[tc_index+1] += morph_model_end.tex_coords[tc_index+1]*new_weight;
            }
        }
        disp_weight = max(0.0f, min(1.0f,interp_weight*(float)(num_parts))); 
    } else {
        disp_weight = max(0.0f, min(1.0f,interp_weight));
    }
}

float MorphTarget::GetScriptWeight()
{
    return script_weight;
}

float MorphTarget::GetWeight() {
    return mix(anim_weight, script_weight, script_weight_weight);
}

void MorphTarget::UpdateForInterpolation()
{
    old_weight = weight;
    weight = GetWeight();
    weight = min(1.0f, max(0.0f, weight));
}

void MorphTarget::Dispose() {
    for(int i=0; i<4; ++i){
        if(model_copy[i]){
            Models::Instance()->DeleteModel(model_id[i]);
        }
    }
    for(auto & morph_target : morph_targets){
        morph_target.Dispose();
    }
}

float RiggedObject::GetStatusKeyValue( const std::string &label ) {
    if(status_keys.find(label) != status_keys.end()){
        return status_keys[label];
    } else {
        return 0.0f;
    }
}

void RiggedObject::ApplyForceToRagdoll( const vec3 &force, const vec3 &position ) {
    std::vector<float> dist(skeleton_.physics_bones.size());
    float total = 0.0f;
    for(unsigned i=0; i<skeleton_.physics_bones.size(); ++i){
        BulletObject* bo = skeleton_.physics_bones[i].bullet_object;
        if(!bo || skeleton_.fixed_obj[i]){
            continue;
        }
        dist[i] = 1.0f / (distance_squared(position, bo->GetPosition())+0.01f);
        dist[i] *= bo->GetMass();
        total += dist[i];
    }
    for(unsigned i=0; i<skeleton_.physics_bones.size(); ++i){
        BulletObject* bo = skeleton_.physics_bones[i].bullet_object;
        if(!bo || skeleton_.fixed_obj[i]){
            continue;
        }
        bo->ApplyForceAtWorldPoint(bo->GetPosition(), force*dist[i] / total);
    }
}

void RiggedObject::ApplyForceToBone( const vec3 &force, const int &bone ) {
    BulletObject* bo = skeleton_.physics_bones[bone].bullet_object;
    bo->ApplyForceAtWorldPoint(bo->GetPosition(), force);
}

void RiggedObject::ApplyForceLineToRagdoll( const vec3 &force, const vec3 &position, const vec3 &line_dir ) {
    std::vector<float> dist(skeleton_.physics_bones.size());
    float total = 0.0f;
    for(unsigned i=0; i<skeleton_.physics_bones.size(); ++i){
        BulletObject* bo = skeleton_.physics_bones[i].bullet_object;
        if(!bo || skeleton_.fixed_obj[i]){
            continue;
        }
        vec3 temp_pos = bo->GetPosition() - position;
        temp_pos -= line_dir * dot(line_dir, temp_pos);
        dist[i] = 1.0f / (length_squared(temp_pos)+0.01f);
        dist[i] *= bo->GetMass();
        total += dist[i];
    }
    for(unsigned i=0; i<skeleton_.physics_bones.size(); ++i){
        BulletObject* bo = skeleton_.physics_bones[i].bullet_object;
        if(!bo || skeleton_.fixed_obj[i]){
            continue;
        }
        bo->ApplyForceAtWorldPoint(bo->GetPosition(), force*dist[i] / total);
    }
}

vec3 RiggedObject::GetDisplayBonePosition(int bone) {
    return display_bone_matrices[bone].GetTranslationPart();
}

vec3 RiggedObject::GetBonePosition( int bone ) {
    return skeleton_.physics_bones[bone].bullet_object->GetPosition();
}

vec3 RiggedObject::GetBoneLinearVel( int bone ) {
    return skeleton_.physics_bones[bone].bullet_object->GetLinearVelocity();
}

mat4 RiggedObject::GetBoneRotation( int bone ) {
    return skeleton_.physics_bones[bone].bullet_object->GetRotation();
}

void RiggedObject::SetSkeletonOwner( Object* owner ) {
    for(auto & physics_bone : skeleton_.physics_bones){
        if(!physics_bone.bullet_object){
            continue;
        }
        physics_bone.bullet_object->owner_object = owner;
    }
}

void RiggedObject::SetAnimUpdatePeriod( int _anim_update_period ) {
    anim_update_period = _anim_update_period;
}

mat4 RiggedObject::GetIKTargetTransform( const std::string &target_name ) {
    int bone = skeleton_.simple_ik_bones[target_name].bone_id;
    return skeleton_.physics_bones[bone].bullet_object->GetTransform();
}

void RiggedObject::HandleAnimationEvents( AnimationClient &_anim_client ) {
    // Loop through animation events and send them to Angelscript to handle
    std::queue<AnimationEvent> active_events = _anim_client.GetActiveEvents();
    while(!active_events.empty()){
        AnimationEvent& the_event = active_events.front();
        vec3 position = skeleton_.physics_bones[the_event.which_bone].
            bullet_object->GetPosition();
        ASArglist args;
        args.AddObject(&the_event.event_string);
        args.AddObject(&position);
        as_context->CallScriptFunction(as_funcs.handle_animation_event, &args);
        active_events.pop();
    }
}

void RiggedObject::SetRagdollStrength( float amount ) {
    ragdoll_strength = amount;
}

mat4 RiggedObject::GetAvgIKChainTransform( const std::string &target_name ) {
    mat4 total(0.0f);
    float total_num = 0.0f;
    SimpleIKBone &sib = skeleton_.simple_ik_bones[target_name];
    int bone = sib.bone_id;
    for(int i=0; i<sib.chain_length; ++i){
        total += skeleton_.physics_bones[bone].bullet_object->GetTransform();
        total_num += 1.0f;
        bone = skeleton_.parents[bone];
        if(bone == -1){
            break;
        }
    }
    return total / total_num;
}

vec3 RiggedObject::GetAvgIKChainPos( const std::string &target_name ) {
    vec3 total;
    float total_num = 0.0f;
    SimpleIKBone &sib = skeleton_.simple_ik_bones[target_name];
    int bone = sib.bone_id;
    for(int i=0; i<sib.chain_length; ++i){
        total += skeleton_.physics_bones[bone].bullet_object->GetPosition();
        total_num += 1.0f;
        bone = skeleton_.parents[bone];
        if(bone == -1){
            break;
        }
    }
    return total / total_num;
}


mat4 RiggedObject::GetIKChainTransform( const std::string &target_name, int which ) {
    SimpleIKBone &sib = skeleton_.simple_ik_bones[target_name];
    int bone = sib.bone_id;
    for(int i=0; i<which; ++i){
        bone = skeleton_.parents[bone];
        if(bone == -1){
            break;
        }
    }
    return skeleton_.physics_bones[bone].bullet_object->GetTransform();
}

vec3 RiggedObject::GetIKChainPos( const std::string &target_name, int which )
{
    SimpleIKBone &sib = skeleton_.simple_ik_bones[target_name];
    int bone = sib.bone_id;
    for(int i=0; i<which; ++i){
        bone = skeleton_.parents[bone];
        if(bone == -1){
            break;
        }
    }
    return skeleton_.physics_bones[bone].bullet_object->GetPosition();
}

void RiggedObject::EnableSleep()
{
    for(auto & physics_bone : skeleton_.physics_bones){
        if(physics_bone.bullet_object){
            physics_bone.bullet_object->CanSleep();
        }
    }
}

void RiggedObject::DisableSleep()
{
    for(auto & physics_bone : skeleton_.physics_bones){
        if(physics_bone.bullet_object){
            physics_bone.bullet_object->NoSleep();
        }
    }
}

void RiggedObject::CheckForNAN()
{
    skeleton_.CheckForNAN();
}

void RiggedObject::HandleLipSyncMorphTargets(float timestep) {
    if(lipsync_reader.valid()){
        lipsync_reader.UpdateWeights();
        const std::map<std::string, float> &mw = lipsync_reader.GetMorphWeights();
        std::map<std::string, float>::const_iterator iter;
        for(iter = mw.begin(); iter != mw.end(); ++iter){
            for(auto & morph_target : morph_targets){
                if(morph_target.name == iter->first){
                    morph_target.script_weight = 
                        mix(morph_target.script_weight, iter->second, 0.5f);
                    morph_target.script_weight_weight = 1.0f;
                    break;
                }
            }    
        }
        lipsync_reader.Update(timestep);
        if(!lipsync_reader.valid()){
            for(iter = mw.begin(); iter != mw.end(); ++iter){
                for(auto & morph_target : morph_targets){
                    if(morph_target.name == iter->first){
                        morph_target.script_weight_weight = 0.0f;
                        break;
                    }
                }    
            }
        }
    }
}

void RiggedObject::CreateBloodDrip( const std::string &ik_name, int ik_link, const vec3 &direction )
{
    Skeleton::SimpleIKBoneMap::iterator iter = 
        skeleton_.simple_ik_bones.find(ik_name);
    if(iter ==  skeleton_.simple_ik_bones.end()){
        return;
    }
    SimpleIKBone &sib = iter->second;
    int look_link = ik_link;
    int bone_id = sib.bone_id;
    while(look_link > 0){
        bone_id = skeleton_.parents[bone_id];
        --look_link;
    }
    vec3 bone_pos = skeleton_.physics_bones[bone_id].initial_position;
  
    blood_surface.CreateBloodDripNearestPointDirection(bone_pos, direction, 1.0f, true);
    blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

void RiggedObject::CreateBloodDripAtBone( int bone_id, int ik_link, const vec3 &direction )
{
	BulletObject* bo = skeleton_.physics_bones[bone_id].bullet_object;
    if(skeleton_.fixed_obj[bone_id]){
        bo = skeleton_.physics_bones[skeleton_.parents[bone_id]].bullet_object;
    }
    if(skeleton_.fixed_obj[skeleton_.parents[bone_id]]){
        bo = skeleton_.physics_bones[skeleton_.parents[skeleton_.parents[bone_id]]].bullet_object;
    }
	vec3 curr_pos = bo->GetPosition();  
    blood_surface.CreateBloodDripNearestPointDirection(curr_pos, direction, 1.0f, true);
    blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

void RiggedObject::CleanBlood() {
    blood_surface.CleanBlood();
}

void RiggedObject::SetCharAnim( const std::string &path, char flags, const std::string &override ) {
    if(char_anim == path && override == char_anim_override){
        return;
    }
    char_anim_override = override;
    char_anim = path;
    char_anim_flags = flags;
    CheckItemAnimBlends(char_anim, flags);
}

const float _item_blend_fade_speed = 5.0f;
void RiggedObject::CheckItemAnimBlends(const std::string &path, char flags) {
    anim_client.ClearBlendAnimation();
    for(auto & iter : attached_items.items)
    {
        ItemObjectScriptReader& item = iter.item;
        if(item->item_ref()->HasAnimBlend(path) && item->state() == ItemObject::kWielded){
            const std::string &a_path = item->item_ref()->GetAnimBlend(path); 
            anim_client.SetBlendAnimation(a_path);
        }
    }
}

void RiggedObject::MPCutPlane(const vec3 & normal, const vec3 & pos, const vec3 & dir, int type, int depth, std::vector<int> mp_hit_list, vec3 mp_points[3])
{
	if (Graphics::Instance()->config_.blood() == BloodLevel::kNone) {
		return;
	}



	Model* model = &Models::Instance()->GetModel(model_id[0]);
	vec3 points[3];

	for (int i = 0; i < 3; i++)
		points[i] = mp_points[i];

	int num_pos_side = 0;
	int num_neg_side = 0;
	int num_hit = 0;
	std::vector<int> hit_list;
	
	hit_list = mp_hit_list;
	num_hit = mp_hit_list.size();



	if (num_hit < 10 && depth < 4) {
		if (num_neg_side > num_pos_side) {
			MPCutPlane(normal, pos - normal * 0.1f, dir, type, depth + 1, mp_hit_list, mp_points);
		}
		else {
			MPCutPlane(normal, pos + normal * 0.1f, dir, type, depth + 1, mp_hit_list, mp_points);
		}
	}
	else {
		//blood_surface.AddBloodToTriangles(hit_list);
		for (int i : hit_list) { 
			GetTransformedTri(i, mp_points);
			if (dot(cross(mp_points[1] - mp_points[0], mp_points[2] - mp_points[0]), dir) > 0.0f) {
				continue;
			}
			float time[3];
			for (unsigned j = 0; j < 3; ++j) {
				const vec3 d = mp_points[(j + 1) % 3] - mp_points[j];
				const vec3 o = mp_points[j] - pos;
				const vec3 &n = normal;
				time[j] = (-o[0] * n[0] - o[1] * n[1] - o[2] * n[2]) /
					(d[0] * n[0] + d[1] * n[1] + d[2] * n[2]);
			}
			vec2 tex_points[3];
			for (unsigned j = 0; j < 3; ++j) {
				tex_points[j][0] = model->tex_coords[model->faces[i * 3 + j] * 2 + 0];
				tex_points[j][1] = model->tex_coords[model->faces[i * 3 + j] * 2 + 1];

				 
			}
			vec2 tex_intersect_points[2];
			vec3 intersect_points[2];
			int which_point = 0;
			for (unsigned j = 0; j < 3; ++j) {
				if (time[j] >= 0.0f && time[j] <= 1.0f) {
					tex_intersect_points[which_point] =
						mix(tex_points[j], tex_points[(j + 1) % 3], time[j]);
					intersect_points[which_point] =
						mix(points[j], mp_points[(j + 1) % 3], time[j]);
					++which_point;
					if (which_point > 1) {
						break;
					}
				}
			}
			//DebugDraw::Instance()->AddLine(intersect_points[0], intersect_points[1], vec4(1.0f), _persistent);
			blood_surface.AddBloodLine(tex_intersect_points[0], tex_intersect_points[1]);


			if (type == _light || RangedRandomFloat(0.0f, 1.0f) < 0.6f) {
				vec2 bleed_point = mix(tex_intersect_points[0], tex_intersect_points[1], 0.5f);
				vec3 coords = bary_coords(bleed_point, tex_points[0], tex_points[1], tex_points[2]); 
				blood_surface.CreateDripInTri(i, coords, RangedRandomFloat(0.1f, 1.0f), RangedRandomFloat(0.0f, 1.0f), true, SurfaceWalker::BLOOD);
			}

			if (type == _heavy || RangedRandomFloat(0.0f, 1.0f) < 0.2f) {
				vec2 bleed_point = mix(tex_intersect_points[0], tex_intersect_points[1], 0.5f);
				vec3 coords = bary_coords(bleed_point, tex_points[0], tex_points[1], tex_points[2]); 
				blood_surface.CreateDripInTri(i, coords, RangedRandomFloat(0.1f, 0.5f), RangedRandomFloat(0.0f, 10.0f), true, SurfaceWalker::BLOOD);
			}

			//dot(offset+dir*time, normal) = 0.0f              
			// ox*nx + dx*t*nx + oy*ny + dy*t*ny + oz*nz + dz*t*nz = 0
			// dx*t*nx + dy*t*ny + dz*t*nz = -ox*nx-oy*ny-oz*nz
			// t = (-ox*nx-oy*ny-oz*nz) / (dx*nx + dy*ny + dz*nz)

			bool add_blood_cloud = (rand() % 10 == 0);
			bool add_blood_splat = (type == _light && rand() % 40 == 0);
			if (add_blood_cloud || add_blood_splat) {
				vec3 pos = (points[0] + points[1] + points[2]) / 3.0f;
				if (add_blood_cloud) {
					scenegraph_->particle_system->MakeParticle(
						scenegraph_, "Data/Particles/bloodcloud.xml", pos,
						vec3(RangedRandomFloat(-1.0f, 1.0f), RangedRandomFloat(-1.0f, 1.0f), RangedRandomFloat(-1.0f, 1.0f)),
						Graphics::Instance()->config_.blood_color());
				}
				if (add_blood_splat) {
					scenegraph_->particle_system->MakeParticle(
						scenegraph_, "Data/Particles/bloodsplat.xml", pos,
						vec3(RangedRandomFloat(-2.0f, 2.0f), RangedRandomFloat(-2.0f, 2.0f), RangedRandomFloat(-2.0f, 2.0f)),
						Graphics::Instance()->config_.blood_color());
				}
			}
		}
	}
	blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

void UpdateAttachedItem(AttachedItem& attached_item, const Skeleton &skeleton, float char_scale, const std::map<int, mat4> weapon_offset, const WeapAnimInfoMap &weap_anim_info_map, const std::map<int, mat4> &weapon_offset_retarget) {
    ItemObjectScriptReader& item = attached_item.item;
    int bone = attached_item.bone_id;
    mat4 bone_mat = skeleton.physics_bones[bone].bullet_object->GetTransform();
    mat4 weap_mat = weapon_offset.find(item->GetID())->second;
    weap_mat.SetTranslationPart(weap_mat.GetTranslationPart() * char_scale);
    mat4 the_weap_mat = bone_mat * weap_mat;
    if(item->state() == ItemObject::kWielded){
        WeapAnimInfoMap::const_iterator iter = weap_anim_info_map.find(item->GetID());
        if(iter != weap_anim_info_map.end()){    
            const WeapAnimInfo &weap_anim_info = iter->second;
            mat4 anim_mat = weap_anim_info.matrix;
            if(weap_anim_info.relative_id != -1){
                anim_mat = skeleton.physics_bones[weap_anim_info.relative_id].bullet_object->GetTransform() * anim_mat;
            }
            anim_mat = anim_mat * weapon_offset_retarget.find(item->GetID())->second;
            the_weap_mat = mix(the_weap_mat, anim_mat, weap_anim_info.weight);
        }
    }
    mat4 temp_weap_mat = the_weap_mat;
    temp_weap_mat.SetColumn(2, the_weap_mat.GetColumn(1)*-1.0f);
    temp_weap_mat.SetColumn(1, the_weap_mat.GetColumn(2));
    if(!item.just_created){
        item.SetPhysicsTransform(temp_weap_mat);
    }
    if(item.just_created){
        item.SetPhysicsTransform(temp_weap_mat);
        item.SetPhysicsTransform(temp_weap_mat);
        item.just_created = false;
    }
}

void UpdateStuckItem(AttachedItem& stuck_item, const Skeleton &skeleton, bool animated) {
    int bone = stuck_item.bone_id;
    mat4 bone_mat = skeleton.physics_bones[bone].bullet_object->GetTransform();
    mat4 the_weap_mat = bone_mat * stuck_item.rel_mat;
    mat4 temp_weap_mat = the_weap_mat;
    ItemObjectScriptReader &item = stuck_item.item;
    item.SetPhysicsTransform(temp_weap_mat);
    if(item.just_created){
        item.SetPhysicsTransform(temp_weap_mat);
        item.just_created = false;
    }
    if(!animated){
        item.SetInterpInfo(1,1);
    }
}

void RiggedObject::UpdateAttachedItems() {
    for(auto & item : attached_items.items){
        UpdateAttachedItem(item, skeleton_, model_char_scale, weapon_offset, weap_anim_info_map, weapon_offset_retarget);
    }
    for(auto & item : stuck_items.items) {
        UpdateStuckItem(item, skeleton_, animated);
    }
}

void RiggedObject::CutPlane( const vec3 &normal, const vec3 &pos, const vec3 &dir, int type, int depth){
    Online* online = Online::Instance();

    if(Graphics::Instance()->config_.blood() == BloodLevel::kNone){
        return;
    }

    Model* model = &Models::Instance()->GetModel(model_id[0]);
    vec3 points[3];
    bool pos_side;
    bool neg_side;
    int num_pos_side = 0;
    int num_neg_side = 0;
    int num_hit = 0;
    std::vector<int> hit_list;
    if (!online->IsClient()) {
        for (int i = 0, len = model->faces.size() / 3; i < len; ++i) {
            GetTransformedTri(i, points);
            pos_side = false;
            neg_side = false;
            for (const auto & point : points) {
                if (dot(point - pos, normal) < 0.0f) {
                    neg_side = true;
                }
                else {
                    pos_side = true;
                }
            }
            if (neg_side && pos_side) {
                hit_list.push_back(i);
                ++num_hit;
            }
            else if (neg_side) {
                ++num_neg_side;
            }
            else {
                ++num_pos_side;
            }
        }
    }

    if(num_hit < 10 && depth < 4){
        if(num_neg_side > num_pos_side){
            CutPlane(normal, pos-normal*0.1f, dir, type, depth+1);
        } else {
            CutPlane(normal, pos+normal*0.1f, dir, type, depth+1);
        }
    } else {
        //blood_surface.AddBloodToTriangles(hit_list);
        for(int i : hit_list){ 
            GetTransformedTri(i, points);
            if(dot(cross(points[1] - points[0], points[2] - points[0]), dir) > 0.0f){
                continue;
            }
            float time[3];
            for(unsigned j=0; j<3; ++j){
                const vec3 d = points[(j+1)%3]-points[j];
                const vec3 o = points[j]-pos;
                const vec3 &n = normal;
                time[j] = (-o[0]*n[0] - o[1]*n[1] - o[2]*n[2])/
                          (d[0]*n[0] + d[1]*n[1] + d[2]*n[2]);
            }
            vec2 tex_points[3];
            for(unsigned j=0; j<3; ++j){
                tex_points[j][0] = model->tex_coords[model->faces[i*3+j]*2+0];
                tex_points[j][1] = model->tex_coords[model->faces[i*3+j]*2+1]; 
            }
            vec2 tex_intersect_points[2];
            vec3 intersect_points[2];
            int which_point = 0;
            for(unsigned j=0; j<3; ++j){
                if(time[j] >= 0.0f && time[j] <= 1.0f){
                    tex_intersect_points[which_point] =
                        mix(tex_points[j], tex_points[(j+1)%3], time[j]);
                    intersect_points[which_point] =
                        mix(points[j], points[(j+1)%3], time[j]);
                    ++which_point;
                    if(which_point>1){
                        break;
                    }
                }
            }
            //DebugDraw::Instance()->AddLine(intersect_points[0], intersect_points[1], vec4(1.0f), _persistent);
            blood_surface.AddBloodLine(tex_intersect_points[0], tex_intersect_points[1]);

            if(type == _light || RangedRandomFloat(0.0f,1.0f)<0.6f){
                vec2 bleed_point = mix(tex_intersect_points[0], tex_intersect_points[1], 0.5f);
                vec3 coords = bary_coords(bleed_point, tex_points[0], tex_points[1], tex_points[2]); 

				//LOGI << "BLEED POINT " << bleed_point << " coords " << coords << std::endl;
                blood_surface.CreateDripInTri(i, coords, RangedRandomFloat(0.1f,1.0f), RangedRandomFloat(0.0f,1.0f), true, SurfaceWalker::BLOOD);
            }

            if(type == _heavy || RangedRandomFloat(0.0f,1.0f)<0.2f){
                vec2 bleed_point = mix(tex_intersect_points[0], tex_intersect_points[1], 0.5f);
                vec3 coords = bary_coords(bleed_point, tex_points[0], tex_points[1], tex_points[2]); 

				//LOGI << "BLEED POINT " << bleed_point << " coords " << coords << std::endl;
                blood_surface.CreateDripInTri(i, coords, RangedRandomFloat(0.1f,0.5f), RangedRandomFloat(0.0f,10.0f), true, SurfaceWalker::BLOOD);
            }

            //dot(offset+dir*time, normal) = 0.0f              
            // ox*nx + dx*t*nx + oy*ny + dy*t*ny + oz*nz + dz*t*nz = 0
            // dx*t*nx + dy*t*ny + dz*t*nz = -ox*nx-oy*ny-oz*nz
            // t = (-ox*nx-oy*ny-oz*nz) / (dx*nx + dy*ny + dz*nz)

            bool add_blood_cloud = (rand()%10 == 0);
            bool add_blood_splat = (type == _light && rand()%40 == 0);
            if(add_blood_cloud || add_blood_splat){
                vec3 pos = (points[0] + points[1] + points[2]) / 3.0f;
                if(add_blood_cloud){
                    scenegraph_->particle_system->MakeParticle(
						scenegraph_, "Data/Particles/bloodcloud.xml", pos,
                        vec3(RangedRandomFloat(-1.0f,1.0f),RangedRandomFloat(-1.0f,1.0f),RangedRandomFloat(-1.0f,1.0f)),
                        Graphics::Instance()->config_.blood_color());
                }
                if(add_blood_splat){
                    scenegraph_->particle_system->MakeParticle(
						scenegraph_, "Data/Particles/bloodsplat.xml",pos,
                        vec3(RangedRandomFloat(-2.0f,2.0f),RangedRandomFloat(-2.0f,2.0f),RangedRandomFloat(-2.0f,2.0f)),
                        Graphics::Instance()->config_.blood_color());
                }
            }
        }            
    }

	if (online->IsHosting()) {
        online->Send<OnlineMessages::CutLine>(
            char_id, 
            points, 
            pos, 
            normal, 
            dir, 
            type, 
            depth, 
            num_hit, 
            hit_list);
	}
    blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

void RiggedObject::Stab( const vec3 &pos, const vec3 &dir, int type, int depth ) {
    const float _stab_threshold = pow(0.001f,1.0f/(float)(depth*0.3f+1));
    Model* model = &Models::Instance()->GetModel(model_id[0]);
    vec3 points[3];
    bool close_enough;
    std::vector<int> hit_list;
    for(int i=0, len=model->faces.size()/3; i<len; ++i){
        close_enough = false;
        GetTransformedTri(i, points);
        for(auto & point : points){
            point -= dir * dot(dir, point - pos);
            if(distance_squared(point, pos) < _stab_threshold){
                close_enough = true;
            }
        }
        if(close_enough){
            hit_list.push_back(i);
        }
    }
    if(hit_list.size() < 5 && depth < 12){
       Stab(pos, dir, type, depth+1);
       return;
    }
    if(Graphics::Instance()->config_.blood() == BloodLevel::kNone){
        return;
    }
    blood_surface.AddDecalToTriangles(hit_list, 
        pos, dir, (depth + 1) * 0.1f,
        stab_texture);
    vec3 start = pos - dir * 1000.0f;
    vec3 end = pos + dir * 1000.0f;
    vec3 intersect;
    for(int i : hit_list){
        GetTransformedTri(i, points);
        if(LineFacet(start, end, points[0], points[1], points[2], &intersect)){
            for(unsigned j=0; j<10; ++j){
                blood_surface.CreateDripInTri(i, vec3(0.333f,0.333f,0.333f), RangedRandomFloat(0.1f,1.0f), RangedRandomFloat(0.0f,10.0f), true, SurfaceWalker::BLOOD);
            }
        }
    }

    for(unsigned i=0; i<hit_list.size(); ++i){
        GetTransformedTri(hit_list[i], points);
        vec3 pos = (points[0] + points[1] + points[2]) / 3.0f;
        if(rand()%(hit_list.size()) == 0){
            scenegraph_->particle_system->MakeParticle(
				scenegraph_, "Data/Particles/bloodcloud.xml",pos,
                vec3(RangedRandomFloat(-1.0f,1.0f),RangedRandomFloat(-1.0f,1.0f),RangedRandomFloat(-1.0f,1.0f))+dir,
                Graphics::Instance()->config_.blood_color());
            scenegraph_->particle_system->MakeParticle(
				scenegraph_, "Data/Particles/bloodcloud.xml",pos,
                vec3(RangedRandomFloat(-1.0f,1.0f),RangedRandomFloat(-1.0f,1.0f),RangedRandomFloat(-1.0f,1.0f))-dir,
                Graphics::Instance()->config_.blood_color());
        }
        if(type == _light && rand()%40 == 0){
            scenegraph_->particle_system->MakeParticle(
				scenegraph_, "Data/Particles/bloodsplat.xml",pos,
                vec3(RangedRandomFloat(-2.0f,2.0f),RangedRandomFloat(-2.0f,2.0f),RangedRandomFloat(-2.0f,2.0f)),
                Graphics::Instance()->config_.blood_color());
        }
    }     
    blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

std::vector<int> water_logged;

void RiggedObject::AddWaterCube( const mat4 &transform ) {
    if(blood_surface.blood_tex.valid()){
        PROFILER_GPU_ZONE(g_profiler_ctx, "RiggedObject::AddWaterCube");

        std::swap(blood_surface.blood_tex, blood_surface.blood_work_tex);
        blood_surface.StartDrawingToBloodTexture();
        Graphics::Instance()->setBlend(false);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        Graphics::Instance()->Clear(true);
        Shaders* shaders = Shaders::Instance();
        int the_shader = shaders->returnProgram(water_cube);

        if(the_shader!=-1) {
            shaders->setProgram(the_shader);

            std::vector<mat4> fire_decal_matrix;
            std::vector<mat4> water_decal_matrix;
            for(auto & decal_object : scenegraph_->decal_objects_){
                DecalObject* obj = (DecalObject*)decal_object;
                if(obj->decal_file_ref->special_type == 3){
                    fire_decal_matrix.push_back(obj->GetTransform());
                }
                if(obj->decal_file_ref->special_type == 5){
                    water_decal_matrix.push_back(obj->GetTransform());
                }
            }
            shaders->SetUniformInt("fire_decal_num", fire_decal_matrix.size());
            if(!fire_decal_matrix.empty()){
                shaders->SetUniformMat4Array("fire_decal_matrix", fire_decal_matrix);
            }
            shaders->SetUniformInt("water_decal_num", water_decal_matrix.size());
            if(!water_decal_matrix.empty()){
                shaders->SetUniformMat4Array("water_decal_matrix", water_decal_matrix);
            }

            shaders->SetUniformMat4("transform", transform);
            shaders->SetUniformFloat("time", game_timer.GetRenderTime());
            Textures::Instance()->bindTexture(blood_surface.blood_work_tex);

            Model* model;
            model =&Models::Instance()->GetModel(model_id[lod_level]);
            DrawModel(model, lod_level);
        }
        blood_surface.EndDrawingToBloodTexture();

        // Expand blood texels by one pixel to fill in seams
        std::swap(blood_surface.blood_tex, blood_surface.blood_work_tex);
        blood_surface.StartDrawingToBloodTexture();
        Graphics::Instance()->setBlend(false);
        Graphics::Instance()->SetBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        int shader_id = Shaders::Instance()->returnProgram(water_cube_expand);
        Shaders::Instance()->setProgram(shader_id);
        Textures::Instance()->bindTexture(blood_surface.blood_work_tex);
        Shaders::Instance()->SetUniformInt("tex_width", Textures::Instance()->getWidth(blood_surface.blood_work_tex));
        Shaders::Instance()->SetUniformInt("tex_height", Textures::Instance()->getHeight(blood_surface.blood_work_tex));

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

        int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_coord", shader_id);
        Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
        glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2*sizeof(GLfloat), 0);
        Graphics::Instance()->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        Graphics::Instance()->ResetVertexAttribArrays();

        blood_surface.EndDrawingToBloodTexture();
        Textures::Instance()->GenerateMipmap(blood_surface.blood_tex);
    }
}

void RiggedObject::UpdateCollisionObjects() {
    for(auto & physics_bone : skeleton_.physics_bones){
        BulletObject* cbo = physics_bone.col_bullet_object;
        if(!cbo){
            continue;
        }
        cbo->SetTransform(physics_bone.bullet_object->GetTransform());
        cbo->UpdateTransform();
    }
    skeleton_.UpdateCollisionWorldAABB();
}

void RiggedObject::AddBloodAtPoint( const vec3 & point ) {
    blood_surface.CreateBloodDripNearestPointTransformed(point, 0.1f, false);
    blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

enum LogReadStage { _deleted_faces, _first_vert, _second_vert, _offset_x, _offset_y, _offset_z, _modified_faces };

static void ASSetBoneInflate(Skeleton* skeleton, int which, float val){
    skeleton->physics_bones[which].display_scale = val;
}

static int ASGetParent(Skeleton* skeleton, int which){
    return skeleton->parents[which];
}

static bool ASIKBoneExists(Skeleton* skeleton, const std::string& name){
    return skeleton->simple_ik_bones.find(name) != skeleton->simple_ik_bones.end();
}

int ASIKBoneStart(Skeleton* skeleton, const std::string& name){
    return skeleton->simple_ik_bones[name].bone_id;
}

int ASIKBoneLength(Skeleton* skeleton, const std::string& name){
    return skeleton->simple_ik_bones[name].chain_length;
}

static int ASNumBones(Skeleton* skeleton){
    return skeleton->physics_bones.size();
}

static vec3 ASGetBoneLinearVelocity(Skeleton* skeleton, int which){
    if(!skeleton->physics_bones[which].bullet_object){
        return vec3(0.0f);
    } else {
        return skeleton->physics_bones[which].bullet_object->GetLinearVelocity();
    }
}

static mat4 ASGetBoneTransform(Skeleton* skeleton, int which){
    mat4 return_mat = skeleton->physics_bones[which].bullet_object->GetRotation();
    return_mat.SetTranslationPart(skeleton->physics_bones[which].bullet_object->GetPosition());
    return return_mat;
}

static void ASSetBoneTransform(Skeleton* skeleton, int which, const mat4 &transform){
    skeleton->physics_bones[which].bullet_object->SetTransform(transform);
}

static BoneTransform ASGetFrameMatrix(RiggedObject* rigged_object, int which){
    return rigged_object->animation_frame_bone_matrices[which];
}

static void ASSetFrameMatrix(RiggedObject* rigged_object, int which, const BoneTransform &transform){
    rigged_object->animation_frame_bone_matrices[which] = transform;
}

static vec3 ASGetPointPos(Skeleton* skeleton, int which){
    return skeleton->points[which];
}

static int ASGetBonePoint(Skeleton* skeleton, int which_bone, int which_point){
    return skeleton->bones[which_bone].points[which_point];
}

static void ASAddVelocity(Skeleton* skeleton, const vec3& vel){
    for(auto & physics_bone : skeleton->physics_bones){
        BulletObject* bullet_object = physics_bone.bullet_object;
        if(bullet_object){
            bullet_object->AddLinearVelocity(vel);
        }
    }
}

void RotateBoneToMatchVec(RiggedObject* rigged_object, const vec3 &a, const vec3 &b, int bone){
    BoneTransform &mat = rigged_object->animation_frame_bone_matrices[bone];
    mat.origin = (a+b)*0.5f;
    vec3 dir = mat.rotation * vec3(0,0,1);
    quaternion rot;
    GetRotationBetweenVectors(dir, b - a, rot);
    mat.rotation = rot * mat.rotation;
}

void RotateBonesToMatchVec(RiggedObject* rigged_object, const vec3& a, const vec3& c, int bone, int bone2, float weight) {
    vec3 b = mix(a,c,1.0f-weight);

    BoneTransform &mat = rigged_object->animation_frame_bone_matrices[bone];
    mat.origin = (a+b)*0.5f;
    vec3 dir = mat.rotation * vec3(0,0,1);
    quaternion rot;
    GetRotationBetweenVectors(dir, b - a, rot);
    mat.rotation = rot * mat.rotation;

    BoneTransform &mat2 = rigged_object->animation_frame_bone_matrices[bone2];
    mat2.origin = (b+c)*0.5f;
    mat2.rotation = rot * mat2.rotation;
}

static void TransformAllFrameMats(RiggedObject* rigged_object, const BoneTransform &transform) {
    std::vector<BoneTransform> &mats = rigged_object->animation_frame_bone_matrices;
    for(auto & mat : mats){
        mat = transform * mat;
    }
}

static vec3 GetFrameCenterOfMass(RiggedObject* rigged_object) {
    Skeleton& skeleton = rigged_object->skeleton_;
    int num_bones = skeleton.physics_bones.size();
    vec3 com;
    float total_mass = 0.0f;
    for(int i=0; i<num_bones; ++i){
        BoneTransform &transform = rigged_object->animation_frame_bone_matrices[i];
        float bone_mass = skeleton.bones[i].mass;
        com += transform.origin * bone_mass;
        total_mass += bone_mass;
    }
    if(total_mass != 0.0f){
        com /= total_mass;
    }
    return com;
}

vec3 GetTransformedBonePoint(RiggedObject* rigged_object, int bone, int point){
    Skeleton& skeleton = rigged_object->skeleton_;
    BoneTransform transform = rigged_object->animation_frame_bone_matrices[bone] * 
                              rigged_object->cached_skeleton_info_.bind_matrices[bone];
    const vec3 &bone_point = skeleton.points[skeleton.bones[bone].points[point]];
    return transform * bone_point;
}

static float ASGetBoneMass(Skeleton* skeleton, int which_bone){
    return skeleton->bones[which_bone].mass;
}

vec3 ASGetModelCenter(RiggedObject* rigged_object){
    const Model& model = Models::Instance()->GetModel(rigged_object->model_id[0]);
    return model.old_center;
}

static mat4 GetDisplayBoneMatrix(RiggedObject* rigged_object, int bone){
    return rigged_object->display_bone_matrices[bone];
}

static void SetDisplayBoneMatrix(RiggedObject* rigged_object, int bone, const mat4 &matrix){
    rigged_object->display_bone_matrices[bone] = matrix;
}

static bool ASHasPhysics(Skeleton* skeleton, int which){
    return (skeleton->physics_bones[which].bullet_object != NULL);
}

static CScriptArray *ASGetConvexHullPoints(Skeleton* skeleton, int bone){
    std::vector<float> points;
    BulletObject* bullet_obj = skeleton->physics_bones[bone].col_bullet_object;
    if(bullet_obj){
        btCollisionShape* shape = bullet_obj->shape.get();
        if(shape->getShapeType() == CONVEX_HULL_SHAPE_PROXYTYPE){
            btConvexHullShape* convex_hull_shape = (btConvexHullShape*)shape;
            int num_verts = convex_hull_shape->getNumVertices();
            points.reserve(num_verts*3);
            for(int i=0; i<num_verts; ++i){
                btVector3 vert;
                convex_hull_shape->getVertex(i, vert);
                points.push_back(vert[0]);
                points.push_back(vert[1]);
                points.push_back(vert[2]);
            }
        }
    }
    
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<float>"));
    CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(points.size());

    for(float & point : points) {
        array->InsertLast(&point);
    }        
    return array;
}

static BoneTransform ASApplyParentRotations(Skeleton* skeleton, const CScriptArray *matrices, int id) {
    int parent_id = skeleton->parents[id];
    if(parent_id == -1){
        return *(BoneTransform*)(matrices->At(id));
    } else {
        return ASApplyParentRotations(skeleton, matrices, parent_id) * *(BoneTransform*)(matrices->At(id));
    }
}

static void ASRemoveAllLayers(AnimationClient* animation_client){
    animation_client->RemoveAllLayers();
}

static void ASSetFire(RiggedObject* rigged_object, float fire){
    rigged_object->blood_surface.SetFire(fire);
}

static void ASIgnite(RiggedObject* rigged_object){
    rigged_object->blood_surface.Ignite();
    rigged_object->blood_surface.sleep_time = game_timer.game_time + 5.0f;
}

static void ASExtinguish(RiggedObject* rigged_object){
    rigged_object->blood_surface.Extinguish();
}

static void ASSetWet(RiggedObject* rigged_object, float wet){
    rigged_object->blood_surface.SetWet(wet);
}

void DefineRiggedObjectTypePublic(ASContext* ctx) {
    if(ctx->TypeExists("AnimationClient")){
        return;
    }
    ctx->RegisterObjectType("AnimationClient", 0, asOBJ_REF | asOBJ_NOCOUNT);
    ctx->RegisterObjectMethod("AnimationClient","float GetAnimationEventTime(const string &in event)",asMETHOD(AnimationClient, GetAnimationEventTime), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","float GetTimeUntilEvent(const string &in event)",asMETHOD(AnimationClient, GetTimeUntilEvent), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void SetAnimationCallback(const string &in function)",asMETHOD(AnimationClient, SetCallbackString), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void AddAnimationOffset(const vec3 &in)",asMETHOD(AnimationClient, AddAnimationOffset), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void AddAnimationRotOffset(float radians)",asMETHOD(AnimationClient, AddAnimationRotOffset), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","int AddLayer(const string &in anim_path, float transition_speed, int8 flags)",asMETHOD(AnimationClient, AddLayer), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","int RemoveLayer(int id, float transition_speed)",asMETHOD(AnimationClient, RemoveLayer), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void SetBlendCoord(const string &in coord_label, float coord_value)",asMETHOD(AnimationClient, SetBlendCoord), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void SetLayerOpacity(int id, float opacity)",asMETHOD(AnimationClient, SetLayerOpacity), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void SetSpeedMult(float)",asMETHOD(AnimationClient, SetSpeedMult), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void SetAnimatedItemID(int index, int id)",asMETHOD(AnimationClient, SetAnimatedItemID), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void SetLayerItemID(int layer, int index, int id)",asMETHOD(AnimationClient, SetLayerItemID), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","const string& GetCurrAnim()",asMETHOD(AnimationClient, GetCurrAnim), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","float GetNormalizedAnimTime()",asMETHOD(AnimationClient, GetNormalizedAnimTime), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","float GetAnimationSpeed()",asMETHOD(AnimationClient, GetAnimationSpeed), asCALL_THISCALL);
    ctx->RegisterObjectMethod("AnimationClient","void RemoveAllLayers()",asFUNCTION(ASRemoveAllLayers), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("AnimationClient","void Reset()",asMETHOD(AnimationClient, Reset), asCALL_THISCALL);
    ctx->DocsCloseBrace();

    ctx->RegisterObjectType("Skeleton", 0, asOBJ_REF | asOBJ_NOCOUNT);
    ctx->RegisterObjectMethod("Skeleton","vec3 GetCenterOfMass()",asMETHOD(Skeleton, GetCenterOfMass), asCALL_THISCALL);
    ctx->RegisterObjectMethod("Skeleton","void SetBoneInflate(int bone_id, float amount)",asFUNCTION(ASSetBoneInflate), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","int GetParent(int bone_id)",asFUNCTION(ASGetParent), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","bool IKBoneExists(const string& in)",asFUNCTION(ASIKBoneExists), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","float GetBoneMass(int bone_id)",asFUNCTION(ASGetBoneMass), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","int IKBoneStart(const string& in IK_label)",asFUNCTION(ASIKBoneStart), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","int IKBoneLength(const string& in IK_label)",asFUNCTION(ASIKBoneLength), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","int NumBones()",asFUNCTION(ASNumBones), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","vec3 GetBoneLinearVelocity(int which)",asFUNCTION(ASGetBoneLinearVelocity), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","mat4 GetBoneTransform(int bone_id)",asFUNCTION(ASGetBoneTransform), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","void SetBoneTransform(int bone_id, const mat4 &in transform)",asFUNCTION(ASSetBoneTransform), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","vec3 GetPointPos(int bone_id)",asFUNCTION(ASGetPointPos), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","int GetBonePoint(int bone_id, int which_point)",asFUNCTION(ASGetBonePoint), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","void AddVelocity(const vec3 &in vel)",asFUNCTION(ASAddVelocity), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","mat4 GetBindMatrix(int bone_id)",asFUNCTION(ASGetBindMatrix), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","bool HasPhysics(int bone_id)",asFUNCTION(ASHasPhysics), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","array<float> @GetConvexHullPoints(int bone_id)",asFUNCTION(ASGetConvexHullPoints), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("Skeleton","BoneTransform ApplyParentRotations(const array<BoneTransform> &in matrices, int id)",asFUNCTION(ASApplyParentRotations), asCALL_CDECL_OBJFIRST);
    ctx->DocsCloseBrace();

    ctx->RegisterObjectType("RiggedObject", 0, asOBJ_REF | asOBJ_NOCOUNT);
    ctx->RegisterObjectProperty("RiggedObject", "bool ik_enabled", asOFFSET(RiggedObject,ik_enabled));
    ctx->RegisterObjectProperty("RiggedObject", "int anim_update_period", asOFFSET(RiggedObject,anim_update_period));
    ctx->RegisterObjectProperty("RiggedObject", "int curr_anim_update_time", asOFFSET(RiggedObject,curr_anim_update_time));
	ctx->RegisterObjectMethod("RiggedObject","vec3 GetBonePosition(int bone)",asMETHOD(RiggedObject, GetBonePosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void AddWaterCube(const mat4 &in transform)",asMETHOD(RiggedObject, AddWaterCube), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void Ignite()",asFUNCTION(ASIgnite), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void Extinguish()",asFUNCTION(ASExtinguish), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void SetFire(float fire)",asFUNCTION(ASSetFire), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void SetWet(float wet)",asFUNCTION(ASSetWet), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","BoneTransform GetFrameMatrix(int which)",asFUNCTION(ASGetFrameMatrix), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void SetFrameMatrix(int which, const BoneTransform &in transform)",asFUNCTION(ASSetFrameMatrix), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","float GetCharScale()", asMETHOD(RiggedObject,GetCharScale), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","float GetRelativeCharScale()", asMETHOD(RiggedObject,GetRelativeCharScale), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void Draw()", asMETHOD(RiggedObject,Draw), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void CutPlane(const vec3 &in normal, const vec3 &in position, const vec3 &in direction, int type, int depth)",asMETHOD(RiggedObject, CutPlane), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void Stab(const vec3 &in position, const vec3 &in direction, int type, int depth)",asMETHOD(RiggedObject, Stab), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void SetRagdollStrength(float)",asMETHOD(RiggedObject, SetRagdollStrength), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void RefreshRagdoll()",asMETHOD(RiggedObject, RefreshRagdoll), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void SetRagdollDamping(float)",asMETHOD(RiggedObject, SetDamping), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void ApplyForceToRagdoll(const vec3 &in force, const vec3 &in position)",asMETHOD(RiggedObject, ApplyForceToRagdoll), asCALL_THISCALL);
	ctx->RegisterObjectMethod("RiggedObject","void ApplyForceToBone(const vec3 &in force, const int &in bone)",asMETHOD(RiggedObject, ApplyForceToBone), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void ApplyForceLineToRagdoll(const vec3 &in force, const vec3 &in position, const vec3 &in line_direction)",asMETHOD(RiggedObject, ApplyForceLineToRagdoll), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void MoveRagdollPart(const string &in, const vec3 &in, float)",asMETHOD(RiggedObject, MoveRagdollPart), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void FixedRagdollPart(int boneID, const vec3 &in)",asMETHOD(RiggedObject, FixedRagdollPart), asCALL_THISCALL);
	ctx->RegisterObjectMethod("RiggedObject","void SpikeRagdollPart(int boneID, const vec3 &start, const vec3 &end, const vec3 &pos)",asMETHOD(RiggedObject, SpikeRagdollPart), asCALL_THISCALL);
	ctx->RegisterObjectMethod("RiggedObject","void ClearBoneConstraints()",asMETHOD(RiggedObject, ClearBoneConstraints), asCALL_THISCALL);
	ctx->RegisterObjectMethod("RiggedObject","vec3 GetAvgPosition()",asMETHOD(RiggedObject, GetAvgPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetAvgVelocity()",asMETHOD(RiggedObject, GetAvgVelocity), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void CleanBlood()",asMETHOD(RiggedObject, CleanBlood), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void CreateBloodDrip(const string &in IK_label, int IK_link, const vec3 &in raycast_direction)",asMETHOD(RiggedObject, CreateBloodDrip), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void CreateBloodDripAtBone(int bone_id, int IK_link, const vec3 &in raycast_direction)",asMETHOD(RiggedObject, CreateBloodDripAtBone), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetAvgAngularVelocity()",asMETHOD(RiggedObject, GetAvgAngularVelocity), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetIKTargetPosition(const string &in IK_label)",asMETHOD(RiggedObject, GetIKTargetPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","float GetIKWeight(const string &in IK_label)",asMETHOD(RiggedObject, GetIKWeight), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","BoneTransform GetIKTransform(const string &in IK_label)",asFUNCTION(GetIKTransform), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","mat4 GetUnmodifiedIKTransform(const string &in IK_label)",asFUNCTION(GetUnmodifiedIKTransform), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","mat4 GetIKTargetTransform(const string &in IK_label)",asMETHOD(RiggedObject, GetIKTargetTransform), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void SetMorphTargetWeight(const string &in morph_label, float weight, float weight_weight)",asMETHOD(RiggedObject, SetMTTargetWeight), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","float GetStatusKeyValue(const string &in status_key_label)",asMETHOD(RiggedObject, GetStatusKeyValue), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetIKTargetAnimPosition(const string &in IK_label)",asMETHOD(RiggedObject, GetIKTargetAnimPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","mat4 GetAvgIKChainTransform(const string &in IK_label)",asMETHOD(RiggedObject, GetAvgIKChainTransform), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","mat4 GetIKChainTransform(const string &in IK_label, int IK_link)",asMETHOD(RiggedObject, GetIKChainTransform), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetIKChainPos(const string &in IK_label, int IK_link)",asMETHOD(RiggedObject, GetIKChainPos), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void DisableSleep()",asMETHOD(RiggedObject, DisableSleep), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void EnableSleep()",asMETHOD(RiggedObject, EnableSleep), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetAvgIKChainPos(const string &in IK_label)",asMETHOD(RiggedObject, GetAvgIKChainPos), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void SetAnimUpdatePeriod(int steps)",asMETHOD(RiggedObject, SetAnimUpdatePeriod), asCALL_THISCALL, "Animation is updated every N engine time steps");
    ctx->RegisterObjectMethod("RiggedObject","void SheatheItem(int item_id, bool on_right)",asMETHOD(RiggedObject, SheatheItem), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void UnSheatheItem(int item_id, bool on_right)",asMETHOD(RiggedObject, UnSheatheItem), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject", "void UnStickItem(int item_id)", asMETHOD(RiggedObject,UnStickItem), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","void SetPrimaryWeaponID(int item_id)",asMETHOD(RiggedObject, SetPrimaryWeaponID), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetModelCenter()",asFunctionPtr(ASGetModelCenter), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void RotateBoneToMatchVec(const vec3 &in a, const vec3 &in b, int bone)",asFunctionPtr(RotateBoneToMatchVec), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void RotateBonesToMatchVec(const vec3 &in a, const vec3 &in c, int bone, int bone2, float weight)",asFunctionPtr(RotateBonesToMatchVec), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void TransformAllFrameMats(const BoneTransform &in transform)",asFunctionPtr(TransformAllFrameMats), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetFrameCenterOfMass()",asFunctionPtr(GetFrameCenterOfMass), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","vec3 GetTransformedBonePoint(int bone, int point)",asFunctionPtr(GetTransformedBonePoint), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","mat4 GetDisplayBoneMatrix(int bone)",asFunctionPtr(GetDisplayBoneMatrix), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","void SetDisplayBoneMatrix(int bone, const mat4 &in transform)",asFunctionPtr(SetDisplayBoneMatrix), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("RiggedObject","Skeleton &skeleton()",asMETHOD(RiggedObject, skeleton), asCALL_THISCALL);
    ctx->RegisterObjectMethod("RiggedObject","AnimationClient &anim_client()",asMETHOD(RiggedObject, GetAnimClient), asCALL_THISCALL);
    ctx->DocsCloseBrace();
}

AnimationClient &RiggedObject::GetAnimClient() {
    return anim_client;
}

static const int simp_cache_vers = 11;

void RiggedObject::SaveSimplificationCache(const std::string &path) {
    FILE *file;
    Path found_path = FindFilePath(path.c_str(), kAnyPath, false);
    if(found_path.isValid())
        file = my_fopen((GetWritePath(found_path.GetModsource()).c_str()+path+".lod_cache").c_str(), "wb");
    else {
        LOGW << "Couldn't find path to \"" << path << "\" when saving simplification cache. This might result in a mod writing to the wrong directory" << std::endl;
        file = my_fopen((GetWritePath(CoreGameModID).c_str()+path+".lod_cache").c_str(), "wb");
    }
    unsigned short checksum = skeleton_.skeleton_asset_ref->checksum()+
        Models::Instance()->GetModel(model_id[0]).checksum;
    for(auto & morph_target : morph_targets){
        checksum += morph_target.checksum;
    }
    fwrite(&simp_cache_vers, sizeof(int), 1, file);
    fwrite(&checksum, sizeof(unsigned short), 1, file);
    for(unsigned i=1; i<4; ++i){
        const Model& model = Models::Instance()->GetModel(model_id[i]);
        int model_num_verts = model.vertices.size()/3;
        fwrite(&model_num_verts,    sizeof(int),     1,                    file);
        fwrite(&model.vertices[0],     sizeof(GLfloat), model_num_verts*3, file);
        fwrite(&model.bone_weights[0], sizeof(vec4),    model_num_verts,   file);
        fwrite(&model.bone_ids[0],     sizeof(vec4),    model_num_verts,   file);
        fwrite(&model.tex_coords[0],   sizeof(GLfloat), model_num_verts*2, file);
        int num_faces = model.faces.size()/3;
        fwrite(&num_faces,  sizeof(int),     1,                    file);
        fwrite(&model.faces[0],        sizeof(GLuint),  model.faces.size(),    file);
    }

    {
        int num_morphs = morph_targets.size();
        fwrite(&num_morphs, sizeof(int), 1, file);
    }

    std::queue<MorphTarget*> queue;
    for(auto & morph_target : morph_targets){
        queue.push(&morph_target);
    }
    while(!queue.empty()){
        MorphTarget& morph = (*queue.front());
        queue.pop();
        for(auto & morph_target : morph.morph_targets){
            queue.push(&morph_target);
        }
        for(unsigned i=1; i<4; ++i){
            const Model& model = Models::Instance()->GetModel(morph.model_id[i]);
            int model_num_verts = model.vertices.size()/3;
            fwrite(&model.vertices[0],      sizeof(GLfloat), model_num_verts*3, file);
            fwrite(&model.tex_coords[0],    sizeof(GLfloat), model_num_verts*2, file);
            std::vector<char> &vm = morph.verts_modified[i];
            fwrite(&vm[0],sizeof(char), model_num_verts,   file);
            int mv_size = morph.modified_verts[i].size();
            fwrite(&mv_size,             sizeof(int),     1,                    file);
            if(mv_size){
                std::vector<int> &mv = morph.modified_verts[i];
                fwrite(&mv[0],sizeof(int),  mv_size,              file);
            }

            std::vector<char> &tcm = morph.tc_modified[i];
            fwrite(&tcm[0],sizeof(char), model_num_verts,   file);
            int mtc_size = morph.modified_tc[i].size();
            fwrite(&mtc_size, sizeof(int), 1, file);
            if(mtc_size){
                std::vector<int> &mtc = morph.modified_tc[i];
                fwrite(&mtc[0],sizeof(int), mtc_size, file);
            }
        }
    }
    fclose(file);
}

bool RiggedObject::LoadSimplificationCache(const std::string& path) {
    char lod_cache_path[kPathSize];
    if(FindFilePath((path+".lod_cache").c_str(), lod_cache_path, kPathSize, kAnyPath, false) == -1){
        return false;
    }
    FILE *file = my_fopen(lod_cache_path, "rb");
    if(!file){
        return false;
    }
    int vers;
    fread(&vers, sizeof(int), 1, file);
    if(vers != simp_cache_vers){
        return false;
    }
    unsigned short checksum = skeleton_.skeleton_asset_ref->checksum()+
        Models::Instance()->GetModel(model_id[0]).checksum;
    for(auto & morph_target : morph_targets){
        checksum += morph_target.checksum;
    }
    unsigned short file_checksum;
    fread(&file_checksum, sizeof(unsigned short), 1, file);
    if(file_checksum != checksum){
        return false;
    }
    for(unsigned i=1; i<4; ++i){
        model_id[i] = Models::Instance()->AddModel();
        Model& model = Models::Instance()->GetModel(model_id[i]);
        int model_num_verts;
        fread(&model_num_verts, sizeof(int),     1,                    file);
        model.vertices.resize(model_num_verts*3);
        model.bone_weights.resize(model_num_verts);
        model.bone_ids.resize(model_num_verts);
        model.tex_coords.resize(model_num_verts*2);
        model.transform_vec.resize(4);
        model.transform_vec[0].resize(model_num_verts);
        model.transform_vec[1].resize(model_num_verts);
        model.transform_vec[2].resize(model_num_verts);
        model.transform_vec[3].resize(model_num_verts);
        model.tex_transform_vec.resize(model_num_verts);
        model.morph_transform_vec.resize(model_num_verts);
        fread(&model.vertices[0],     sizeof(GLfloat), model_num_verts*3, file);
        fread(&model.bone_weights[0], sizeof(vec4),    model_num_verts,   file);
        fread(&model.bone_ids[0],     sizeof(vec4),    model_num_verts,   file);
        fread(&model.tex_coords[0],   sizeof(GLfloat), model_num_verts*2, file);
        int num_faces;
        fread(&num_faces,    sizeof(int),     1,                    file);
        model.faces.resize(num_faces*3);
        fread(&model.faces[0],        sizeof(GLuint),  num_faces*3,    file);
    }

    {
        int num_morphs;
        fread(&num_morphs, sizeof(int), 1, file);
        if(num_morphs != (int)morph_targets.size()){
            return false;
        }
    }

    std::queue<MorphTarget*> queue;
    for(auto & morph_target : morph_targets){
        queue.push(&morph_target);
    }
    while(!queue.empty()){
        MorphTarget& morph = (*queue.front());
        queue.pop();
        for(auto & morph_target : morph.morph_targets){
            queue.push(&morph_target);
        }
        for(unsigned i=1; i<4; ++i){
            morph.model_id[i] = Models::Instance()->AddModel();
            morph.model_copy[i] = true;
            const Model& base_model = Models::Instance()->GetModel(model_id[i]);
            Model& model = Models::Instance()->GetModel(morph.model_id[i]);
            int model_num_verts = base_model.vertices.size()/3;
            int base_model_num_verts = base_model.vertices.size()/3;
            model.vertices.resize(base_model_num_verts*3);
            model.tex_coords.resize(base_model_num_verts*2);
            morph.verts_modified[i].resize(base_model_num_verts);
            morph.tc_modified[i].resize(base_model_num_verts);
            fread(&model.vertices[0], sizeof(GLfloat), model_num_verts*3, file);
            fread(&model.tex_coords[0], sizeof(GLfloat), model_num_verts*2, file);

            std::vector<char> &vm = morph.verts_modified[i];
            fread(&vm[0],sizeof(char), model_num_verts,   file);
            int mv_size;
            fread(&mv_size, sizeof(int), 1, file);
            morph.modified_verts[i].resize(mv_size);
            std::vector<int> &mv = morph.modified_verts[i];
            if(mv_size){
                fread(&mv[0],sizeof(int),  mv_size, file);
            }

            std::vector<char> &tcm = morph.tc_modified[i];
            fread(&tcm[0],sizeof(char), model_num_verts,   file);
            int mtc_size;
            fread(&mtc_size, sizeof(int), 1, file);
            morph.modified_tc[i].resize(mtc_size);
            std::vector<int> &mtc = morph.modified_tc[i];
            if(mtc_size){
                fread(&mtc[0],sizeof(int),  mtc_size, file);
            }
        }
    }
    fclose(file);
    return true;
}

void RiggedObject::CreateFurModel( int model_id, int fur_model_id ) {
    const Model &base_model = Models::Instance()->GetModel(model_id);
    const Model &fur_ref_model = Models::Instance()->GetModel(fur_model_id);
    
    // Create map to look up duplicate vertex positions
    std::map<vec3, std::vector<int> > base_model_points;
    for(unsigned i=0; i<base_model.vertices.size(); i+=3){
        base_model_points[vec3(base_model.vertices[i+0],
                               base_model.vertices[i+1],
                               base_model.vertices[i+2])].push_back(i/3);
    }

    {
        // Create vector of fur vertices offset by base model center
        std::vector<vec3> fur_verts;
        fur_verts.reserve(fur_ref_model.vertices.size()/3);
        for(unsigned i=0; i<fur_ref_model.vertices.size(); i+=3){
            fur_verts.push_back(vec3(fur_ref_model.vertices[i+0],
                                     fur_ref_model.vertices[i+1],
                                     fur_ref_model.vertices[i+2]));
            fur_verts.back() -= base_model.old_center;
        }

        // Create fur model and populate it with tris that have at least one vertex
        // that is not in the base model
        int new_tris = 0;
        const std::vector<GLuint> &faces = fur_ref_model.faces;
        for(unsigned i=0; i<faces.size(); i+=3){
            if(base_model_points.find(fur_verts[faces[i+0]]) == base_model_points.end() ||
               base_model_points.find(fur_verts[faces[i+1]]) == base_model_points.end() ||
               base_model_points.find(fur_verts[faces[i+2]]) == base_model_points.end())
            {
                ++new_tris;
                fur_model.faces.push_back(faces[i+0]);
                fur_model.faces.push_back(faces[i+1]);
                fur_model.faces.push_back(faces[i+2]);
            }
        }
     }

    // Populate fur model with vertices used by the fur tris, and remove unused
    // verts
    {
        std::vector<bool> included(fur_ref_model.vertices.size()/3, false);
        for(unsigned int face : fur_model.faces){
            included[face] = true;
        }

        int remap_index = 0;
        int vert_index;
        std::vector<int> remapped(fur_ref_model.vertices.size()/3);
        for(int i=0, len=fur_ref_model.vertices.size()/3; i<len; ++i){
            if(included[i]){
                remapped[i] = remap_index++;
                vert_index = i*3;
                fur_model.vertices.push_back(fur_ref_model.vertices[vert_index+0] - base_model.old_center[0]);
                fur_model.vertices.push_back(fur_ref_model.vertices[vert_index+1] - base_model.old_center[1]);
                fur_model.vertices.push_back(fur_ref_model.vertices[vert_index+2] - base_model.old_center[2]);
            }
        }
        for(unsigned int & face : fur_model.faces){
            face = remapped[face];
        }
    }
   
    // Get vec3 vector of fur vertices
    std::vector<vec3> fur_verts;
    fur_verts.reserve(fur_model.vertices.size()/3);
    for(unsigned i=0; i<fur_model.vertices.size(); i+=3){
        fur_verts.push_back(vec3(fur_model.vertices[i+0],
                                 fur_model.vertices[i+1],
                                 fur_model.vertices[i+2]));
    }

    // Collapse duplicate vertices
    {
        std::vector<int> vert_dup(fur_model.vertices.size()/3,-1);
        std::map<vec3, std::vector<int> > fur_model_points;
        for(unsigned i=0; i<fur_model.vertices.size(); i+=3){
            fur_model_points[vec3(fur_model.vertices[i+0],
                fur_model.vertices[i+1],
                fur_model.vertices[i+2])].push_back(i/3);
        } 
        for(int i=0, len=fur_model.vertices.size()/3; i<len; ++i){
            vert_dup[i] = fur_model_points[fur_verts[i]].front();
        }
        for(unsigned int & face : fur_model.faces){
            face = vert_dup[face];
        } 
    }

    // Get a vector of the base vertices at each fur vertex
    std::vector<std::vector<int>* > attachments(fur_model.vertices.size()/3);
    for(unsigned i=0; i<fur_verts.size(); ++i){
        std::map<vec3, std::vector<int> >::iterator iter = 
            base_model_points.find(fur_verts[i]);
        if(iter != base_model_points.end()){
            attachments[i] = &(iter->second);
        }
    }

    // Get a set of the fur vertices connected to each fur vertex
    std::vector<std::set<int> > connections(fur_model.vertices.size()/3);
    for(unsigned i=0; i<fur_model.faces.size(); i+=3){
        for(unsigned j=0; j<3; ++j){
            connections[fur_model.faces[i+j]].insert(fur_model.faces[i+(1+j)%3]);
            connections[fur_model.faces[i+j]].insert(fur_model.faces[i+(2+j)%3]);
        }
    }

    // Get a set of the base vertices connected to each base vertex
    std::vector<std::set<int> > base_connections(base_model.vertices.size()/3);
    for(unsigned i=0; i<base_model.faces.size(); i+=3){
        for(unsigned j=0; j<3; ++j){
            base_connections[base_model.faces[i+j]].insert(base_model.faces[i+(1+j)%3]);
            base_connections[base_model.faces[i+j]].insert(base_model.faces[i+(2+j)%3]);
        }
    }

    // Map fur edges to their equivalent base edges
    typedef std::map< std::pair<int, int>, std::pair<int, int> > PairMap;
    PairMap base_pairs;
    {
        bool found_edge;
        const std::vector<int> *attach_ptr[2];
        std::vector<int>::const_iterator attach_iter[2];
        std::set<int>::const_iterator connect_iter;
        std::pair<int, int> edge;
        for(int i=0, len=fur_model.faces.size()/3; i<len; ++i){
            edge.first = -1;
            edge.second = -1;
            for(unsigned j=0; j<3; ++j){
                if(attachments[fur_model.faces[i*3+j]]){
                    if(edge.first == -1){
                        edge.first = fur_model.faces[i*3+j];
                    } else {
                        edge.second = fur_model.faces[i*3+j];
                    }
                }
            }
            if(edge.first == -1 || edge.second == -1){
                continue;
            }
            if(edge.first > edge.second){
                std::swap(edge.first, edge.second);
            }
            found_edge = false;
            attach_ptr[0] = attachments[edge.first];
            attach_ptr[1] = attachments[edge.second];
            for(attach_iter[0] = attach_ptr[0]->begin(); 
                attach_iter[0] != attach_ptr[0]->end(); ++attach_iter[0])
            {
                const std::set<int>* connect_ptr = 
                    &base_connections[*attach_iter[0]];
                for(connect_iter = connect_ptr->begin(); 
                    connect_iter != connect_ptr->end(); ++connect_iter)
                {
                    for(attach_iter[1] = attach_ptr[1]->begin(); 
                        attach_iter[1] != attach_ptr[1]->end(); ++attach_iter[1])
                    {
                        if((*connect_iter) == (*attach_iter[1])){
                            base_pairs[edge] = std::pair<int,int>((*attach_iter[0]), (*attach_iter[1]));
                            found_edge = true;
                            break;
                        }
                    }
                    if(found_edge){
                        break;
                    }
                }
            }
        }
    }

    std::vector<int> face_attach(fur_model.faces.size());
    {
        // Get vector specifying which fur verts are attached to base mesh
        std::vector<bool> in_base_pair(fur_model.vertices.size()/3, false);
        {
            PairMap::const_iterator iter;
            for(iter = base_pairs.begin(); iter != base_pairs.end(); ++iter){
                in_base_pair[iter->first.first] = true;
                in_base_pair[iter->first.second] = true;
            }
        }

        // Get map of the neighbors for each triangle
        // Get map of the faces that share each edge
        std::map< int, std::set<int> > face_neighbors;
        std::map< std::pair<int, int>, std::set<int> > edge_tris;
        {
            std::pair<int, int> edge;
            for(int i=0, len=fur_model.faces.size()/3; i<len; ++i){
                for(unsigned j=0; j<3; ++j){
                    edge.first = fur_model.faces[i*3+j];
                    edge.second = fur_model.faces[i*3+(j+1)%3];
                    if(edge.first > edge.second){
                        std::swap(edge.first, edge.second);
                    }
                    edge_tris[edge].insert(i);
                }
            }

            std::map< std::pair<int, int>, std::set<int> >::const_iterator edge_iter;
            std::set<int>::const_iterator set_iter;
            edge_iter = edge_tris.begin();
            for(;edge_iter != edge_tris.end(); ++edge_iter){
                set_iter = edge_iter->second.begin();
                for(;set_iter != edge_iter->second.end(); ++set_iter){
                    face_neighbors[(*set_iter)].insert(edge_iter->second.begin(), edge_iter->second.end());
                }
            }

            for(int i=0, len=fur_model.faces.size()/3; i<len; ++i){
                face_neighbors[i].erase(i);
            }
        }

        // Get map of extruded edges and their base-attached equivalents
        PairMap edge_pairs;
        {
            int base_tri;
            float closest_dist;
            float dist;
            std::pair<int, int> edge;
            std::set<int>::const_iterator set_iter;
            PairMap::const_iterator base_pair_iter = base_pairs.begin();
            for(;base_pair_iter != base_pairs.end(); ++base_pair_iter){
                closest_dist = -1.0f;
                base_tri = *(edge_tris[base_pair_iter->first].begin());
                const std::set<int>& neighbors = face_neighbors[base_tri];
                for(set_iter = neighbors.begin(); 
                    set_iter != neighbors.end(); ++set_iter)
                {
                    for(unsigned j=0; j<3; ++j){
                        edge.first = fur_model.faces[(*set_iter)*3+j];
                        edge.second = fur_model.faces[(*set_iter)*3+(j+1)%3];
                        if(in_base_pair[edge.first] || in_base_pair[edge.second]){
                            continue;
                        }
                        if(edge.first > edge.second){
                            std::swap(edge.first, edge.second);
                        }
                        dist = distance_squared(
                            fur_verts[edge.first] + fur_verts[edge.second],
                            fur_verts[base_pair_iter->first.first] + 
                            fur_verts[base_pair_iter->first.second]);
                        if(closest_dist == -1.0f || dist < closest_dist){
                            closest_dist = dist;
                        } else {
                            continue;
                        }
                        edge_pairs[base_pair_iter->first] = edge;
                    }
                }
            }
        }
        
        {
            PairMap::const_iterator edge_pair_iter = edge_pairs.begin();
            std::map<int, int> vert_attach;
            std::map<int, int> edge_vert_attach;
            for(;edge_pair_iter != edge_pairs.end(); ++edge_pair_iter){
                vert_attach.clear();
                const std::pair<int,int>& edge_pair = edge_pair_iter->second;
                const std::pair<int,int>& base_pair = edge_pair_iter->first;
                vert_attach[base_pair.first] = base_pairs[base_pair].first;
                vert_attach[base_pair.second] = base_pairs[base_pair].second;
                int edge_tri = *(edge_tris[edge_pair].begin());
                int base_tri = *(edge_tris[base_pair].begin());
                int edge_vert_in_base = 0;
                int base_vert_in_edge = 0;
                for(unsigned i=0; i<3; ++i){
                    if(in_base_pair[fur_model.faces[edge_tri*3+i]]){
                        edge_vert_in_base = fur_model.faces[edge_tri*3+i];
                    }
                    if(!in_base_pair[fur_model.faces[base_tri*3+i]]){
                        base_vert_in_edge = fur_model.faces[base_tri*3+i];
                    }
                }
                if((edge_vert_in_base == base_pair.first &&
                    base_vert_in_edge == edge_pair.first) ||
                   (edge_vert_in_base == base_pair.second &&
                    base_vert_in_edge == edge_pair.second))
                {
                    vert_attach[edge_pair.first] = vert_attach[base_pair.second];
                    vert_attach[edge_pair.second] = vert_attach[base_pair.first];
                    edge_vert_attach[edge_pair.first] = base_pair.second;
                    edge_vert_attach[edge_pair.second] = base_pair.first;
                } else {
                    vert_attach[edge_pair.first] = vert_attach[base_pair.first];
                    vert_attach[edge_pair.second] = vert_attach[base_pair.second];
                    edge_vert_attach[edge_pair.first] = base_pair.first;
                    edge_vert_attach[edge_pair.second] = base_pair.second;
                }
                for(unsigned i=0; i<3; ++i){
                    face_attach[edge_tri*3+i] = vert_attach[fur_model.faces[edge_tri*3+i]];
                    face_attach[base_tri*3+i] = vert_attach[fur_model.faces[base_tri*3+i]];
                }
            }

            fur_model.tex_coords2.resize(fur_model.vertices.size()/3*2,0.0f);
            {
                PairMap::const_iterator edge_pair_iter = edge_pairs.begin();
                for(;edge_pair_iter != edge_pairs.end(); ++edge_pair_iter){
                    const std::pair<int,int>& edge_pair = edge_pair_iter->second;
                    fur_model.tex_coords2[edge_pair.first*2+1] = 1.0f;
                    fur_model.tex_coords2[edge_pair.second*2+1] = 1.0f;
                }
            }
            {
                float tex_val;
                static const float tex_scale = 20.0f;
                int num_base_connections;
                int starting_point, last_point, point, next_point;
                std::set<int>::const_iterator set_iter;
                std::vector<bool> fur_tex_set(fur_model.vertices.size()/3, false);
                for(int i=0, len=fur_model.vertices.size()/3; i<len; ++i){
                    if(!in_base_pair[i] || fur_tex_set[i]){
                        continue;
                    }
                    starting_point = i;
                    point = starting_point;
                    next_point = starting_point;
                    num_base_connections = 2;
                    do {
                        last_point = point;
                        point = next_point;
                        num_base_connections = 0;
                        set_iter = connections[point].begin();
                        for(;set_iter != connections[point].end(); ++set_iter){
                            if(in_base_pair[(*set_iter)]){
                                if((*set_iter) != last_point){
                                    next_point = (*set_iter);
                                }
                                ++num_base_connections;
                            }
                        }
                    } while(starting_point != next_point && 
                            num_base_connections == 2);

                    tex_val = 0.0f;
                    next_point = point;
                    do {
                        tex_val += distance(fur_verts[point], fur_verts[next_point]);
                        point = next_point;
                        fur_model.tex_coords2[point*2+0] = tex_val * tex_scale;
                        fur_tex_set[point] = true;
                        set_iter = connections[point].begin();
                        for(;set_iter != connections[point].end(); ++set_iter){
                            if(in_base_pair[(*set_iter)] && 
                               !fur_tex_set[(*set_iter)])
                            {
                                next_point = (*set_iter);
                            }
                        }
                    } while(fur_tex_set[next_point] == false);
                }
            }
            {
                std::map<int, int>::iterator iter = edge_vert_attach.begin();
                for(;iter != edge_vert_attach.end(); ++iter){
                    fur_model.tex_coords2[iter->first*2+0] = 
                        fur_model.tex_coords2[iter->second*2+0];
                }
            }
        }
    }


    int next_vert = fur_model.vertices.size()/3;
    std::vector< std::map<int, int> > vert_attach(fur_model.vertices.size()/3);
    std::vector< int > attach_base(fur_model.vertices.size()/3);
    std::pair<int, int> edge;
    int attach_id, v_id;
    for(int i=0, len=fur_model.faces.size(); i<len; i+=3){
        if(face_attach[i+0] == 0 &&
           face_attach[i+1] == 0 &&
           face_attach[i+2] == 0)
        {
            fur_model.faces[i+0] = 0;
            fur_model.faces[i+1] = 0;
            fur_model.faces[i+2] = 0;
            continue;
        }
        for(unsigned j=0; j<3; ++j){
            v_id = fur_model.faces[i+j];
            attach_id = face_attach[i+j];
            if(vert_attach[v_id].find(attach_id) == vert_attach[v_id].end()){
                if(vert_attach[v_id].empty()){
                    vert_attach[v_id][attach_id] = v_id;
                    attach_base[v_id] = attach_id;
                } else {;
                    attach_base.push_back(attach_id);
                    fur_model.vertices.push_back(fur_model.vertices[v_id*3+0]);
                    fur_model.vertices.push_back(fur_model.vertices[v_id*3+1]);
                    fur_model.vertices.push_back(fur_model.vertices[v_id*3+2]);
                    fur_model.tex_coords2.push_back(fur_model.tex_coords2[v_id*2+0]);
                    fur_model.tex_coords2.push_back(fur_model.tex_coords2[v_id*2+1]);
                    fur_model.faces[i+j] = next_vert;
                    vert_attach[v_id][attach_id] = next_vert++;
                }
            } else {
                fur_model.faces[i+j] = vert_attach[v_id][attach_id];
            }
        }
    }

    int fur_model_num_vertices = attach_base.size();
    fur_model.tex_coords.resize(fur_model_num_vertices*2);
    fur_model.bone_weights.resize(fur_model_num_vertices);
    fur_model.bone_ids.resize(fur_model_num_vertices);
    fur_base_vertex_ids.resize(fur_model_num_vertices);
    for(int i=0; i<fur_model_num_vertices; ++i){
        const int &base_id = attach_base[i];
        fur_base_vertex_ids[i] = base_id;
        fur_model.tex_coords[i*2+0] = base_model.tex_coords[base_id*2+0];
        fur_model.tex_coords[i*2+1] = base_model.tex_coords[base_id*2+1];
        fur_model.bone_weights[i] = base_model.bone_weights[base_id];
        fur_model.bone_ids[i] = base_model.bone_ids[base_id];
    }

    fur_model.transform_vec.resize(4);
    fur_model.transform_vec[0].resize(fur_model_num_vertices);
    fur_model.transform_vec[1].resize(fur_model_num_vertices);
    fur_model.transform_vec[2].resize(fur_model_num_vertices);
    fur_model.transform_vec[3].resize(fur_model_num_vertices);
    fur_model.tex_transform_vec.resize(fur_model_num_vertices, vec2(0.0f));
    fur_model.morph_transform_vec.resize(fur_model_num_vertices, vec3(0.0f));
}

void RiggedObject::MoveRagdollPart( const std::string &label, const vec3 &position, float strength_mult ) const {
    Skeleton::SimpleIKBoneMap::const_iterator iter = 
        skeleton_.simple_ik_bones.find(label);
    if( iter == skeleton_.simple_ik_bones.end())
    {
        return;
    }
    const SimpleIKBone &sib = iter->second;
    BulletObject* bo = skeleton_.physics_bones[sib.bone_id].bullet_object;
    vec3 curr_pos = bo->GetPosition();
    if(skeleton_.fixed_obj[sib.bone_id]){
        bo = skeleton_.physics_bones[skeleton_.parents[sib.bone_id]].bullet_object;
    }
    if(skeleton_.fixed_obj[skeleton_.parents[sib.bone_id]]){
        bo = skeleton_.physics_bones[skeleton_.parents[skeleton_.parents[sib.bone_id]]].bullet_object;
    }
    scenegraph_->bullet_world_->CreateTempDragConstraint(bo, curr_pos, position);
}

void RiggedObject::FixedRagdollPart( int boneID, const vec3 &position) const {
    BulletObject* bo = skeleton_.physics_bones[boneID].bullet_object;
    if(skeleton_.fixed_obj[boneID]){
        bo = skeleton_.physics_bones[skeleton_.parents[boneID]].bullet_object;
    }
    if(skeleton_.fixed_obj[skeleton_.parents[boneID]]){
        bo = skeleton_.physics_bones[skeleton_.parents[skeleton_.parents[boneID]]].bullet_object;
    }
	vec3 curr_pos = bo->GetPosition();
	scenegraph_->bullet_world_->CreateBoneConstraint(bo, curr_pos, position);
}

void RiggedObject::SpikeRagdollPart( int boneID, const vec3 &start, const vec3 &end, const vec3 &pos) const {
	BulletObject* bo = skeleton_.physics_bones[boneID].bullet_object;
	if(skeleton_.fixed_obj[boneID]){
		bo = skeleton_.physics_bones[skeleton_.parents[boneID]].bullet_object;
	}
	if(skeleton_.parents[boneID] != -1 && skeleton_.fixed_obj[skeleton_.parents[boneID]]){
		bo = skeleton_.physics_bones[skeleton_.parents[skeleton_.parents[boneID]]].bullet_object;
	}
	scenegraph_->bullet_world_->CreateSpikeConstraint(bo, start, end, pos);
}

void RiggedObject::ClearBoneConstraints(){
	scenegraph_->bullet_world_->ClearBoneConstraints();
}

void RiggedObject::ApplyPalette( const OGPalette& _palette ) {
    unsigned num_iters = min(palette_colors.size(), _palette.size());
    for(unsigned i=0; i<num_iters; ++i){
        palette_colors_srgb[_palette[i].channel] = _palette[i].color;
        palette_colors[_palette[i].channel] = _palette[i].color;
        palette_colors[_palette[i].channel][0] = pow(palette_colors[_palette[i].channel][0], 1.0f/2.2f);
        palette_colors[_palette[i].channel][1] = pow(palette_colors[_palette[i].channel][1], 1.0f/2.2f);
        palette_colors[_palette[i].channel][2] = pow(palette_colors[_palette[i].channel][2], 1.0f/2.2f);
    }
}

std::vector<vec3> * RiggedObject::GetPaletteColors() {
    return &palette_colors_srgb;
}

void RiggedObject::StickItem( int id, const vec3 & start, const vec3 & end, const mat4 &transform ) {
    int bone = skeleton_.CheckRayCollisionBone(start, end);
    if(bone != -1){
        Object* obj = scenegraph_->GetObjectFromID(id);
        if(!obj || obj->GetType() != _item_object){
            FatalError("Error", "There is no item object %d", id);
        }
        ItemObject* io = (ItemObject*)obj;
        vec3 old_vel = io->GetLinearVelocity();
        io->SetPhysicsTransform(transform);
        io->SetLinearVelocity(old_vel);
        mat4 rel_mat = invert(skeleton_.physics_bones[bone].bullet_object->GetTransform()) * transform;
        stuck_items.Add(io, bone, rel_mat, char_id, scenegraph_->bullet_world_, skeleton_);
		if(!animated){
			stuck_items.items.back().item.ActivatePhysics();
            stuck_items.items.back().item.AddConstraint(skeleton_.physics_bones[bone].bullet_object);
        }
    }
}

extern std::stack<ASContext*> active_context_stack;

void RiggedObject::UnStickItem( int id ) {
    stuck_items.Remove(id);
}

std::vector<ItemRef> RiggedObject::GetWieldedItemRefs() const {
    int num_wielded = 0;
    for(const auto & iter : attached_items.items)
    {
        const ItemObjectScriptReader& item = iter.item;
        if(item->GetID() == primary_weapon_id){
            ++num_wielded;
        }
    }
    std::vector<ItemRef> wielded_item_refs(num_wielded);
    num_wielded = 0;
    for(const auto & iter : attached_items.items)
    {
        const ItemObjectScriptReader& item = iter.item;
        if(item->GetID() == primary_weapon_id){
            wielded_item_refs[num_wielded++] = item->item_ref();
        }
    }
    return wielded_item_refs;
}

void RiggedObject::SetCharacterScriptGetter( CharacterScriptGetter &csg ) {
    character_script_getter = &csg;
}

void RiggedObject::SetPrimaryWeaponID( int id ) {
    primary_weapon_id = id;
    character_script_getter->ItemsChanged(GetWieldedItemRefs());
}

void DrawAvailableAttachmentSlot(const ItemRef& item_ref, AttachmentType type, const Skeleton &skeleton, bool mirrored, std::list<AttachmentSlot> *list){
    int bone_id;
    mat4 rel_mat = LoadAttachPose(item_ref->GetAttachAnimPath(type), item_ref->GetIKAttach(type), skeleton, &bone_id, mirrored, true, 0);
    mat4 bone_mat = skeleton.physics_bones[bone_id].bullet_object->GetTransform();
    mat4 world_mat = bone_mat * rel_mat;

    list->resize(list->size()+1);
    AttachmentSlot &slot = list->back();
    slot.pos = world_mat.GetTranslationPart();
    slot.type = type;
    slot.mirrored = mirrored;
}

void DrawAvailableAttachment(const AttachmentRef attachment_ref, const Skeleton &skeleton, bool mirrored, AttachmentSlotList *list){
    int bone_id;
    mat4 rel_mat = LoadAttachPose(attachment_ref->anim, attachment_ref->ik_chain_label, skeleton, &bone_id, mirrored, false, attachment_ref->ik_chain_bone);
    mat4 bone_mat = skeleton.physics_bones[bone_id].bullet_object->GetTransform();
    mat4 world_mat = bone_mat * rel_mat;

    list->resize(list->size()+1);
    AttachmentSlot &slot = list->back();
    slot.pos = world_mat.GetTranslationPart();
    slot.type = _at_attachment;
    slot.attachment_ref = attachment_ref;
    slot.mirrored = mirrored;
}

void DrawAvailableAttachmentSlots(const ItemRef& item_ref, AttachmentType type, const Skeleton &skeleton, AttachmentSlotList *list){
    DrawAvailableAttachmentSlot(item_ref, type, skeleton, false, list);
    DrawAvailableAttachmentSlot(item_ref, type, skeleton, true, list);
}

void RiggedObject::AvailableItemSlots( const ItemRef& item_ref, AttachmentSlotList *list ) const {
    if(item_ref->HasAttachment(_at_grip)){
        DrawAvailableAttachmentSlots(item_ref, _at_grip, skeleton_, list);
    }
    if(item_ref->HasAttachment(_at_sheathe)){
        DrawAvailableAttachmentSlots(item_ref, _at_sheathe, skeleton_, list);
    }
    const std::list<std::string> &attachment_list = item_ref->GetAttachmentList();
    for(const auto & iter : attachment_list)
    {
        //AttachmentRef attachment_ref = Attachments::Instance()->ReturnRef((*iter));
        AttachmentRef attachment_ref = Engine::Instance()->GetAssetManager()->LoadSync<Attachment>(iter);
        DrawAvailableAttachment(attachment_ref, skeleton_, false, list);
        if(attachment_ref->mirror_allowed){
            DrawAvailableAttachment(attachment_ref, skeleton_, true, list);
        }
    }
}

void RiggedObject::AttachItemToSlot( ItemObject* item_object, AttachmentType type, bool mirrored, const AttachmentRef* attachment_ref ) {
    // Detach item if it is already attached
    DetachItem(item_object);
    
    // Set up item attachment
    attached_items.items.resize(attached_items.items.size()+1);
    AttachedItem &stuck_item = attached_items.items.back();
    ItemObjectScriptReader &item = stuck_item.item;
    item.char_id = char_id;
    item.holding = true;
    if(attachment_ref){
        item.attachment_ref = *attachment_ref;
    }
    item.SetInvalidateCallback(&RiggedObject::InvalidatedItemCallback, this);
    item.AttachToItemObject(item_object);
    if(type == _at_grip){
        item->SetState(ItemObject::kWielded);
    } else if(type == _at_sheathe){
        item->SetState(ItemObject::kSheathed);
    }
    if(attachment_ref && attachment_ref->valid()){
        SetItemAttachment(stuck_item, *attachment_ref, mirrored);
    } else {
        SetWeaponOffset(stuck_item, type, mirrored);
    }

    if(animated){
        UpdateAttachedItem(stuck_item, skeleton_, model_char_scale, weapon_offset, weap_anim_info_map, weapon_offset_retarget);
    } else {
        UpdateAttachedItemRagdoll(stuck_item, skeleton_, weapon_offset);
    }
}

Skeleton & RiggedObject::skeleton() {
    return skeleton_;
}

void RiggedObject::SetCharScale(float val) {
    char_scale = val * character_script_getter->GetDefaultScale();
    model_char_scale = char_scale / character_script_getter->GetModelScale();
}

float RiggedObject::GetCharScale() {
    return char_scale;
}

float RiggedObject::GetRelativeCharScale() {
    return char_scale / character_script_getter->GetDefaultScale();
}

void RiggedObject::UpdateGPUSkinning() {
    GPU_skinning = Graphics::Instance()->config_.gpu_skinning();
    for(int i=0; i<kLodLevels; ++i){
        transform_vec_vbo0[i]->Dispose();
        transform_vec_vbo1[i]->Dispose();
    }
    fur_transform_vec_vbo0.Dispose();
    fur_transform_vec_vbo1.Dispose();
}

void RiggedObject::GetShaderNames(std::map<std::string, int>& shaders) {
    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    strcpy(shader_str[0], shader);
    if(GPU_skinning){
        FormatString(shader_str[1], kShaderStrSize, "%s #GPU_SKINNING", shader_str[0]);
        std::swap(shader_str[0], shader_str[1]);
    }
    shaders[shader_str[0]] = SceneGraph::kPreloadTypeAll;
    shaders[water_cube] = 0;
    shaders[water_cube_expand] = 0;

    blood_surface.GetShaderNames(shaders);
}

void AttachedItems::Add( ItemObject* io, int bone, mat4 rel_mat, int char_id, BulletWorld *bw, Skeleton& skeleton ) {
    items.resize(items.size()+1);
    AttachedItem& item = items.back();
    item.bone_id = bone;
    item.rel_mat = rel_mat;

    ItemObjectScriptReader& iosr = item.item;
    iosr.stuck = true;
    iosr.char_id = char_id;
    iosr.SetInvalidateCallback(&AttachedItems::InvalidatedItemCallback, this);
    iosr.AttachToItemObject(io);
}

void AttachedItems::InvalidatedItem(ItemObjectScriptReader *invalidated) {
    for(std::list<AttachedItem>::iterator iter = items.begin(); iter != items.end();) {
        if(&(iter->item) == invalidated){
            iter->item->ActivatePhysics();
            iter = items.erase(iter);
        } else {
            ++iter;
        }
    }
}

void AttachedItems::InvalidatedItemCallback(ItemObjectScriptReader *invalidated, void* this_ptr) {
    ((AttachedItems*)this_ptr)->InvalidatedItem(invalidated);
}

void AttachedItems::Remove( int id ) {
    for(std::list<AttachedItem>::iterator iter = items.begin(); iter != items.end();) {
        if(iter->item->GetID() == id){
            iter->item->ActivatePhysics();
            iter = items.erase(iter);
        } else {
            ++iter;
        }
    }
}

void AttachedItems::RemoveAll() {
    for(auto & item : items) {
        item.item->ActivatePhysics();
    }
    items.clear();
}

vec3 RiggedTransformedVertexGetter::GetTransformedVertex(int val) {
    return rigged_object->GetTransformedVertex(val);
}
