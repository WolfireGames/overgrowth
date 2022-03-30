//-----------------------------------------------------------------------------
//           Name: animationclient.cpp
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
#include "animationclient.h"

#include <Graphics/retargetfile.h>
#include <Graphics/skeleton.h>

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <Internal/timer.h>
#include <Internal/profiler.h>

#include <Asset/Asset/syncedanimation.h>
#include <Scripting/angelscript/ascontext.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <cassert>

extern AnimationConfig animation_config;
extern Timer game_timer;

void AnimationClient::SwapAnimation( const std::string &path, char flags ) {
    current_anim.clear();
    reader.SetCallbackString("");
    SetAnimation(path, 100.0f, flags | _ANM_SWAP);
}

void AnimationClient::ApplyAnimationFlags(AnimationReader &reader, AnimationAssetRef &new_ref, char _flags) {
    flags = _flags;
    bool swap = (flags & _ANM_SWAP)!=0;
    bool mobile = (flags & _ANM_MOBILE)!=0;
    bool super_mobile = (flags & _ANM_SUPER_MOBILE)!=0;
    bool mirrored = (flags & _ANM_MIRRORED)!=0;
    float last_rotation;
    float old_time;
    if(swap){
        old_time = reader.GetAbsoluteTime();
        mobile = reader.GetMobile();
        super_mobile = reader.GetSuperMobile();
        mirrored = reader.GetMirrored();
        last_rotation = reader.GetLastRotation();
    }
    reader.AttachTo(new_ref);
    if(swap){
        reader.SetAbsoluteTime(old_time);
        reader.SetLastRotation(last_rotation);
    } else {
        reader.SetSpeedMult(1.0f);
    }
    reader.SetMobile(mobile);
    reader.SetSuperMobile(super_mobile);
    
    std::vector<mat4> initial_mats(skeleton->physics_bones.size());
    for(unsigned i=0; i<skeleton->physics_bones.size(); ++i){
        initial_mats[i] = skeleton->physics_bones[i].initial_rotation;
    }
    
    if(mirrored){
        reader.SetMirrored(true);
    } else {
        reader.SetMirrored(false);
    }

    reader.ClearBlendAnim();
}

void AnimationClient::SetAnimation( const std::string &path, 
                                    float fade_speed,
                                    char flags)
{
    if(flags & _ANM_FROM_START){
        current_anim.clear();
    }
    if(current_anim == path){
        return;
    }

	AnimationAssetRef new_ref;
	if(animation_config.kForceIdleAnim){
		new_ref = ReturnAnimationAssetRef("Data/Animations/r_actionidle.xml");
	} else {
        Path new_path = FindFilePath(path);
        if(new_path.isValid()){
            new_ref = ReturnAnimationAssetRef(path);
            if(new_ref.valid() == false ){
                new_ref = ReturnAnimationAssetRef("Data/Animations/r_actionidle.xml");
            }
        }
	}

    if(new_ref.valid()){
        if(reader.valid()){
            fade_out.AddFadingAnimation(reader, blendmap, fade_speed);
        }
        ApplyAnimationFlags(reader, new_ref, flags);
        current_anim = path;
    }
}

std::queue<AnimationEvent> AnimationClient::GetActiveEvents() {
    std::queue<AnimationEvent> total_events;
    {
        std::queue<AnimationEvent> &events = reader.GetActiveEvents();
        while(!events.empty()){
            total_events.push(events.front());
            events.pop();
        }
    }
    for(unsigned i=0; i<layers.size(); ++i){
        std::queue<AnimationEvent> &events = layers[i].reader.GetActiveEvents();
        while(!events.empty()){
            total_events.push(events.front());
            events.pop();
        }
    }
    return total_events;
}

void AnimationClient::SetSpeedMult(float speed_mult) {
    if(reader.valid()){
        reader.SetSpeedMult(speed_mult);
    }
}

