//-----------------------------------------------------------------------------
//           Name: animationreader.cpp
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
#include "animationreader.h"

#include <Internal/timer.h>
#include <Internal/error.h>

#include <Graphics/pxdebugdraw.h>
#include <Math/vec3math.h>
#include <Logging/logdata.h>

#include <algorithm>

void AnimationReader::ActivateEventsInRange(float start, float end, int active_id){
    while(next_event < events.size() &&    
          start <= events[next_event].time && 
          events[next_event].time < end)
    {
        LOGD << "Active id: " <<  active_id << std::endl;
        LOGD << "Activating event:" << events[next_event].anim_id << " " << events[next_event].event.event_string << std::endl;
        
        if((int)events[next_event].anim_id == active_id){
            active_events.push(events[next_event].event);
        }
        next_event++;
        if(next_event < events.size()){
            LOGD << "Next event: " << events[next_event].event.event_string << std::endl;
        }
    }

    if(next_event == events.size() && !events.empty()){
        next_event = 0;
        LOGD << "Next event: " << events[next_event].event.event_string << std::endl;
    }
}

std::queue<AnimationEvent>& AnimationReader::GetActiveEvents(){
    return active_events;
}
 
float AnimationReader::GetSpeedNormalized(const BlendMap &blendmap) const {
    const AnimationAsset &anim_asset = (*anim);
    return anim_asset.GetFrequency(blendmap) * speed_mult;
}

bool AnimationReader::IncrementTime( const BlendMap &blendmap, float timestep ) {
    if(!anim.valid()){
        FatalError("Error", "No animation bound to reader");
    }

    AnimationAsset &anim_asset = (*anim);

    float old_time = normalized_time;
    
    // Increment time by clock time multiplied by animation frequency
    normalized_time += timestep * GetSpeedNormalized(blendmap);
    
    int anim_id = 0;
    int active_id = (*anim).GetActiveID(blendmap, anim_id);
    ActivateEventsInRange(old_time, normalized_time, active_id);

    // Wrap looped time around if it is greater than 1
    while(anim_asset.IsLooping() && normalized_time >= 1.0f){
        if(!events.empty()){
            LOGD << "Old times: " <<  old_time << " " << normalized_time << std::endl;
        }
        old_time -= 1.0f;
        normalized_time -= 1.0f;
        if(!events.empty()){
            LOGD << "Wrapped times: " << old_time << " " << normalized_time << std::endl;
        }
        ActivateEventsInRange(old_time, normalized_time, active_id);
    } 
    if(!anim_asset.IsLooping()){
        if(old_time < 1.0f && normalized_time >= 1.0f){
            return true;
        }
    }

    return false;
}

float AnimationReader::GetTimeUntilEvent(const std::string &event){
    for(auto & i : events){
        if(i.event.event_string == event){
            if(i.time > normalized_time){
                return (*anim).AbsoluteTimeFromNormalized(i.time - normalized_time) / speed_mult * 0.001f;
            }
        }
    }
    return -1.0f;
}

void AnimationReader::GetMatrices( AnimOutput &anim_output,
                                   AnimInput &anim_input)
{
    anim_input.mirrored = mirrored;
    (*anim).GetMatrices(normalized_time, anim_output, anim_input);
	if(anim_output.old_path != "retargeted"){
		Retarget(anim_input, anim_output, anim_output.old_path);
	}

    if(!skip_offset){
        anim_output.delta_offset = anim_output.center_offset - last_offset;
    } else {
        skip_offset = false;
    }
    anim_output.delta_rotation = anim_output.rotation - last_rotation;
    last_offset = anim_output.center_offset;
    last_rotation = anim_output.rotation;

    if(blend_anim.valid()){
        AnimOutput blend_anim_output;
        (*blend_anim).GetMatrices(normalized_time,
                                  blend_anim_output, 
                                  anim_input);
		if(blend_anim_output.old_path != "retargeted"){
			Retarget(anim_input, blend_anim_output, blend_anim_output.old_path);
		}
        anim_output = add_mix(anim_output,blend_anim_output,1.0f);
    }

    for(unsigned i=0; i<anim_output.weapon_matrices.size(); ++i){
        int item_id = animated_item_ids[i];
        if(item_id != -1){
            WeapAnimInfo& weap_anim_info = anim_output.weap_anim_info_map[item_id];
            weap_anim_info.bone_transform = anim_output.weapon_matrices[i];
            weap_anim_info.weight = anim_output.weapon_weights[i] * anim_output.weapon_weight_weights[i];
            weap_anim_info.relative_id = anim_output.weapon_relative_ids[i];
            weap_anim_info.relative_weight = anim_output.weapon_relative_weights[i];
        }
    }
    
    if(!super_mobile){
        for(unsigned i=0; i<anim_output.matrices.size(); ++i){
            if(anim_input.parents && anim_input.parents->at(i) != -1){
                continue;
            }
            anim_output.matrices[i].origin[1] += anim_output.center_offset[1];
            if(!mobile){
                anim_output.matrices[i].origin[0] += anim_output.center_offset[0];
                anim_output.matrices[i].origin[2] += anim_output.center_offset[2];
            }
        }
        for(int i=0, len=anim_output.ik_bones.size(); i<len; ++i){
            vec3 &origin = anim_output.ik_bones[i].transform.origin;
            origin[1] += anim_output.center_offset[1];
            if(!mobile){
                origin[0] += anim_output.center_offset[0];
                origin[2] += anim_output.center_offset[2];
            }
        }
        anim_output.center_offset[1] = 0.0f;
        anim_output.delta_offset[1] = 0.0f;
        if(!mobile){
            anim_output.center_offset[0] = 0.0f;
            anim_output.center_offset[2] = 0.0f;
            anim_output.delta_offset[0] = 0.0f;
            anim_output.delta_offset[2] = 0.0f;
        }
    }
}