void AnimationClient::Update(float timestep) {
    if(!reader.valid()){
        return;
    }
    if(reader.IncrementTime(blendmap, timestep)){
        const std::string& callback = reader.GetCallbackString();
        if(!callback.empty()){
            as_context->CallScriptFunction(callback,NULL,NULL,true);
            reader.SetCallbackString("");
        }
    }
    fade_out.Update(blendmap, as_context, timestep);
    for(unsigned i=0; i<layers.size(); ++i){
        layers[i].fade_out.Update(blendmap, as_context, timestep);
    }
    UpdateLayers(timestep);
}

const float _three_halves_pi = 4.7124f;
const float _pi = 3.1416f;
const float _overshoot_amount = 0.3f;

void AnimationClient::GetMatrices( AnimOutput &anim_output, const std::vector<int> *parents) {
    if(!reader.valid()){
        return;
    }
    AnimInput ang_anim_input(blendmap, parents);
    ang_anim_input.retarget_new = retarget_new;
    {
        PROFILER_ZONE(g_profiler_ctx, "reader.GetMatrices()");
        reader.GetMatrices(ang_anim_output, ang_anim_input);
    }

    if(special_rotation != 0.0f){
        mat4 special_rotation_mat;
        special_rotation_mat.SetRotationY(special_rotation);
        ang_anim_output.delta_offset = special_rotation_mat * ang_anim_output.delta_offset; 
        special_rotation = 0.0f;
    }

    ang_anim_output.delta_offset += invert(rotation_matrix) * delta_offset_overflow;
    ang_anim_output.delta_rotation += delta_rotation_overflow;
    if(ang_anim_output.delta_rotation > (float)PI){
        ang_anim_output.delta_rotation -= (float)PI * 2.0f;
    }
    if(ang_anim_output.delta_rotation < (float)-PI){
        ang_anim_output.delta_rotation += (float)PI * 2.0f;
    }
    float opac = fade_out.GetOpac();
    delta_offset_overflow = rotation_matrix * (ang_anim_output.delta_offset * (1.0f-opac));
    delta_rotation_overflow = ang_anim_output.delta_rotation * (1.0f-opac);
    ang_anim_output.delta_offset *= opac;
    ang_anim_output.delta_rotation *= opac;
    
    vec3 old_delta_offset = ang_anim_output.delta_offset;
    ang_anim_output.delta_offset = vec3(0.0f);
    
    float old_delta_rotation = ang_anim_output.delta_rotation;
    ang_anim_output.delta_rotation = 0.0f;

	if(!animation_config.kDisableAnimationTransition){
        // Mix with angular matrices from fading animations
        fade_out.ApplyAngular(ang_anim_input, ang_anim_output, temp_ang_anim_output, retarget_new, parents);
	}
	if(!animation_config.kDisableAnimationLayers){
		for(int i=(int)layers.size()-1; i>=0; i--){
			AnimInput old_ang_anim_input(blendmap, parents);
			old_ang_anim_input.retarget_new = retarget_new;
			layers[i].reader.GetMatrices(temp_ang_anim_output, old_ang_anim_input);
			if(layers[i].fade_out.fade_out.size()){
				layers[i].fade_out.ApplyAngular(old_ang_anim_input, temp_ang_anim_output, temp_ang_anim_output2, retarget_new, parents);
			}
			ang_anim_output = add_mix(ang_anim_output,temp_ang_anim_output,layers[i].opac);
		}    
	}
    ang_anim_output.delta_offset += old_delta_offset;
    ang_anim_output.delta_rotation += old_delta_rotation;

    std::vector<BoneTransform> matrices(ang_anim_output.matrices.size());
    {
        PROFILER_ZONE(g_profiler_ctx, "Apply Parent Rotations");
        // Blend between angular and linear matrices based on IK weights
        if(parents){
            std::vector<BoneTransform> temp_matrices(matrices.size());
            for(unsigned j=0; j<matrices.size(); j++){
                temp_matrices[j] = ApplyParentRotations(ang_anim_output.matrices, j, *parents);
            }
            matrices = temp_matrices;
        } 
    }

    std::vector<BoneTransform> weapon_matrices = ang_anim_output.weapon_matrices;

	// Apply character rotation modifier
    std::map<int, WeapAnimInfo> &weap_anim_info_map = ang_anim_output.weap_anim_info_map;
    for(int i=0, len=ang_anim_output.ik_bones.size(); i<len; ++i){
        ang_anim_output.ik_bones[i].unmodified_transform = ang_anim_output.ik_bones[i].transform;
    }

    anim_output.unmodified_matrices = matrices;

    // Put together output
    anim_output.matrices = matrices;
    anim_output.weap_anim_info_map = weap_anim_info_map;
    anim_output.weapon_matrices = weapon_matrices;
    anim_output.weapon_weights = ang_anim_output.weapon_weights;
    anim_output.weapon_weight_weights = ang_anim_output.weapon_weight_weights;
    anim_output.weapon_relative_ids = ang_anim_output.weapon_relative_ids;
    anim_output.weapon_relative_weights = ang_anim_output.weapon_relative_weights;
    anim_output.physics_weights = ang_anim_output.physics_weights;
    anim_output.ik_bones = ang_anim_output.ik_bones;
    anim_output.shape_keys = ang_anim_output.shape_keys;
    anim_output.status_keys = ang_anim_output.status_keys;
    anim_output.center_offset = rotation_matrix * ang_anim_output.center_offset;
    anim_output.delta_offset = rotation_matrix * ang_anim_output.delta_offset;
    anim_output.rotation = ang_anim_output.rotation;
    anim_output.delta_rotation = ang_anim_output.delta_rotation;
}