float AnimationReader::GetTime() const {
    return normalized_time;
}

void AnimationReader::SetTime(float time) {
    normalized_time = time;
    skip_offset = true;

    while(next_event < events.size() &&    
          events[next_event].time < time)
    {
        ++next_event;
    }
    if(next_event == events.size() && !events.empty()){
        next_event = 0;
    }
}

void AnimationReader::AttachTo( const AnimationAssetRef &_ref ) {
    anim = _ref;
    normalized_time = 0.0f;
    last_offset = vec3(0.0f);
    last_rotation = 0.0f;
    skip_offset = false;
    callback_string.clear();
    for(int & animated_item_id : animated_item_ids){
        animated_item_id = -1;
    }
    const AnimationAsset &anim = (*_ref);
    int anim_id = 0;
    events = anim.GetEvents(anim_id, mirrored);
    std::sort(events.begin(),
              events.end(),
              NormalizedAnimationEventCompare());

    next_event = 0;
    while((int)next_event < (int)events.size()-1 && events[next_event].time == 0.0f) {
        next_event++;
    }
}

bool AnimationReader::valid() {
    return anim.valid();
}

const AnimationReader& AnimationReader::operator=( const AnimationReader& other ) {
    anim = other.anim;
    blend_anim = other.blend_anim;
    normalized_time = other.normalized_time;
    events = other.events;
    next_event = other.next_event;
    active_events = other.active_events;
    mobile = other.mobile;
    super_mobile = other.super_mobile;
    mirrored = other.mirrored;
    last_offset = other.last_offset;
    last_rotation = other.last_rotation;
    skip_offset = other.skip_offset;
    speed_mult = other.speed_mult;
    callback_string = other.callback_string;
    for(int i=0; i<_max_animated_items; ++i){
        animated_item_ids[i] = other.animated_item_ids[i];
    }
    return *this;
}

void AnimationReader::clear()
{
    anim.clear();
}

float AnimationReader::GetAbsoluteTime(){
    return (*anim).AbsoluteTimeFromNormalized(normalized_time);
}

void AnimationReader::SetAbsoluteTime(float time){
    Animation* anm = (Animation*)&(*anim);
    SetTime(anm->NormalizedFromLocal(time));
}

bool AnimationReader::GetMobile() {
    return mobile;
}

bool AnimationReader::GetMirrored() {
    return mirrored;
}

void AnimationReader::SetMobile( bool _mobile ) {
    mobile = _mobile;
}

AnimationReader::AnimationReader():
    mobile(false),
    super_mobile(false),
    mirrored(false),
    speed_mult(1.0f)
{
    for(int & animated_item_id : animated_item_ids){
        animated_item_id = -1;
    }
}

void AnimationReader::SetMirrored( bool _mirrored ) {
    mirrored = _mirrored;

    int anim_id = 0;
    events = anim->GetEvents(anim_id, mirrored);
    std::sort(events.begin(),
              events.end(),
              NormalizedAnimationEventCompare());
}

bool AnimationReader::Finished() {
    return normalized_time >= 1.0f;
}

const std::string & AnimationReader::GetPath() {
    return anim->path_;
}

void AnimationReader::SetSuperMobile( bool _super_mobile ) {
    super_mobile = _super_mobile;
}

bool AnimationReader::GetSuperMobile() {
    return super_mobile;
}

bool AnimationReader::SameAnim( const AnimationAssetRef &_anim ) {
    return anim == _anim;
}

void AnimationReader::SetBlendAnim( AnimationAssetRef anim ) {
    LOGD << "Setting blend anim: " << anim->path_ << std::endl;
    blend_anim = anim;
}

void AnimationReader::ClearBlendAnim()
{
    blend_anim.clear();
}

void AnimationReader::SetLastRotation( float val )
{
    last_rotation = val;
}

float AnimationReader::GetLastRotation()
{
    return last_rotation;
}

void AnimationReader::GetPrevEvents( const BlendMap &blendmap, float timestep ) {
    AnimationAsset &anim_asset = (*anim);
    float old_time = normalized_time -
        timestep *
        anim_asset.GetFrequency(blendmap);

    int anim_id = 0;
    int active_id = (*anim).GetActiveID(blendmap, anim_id);
    ActivateEventsInRange(old_time, normalized_time, active_id);
}

float AnimationReader::GetAnimationEventTime( const std::string & event_name, const BlendMap &blend_map ) const
{
    for(const auto & event : events){
        if(event.time > normalized_time && event.event.event_string == event_name){
            float time = event.time - normalized_time;
            const AnimationAsset &anim_asset = (*anim);
            time /= anim_asset.GetFrequency(blend_map);
            return time;
        }
    }
    return 0.0f;
}

void AnimationReader::SetSpeedMult( float _speed_mult ) {
    speed_mult = _speed_mult;
}

void AnimationReader::SetCallbackString( const std::string& str ) {
    callback_string = str;
}

const std::string& AnimationReader::GetCallbackString() {
    return callback_string;
}

int AnimationReader::GetAnimatedItemID( int i ) {
    return animated_item_ids[i];
}

void AnimationReader::SetAnimatedItemID( int index, int id ) {
    animated_item_ids[index] = id;
}