vec3 AnimationClient::GetAvgTranslation(const std::vector<BoneTransform> &matrices ) {
    vec3 center(0.0f);
    float total_mass = 0.0f;
    for(unsigned j=0; j<matrices.size(); j++){
        center += matrices[j].origin*bone_masses[j];
        total_mass += bone_masses[j];
    }
    LOG_ASSERT(total_mass != 0.0f);
    center /= total_mass;
    return center;
}

void AnimationClient::SetBlendCoord( const std::string &label, float coord ) {
    blendmap[label] = coord;
}

void FadeCollection::Update(const BlendMap& blendmap, ASContext *as_context, float timestep) {
    std::vector<AnimationFadeOut>::iterator iter;
    for(iter = fade_out.begin(); iter != fade_out.end();){
        AnimationFadeOut &the_fade_out = (*iter);

        the_fade_out.reader.IncrementTime(blendmap, timestep);
        the_fade_out.opac -= timestep * 
                             the_fade_out.fade_speed;

        if(the_fade_out.opac <= 0.0f){
            const std::string& callback = the_fade_out.reader.GetCallbackString();
            if(!callback.empty()){
                as_context->CallScriptFunction(callback);
                the_fade_out.reader.SetCallbackString("");
            }
            iter = fade_out.erase(iter);
        } else {
            iter++;
        }
    }
}

void FadeCollection::ApplyAngular( AnimInput &ang_anim_input, 
                                   AnimOutput &ang_anim_output, 
                                   AnimOutput &temp_ang_anim_output,
                                   const std::string &retarget_new,
                                   const std::vector<int> *parents)
{
    for(int i=(int)fade_out.size()-1; i>=0; --i){
        AnimInput temp_ang_anim_input(fade_out[i].blendmap, parents);
        temp_ang_anim_input.retarget_new = retarget_new;
        fade_out[i].reader.GetMatrices(temp_ang_anim_output, temp_ang_anim_input);
        float temp_opac;
        if(fade_out[i].overshoot){
            temp_opac = sinf(fade_out[i].opac*_three_halves_pi+_pi)*fade_out[i].opac;
            if(temp_opac<0.0f){
                temp_opac *= _overshoot_amount;
            }
        } else {
            temp_opac = fade_out[i].opac;
        }
        ang_anim_output = mix(ang_anim_output,temp_ang_anim_output,temp_opac);
    }    
}

void FadeCollection::AddFadingAnimation( AnimationReader &reader, const BlendMap& blendmap, float fade_speed )
{
    fade_out.resize(fade_out.size()+1);
    fade_out.back().opac = 1.0f;
    fade_out.back().reader = reader;
    fade_out.back().blendmap = blendmap;
    if(fade_speed<0.0f){
        fade_out.back().fade_speed = fade_speed * -0.33f;
        fade_out.back().overshoot = true;
    } else {
        fade_out.back().fade_speed = fade_speed;
        fade_out.back().overshoot = false;
    }
}

void FadeCollection::clear()
{
    fade_out.clear();
}

float FadeCollection::GetOpac()
{
    float opac = 1.0f;
    for(unsigned i=0; i<fade_out.size(); ++i){
        opac *= (1.0f-fade_out[i].opac);
    }
    return opac;
}


void AnimationClient::UpdateLayers(float timestep) {
    bool deleted = false;
    std::vector<AnimationLayer>::iterator iter;
    for(iter = layers.begin(); iter != layers.end();){
        AnimationLayer &the_layer = (*iter);

        the_layer.reader.IncrementTime(blendmap, timestep);

        if(the_layer.reader.Finished() || the_layer.fading){
            the_layer.fade_opac -= timestep * the_layer.fade_speed;
        } else {
            the_layer.fade_opac += timestep * the_layer.fade_speed;
            the_layer.fade_opac = min(1.0f, the_layer.fade_opac);
        }
        the_layer.opac = the_layer.fade_opac * the_layer.opac_mult;
        if(the_layer.fade_opac <= 0.0f){
            ASArglist args;
            args.Add(the_layer.id);
            as_context->CallScriptFunction(as_funcs.layer_removed, &args);
            iter = layers.erase(iter);
            deleted = true;
        } else {
            iter++;
        }
    }
    if(deleted){
        /*printf("Layers after remove:\n");
        for(unsigned i=0; i<layers.size(); ++i){
            printf("%d %s\n", layers[i].id, layers[i].reader.GetPath().c_str());
        }*/
    }
}

void AnimationClient::SetASContext( ASContext * _as_context ) {
    as_context = _as_context;

    as_funcs.layer_removed = as_context->RegisterExpectedFunction("void LayerRemoved(int id)",true);

    as_context->LoadExpectedFunctions();
}

void AnimationClient::SetCallbackString( const std::string &_callback_string ) {
    reader.SetCallbackString(_callback_string);
}

void AnimationClient::SetRotationMatrix( const mat4 &matrix ) {
    rotation_matrix = matrix;
}

const mat4 &AnimationClient::GetRotationMatrix( ) {
    return rotation_matrix;
}

void AnimationClient::SetBoneMasses( const std::vector<float> &_bone_masses ) {
    bone_masses = _bone_masses;
}

void AnimationClient::Reset() {
    current_anim.clear();
    fade_out.clear();
    layers.clear();
    reader.clear();
    delta_offset_overflow = vec3(0.0f);
    delta_rotation_overflow = 0.0f;
    special_rotation = 0.0f;
}

void AnimationClient::SetSkeleton( Skeleton *_skeleton ) {
    skeleton = _skeleton;
}

void AnimationClient::SetRetargeting( const std::string &new_path) {
    retarget_new = new_path;
}

int AnimationClient::AddLayer( const std::string &path, float fade_speed /*= _default_fade_speed*/, char flags /*= NULL*/ ) {
    AnimationAssetRef new_ref = ReturnAnimationAssetRef(path);
    if(new_ref.valid()){
        layers.resize(layers.size()+1);
        layers.back().fade_opac = fade_speed*game_timer.timestep;
        layers.back().opac_mult = 1.0f;
        layers.back().opac = layers.back().fade_opac * layers.back().opac_mult;
        layers.back().fade_speed = fade_speed;
        layers.back().fading = false;
        bool conflict = true;
        int new_id = 0;
        while(conflict){
            conflict = false;    
            for(unsigned i=0; i<layers.size()-1; ++i){
                if(new_id == layers[i].id){
                    ++new_id;
                    conflict = true;
                }
            }
        }
        layers.back().id = new_id;
        ApplyAnimationFlags(layers.back().reader, new_ref, flags);
        return layers.back().id;
    }
    return -1;
}

void AnimationClient::RemoveLayer( int which, float fade_speed ) {
    int which_layer = -1;
    for(unsigned i=0; i<layers.size(); ++i){
        if(layers[i].id == which){
            which_layer = i;
        }
    }
    if(which_layer != -1){
        layers[which_layer].fade_speed = fade_speed;
        layers[which_layer].fading = true;
    }
}

void AnimationClient::SetLayerOpacity( int which, float opac ) {
    if( opac > 1.0f ) { 
        opac = 1.0f;
    }

    if( opac < 0.00001f ) { 
        opac = 0.00001f;
    }

    int which_layer = -1;
    for(unsigned i=0; i<layers.size(); ++i){
        if(layers[i].id == which){
            which_layer = i;
        }
    }
    if(which_layer != -1){
        layers[which_layer].opac_mult = opac;
    }
}

void AnimationClient::SetBlendAnimation( const std::string & path ) {
    LOGD << "Setting blend animation: " << path << std::endl;

    AnimationAssetRef new_ref = ReturnAnimationAssetRef(path);

    std::vector<mat4> initial_mats(skeleton->physics_bones.size());
    for(unsigned i=0; i<skeleton->physics_bones.size(); ++i){
        initial_mats[i] = skeleton->physics_bones[i].initial_rotation;
    }

    reader.SetBlendAnim(new_ref);
}

void AnimationClient::AddAnimationOffset( const vec3 & offset )
{
    delta_offset_overflow += offset;
}

void AnimationClient::AddAnimationRotOffset( float offset )
{
    delta_rotation_overflow += offset;
    special_rotation = offset;
}

void AnimationClient::ClearBlendAnimation()
{
    reader.ClearBlendAnim();
}

float AnimationClient::GetAnimationEventTime( const std::string & event_name ) const
{
    return reader.GetAnimationEventTime(event_name, blendmap);
}

AnimationClient::AnimationClient() {
    Reset();
}

float AnimationClient::GetTimeUntilEvent( const std::string &event ) {
    float time = reader.GetTimeUntilEvent(event);
    int num_layers = layers.size();
    for(int i=0; i<num_layers; ++i){
        float layer_time = layers[i].reader.GetTimeUntilEvent(event);
        if(layer_time != -1.0f){
            if(time == -1.0f || layer_time < time){
                time = layer_time;
            }
        }
    }
    return time;
}

void AnimationClient::SetAnimatedItemID( int index, int id )
{
    reader.SetAnimatedItemID(index, id);
}

void AnimationClient::SetLayerItemID( int layer_id, int index, int item_id )
{
    int which_layer = -1;
    for(unsigned i=0; i<layers.size(); ++i){
        if(layers[i].id == layer_id){
            which_layer = i;
        }
    }
    if(which_layer != -1){
        layers[which_layer].reader.SetAnimatedItemID(index, item_id);
    }
}

const std::string& AnimationClient::GetCurrAnim() const {
    return current_anim;
}

float AnimationClient::GetNormalizedAnimTime() const {
    return reader.GetTime();
}

float AnimationClient::GetAnimationSpeed() const {
    return reader.GetSpeedNormalized(blendmap);
}

void AnimationClient::RemoveAllLayers() {
    layers.clear();
}
