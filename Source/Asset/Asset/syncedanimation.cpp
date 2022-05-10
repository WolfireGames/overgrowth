//-----------------------------------------------------------------------------
//           Name: syncedanimation.cpp
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
#include "syncedanimation.h"

#include <Internal/timer.h>
#include <Internal/filesystem.h>
#include <Internal/error.h>
#include <Internal/profiler.h>

#include <Graphics/animationclient.h>
#include <Math/enginemath.h>
#include <Physics/physics.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Main/engine.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <tinyxml.h>

#include <cmath>

extern AnimationConfig animation_config;

struct BlendCoordSorter {
    bool operator()(const SyncedAnimation& a, const SyncedAnimation& b) {
        return a.blend_coord < b.blend_coord;
    }
};

void SyncedAnimationGroup::AddAnimation(AnimationAssetRef animation, float blend_coord, float ground_speed, float run_bounce, const std::string& time_coord_label) {
    // Check if animation has already been added
    bool already_added = false;
    std::vector<SyncedAnimation>::iterator iter;
    for (iter = animations.begin(); iter != animations.end(); ++iter) {
        const SyncedAnimation& synced_anim = (*iter);
        if (synced_anim.animation == animation) {
            already_added = true;
        }
    }

    // If animation is not already there, add it!
    if (!already_added) {
        animations.resize(animations.size() + 1);
        SyncedAnimation& new_anim = animations.back();
        new_anim.animation = animation;
        looping = (*animation).IsLooping();

        // const AnimationAsset& base_anim = (*animation);
        new_anim.blend_coord = blend_coord;
        new_anim.ground_speed = ground_speed;
        new_anim.run_bounce = run_bounce;
        new_anim.time_coord_label = time_coord_label;

        // Make sure animations are sorted by blend_coord
        std::sort(animations.begin(),
                  animations.end(),
                  BlendCoordSorter());
    }
}

PrevNext SyncedAnimationGroup::NearestAnimations(float blend_coord) const {
    PrevNext results;

    results.prev_index = -1;
    for (unsigned i = 0; i < animations.size(); i++) {
        if (blend_coord > animations[i].blend_coord) {
            results.prev_index = i;
        }
    }
    results.next_index = min(results.prev_index + 1,
                             (int)animations.size() - 1);
    results.prev_index = max(results.prev_index, 0);

    return results;
}

unsigned SyncedAnimationGroup::NearestAnimation(float blend_coord) const {
    int closest = -1;
    float closest_distance;
    for (unsigned i = 0; i < animations.size(); i++) {
        if (closest == -1 || fabs(blend_coord - animations[i].blend_coord) < closest_distance) {
            closest = i;
            closest_distance = fabs(blend_coord - animations[i].blend_coord);
        }
    }
    return closest;
}

float SyncedAnimationGroup::GetInterpolationValue(const PrevNext& indices,
                                                  float blend_coord) const {
    if (indices.prev_index == indices.next_index) {
        return 0.0f;
    }

    float coord_a = animations[indices.prev_index].blend_coord;
    float coord_b = animations[indices.next_index].blend_coord;

    if (coord_a == coord_b) {
        return 0.0f;
    }

    return (blend_coord - coord_a) / (coord_b - coord_a);
}

namespace {
void GetMatricesFromSyncedAnimation(const SyncedAnimation& sa,
                                    float normalized_time,
                                    AnimOutput& anim_output,
                                    const AnimInput& anim_input) {
    PROFILER_ZONE(g_profiler_ctx, "GetMatricesFromSyncedAnimation");
    if (!sa.time_coord_label.empty()) {
        const BlendMap& blendmap = anim_input.blendmap;
        BlendMap::const_iterator iter = blendmap.find(sa.time_coord_label);
        if (iter != blendmap.end()) {
            normalized_time = (*iter).second;
        }
    }
    const AnimationAsset& animation = *sa.animation;
    animation.GetMatrices(normalized_time, anim_output, anim_input);
    if (sa.run_bounce != _no_ground_speed && !animation_config.kDisableModifiers) {
        float anim_speed = sa.animation->GetFrequency(anim_input.blendmap);

        float curr_ground_speed = 0.5f;
        {
            BlendMap::const_iterator iter = anim_input.blendmap.find("ground_speed");
            if (iter != anim_input.blendmap.end()) {
                curr_ground_speed = (*iter).second;
            }
        }
        if (sa.ground_speed != _no_ground_speed) {
            anim_speed *= curr_ground_speed / sa.ground_speed;
        }

        anim_speed = max(anim_speed, 2.0f);
        float time = (1.0f / anim_speed) * 0.5f;
        float initial_vel = -time * Physics::Instance()->gravity[1];
        float anim_time = normalized_time;
        if (anim_time > 0.5f) {
            anim_time -= 0.5f;
        }
        float bounce_mult = sa.run_bounce;
        float t = anim_time / anim_speed;
        anim_output.center_offset[1] += (initial_vel * t + t * t * Physics::Instance()->gravity[1]) * bounce_mult;
    }
}
}  // namespace

void SyncedAnimationGroup::GetMatrices(int first,
                                       int second,
                                       float weight,
                                       float normalized_time,
                                       AnimOutput& anim_output,
                                       const AnimInput& anim_input) const {
    AnimOutput prev_anim_output;
    GetMatricesFromSyncedAnimation(animations[first], normalized_time, prev_anim_output, anim_input);
    AnimOutput next_anim_output;
    GetMatricesFromSyncedAnimation(animations[second], normalized_time, next_anim_output, anim_input);
    if (animation_config.kDisableAnimationMix) {
        weight = floorf(weight + 0.5f);
    }
    std::string old_path;
    if (prev_anim_output.old_path != next_anim_output.old_path) {
        if (prev_anim_output.old_path != "retargeted") {
            Retarget(anim_input, prev_anim_output, prev_anim_output.old_path);
        }
        if (next_anim_output.old_path != "retargeted") {
            Retarget(anim_input, next_anim_output, next_anim_output.old_path);
        }
        old_path = "retargeted";
    } else {
        old_path = prev_anim_output.old_path;
    }
    anim_output = mix(prev_anim_output, next_anim_output, weight);
    anim_output.old_path = old_path;
}

void SyncedAnimationGroup::GetMatrices(float normalized_time,
                                       AnimOutput& anim_output,
                                       const AnimInput& anim_input,
                                       float blend_coord) const {
    LOG_ASSERT(!animations.empty());

    PrevNext nearest = NearestAnimations(blend_coord);

    if (nearest.prev_index == nearest.next_index) {
        if (!overshoot || animations.size() == 1) {
            GetMatricesFromSyncedAnimation(animations[nearest.prev_index], normalized_time, anim_output, anim_input);
        } else {
            if (nearest.prev_index == 0) {
                nearest.next_index = 1;
            }
            if (nearest.next_index == (int)animations.size() - 1) {
                nearest.prev_index = animations.size() - 2;
            }
            GetMatrices(nearest.prev_index,
                        nearest.next_index,
                        GetInterpolationValue(nearest, blend_coord),
                        normalized_time,
                        anim_output,
                        anim_input);
        }
    } else {
        GetMatrices(nearest.prev_index,
                    nearest.next_index,
                    GetInterpolationValue(nearest, blend_coord),
                    normalized_time,
                    anim_output,
                    anim_input);
    }
    if (in_air) {
        anim_output.center_offset = vec3(0.0f);
    }
}

void SyncedAnimationGroup::GetMatrices(float normalized_time,
                                       AnimOutput& anim_output,
                                       const AnimInput& anim_input) const {
    float blend_coord = 0.0f;
    BlendMap::const_iterator iter = anim_input.blendmap.find(coord_label);
    if (iter != anim_input.blendmap.end()) {
        blend_coord = (*iter).second;
    }
    GetMatrices(normalized_time, anim_output, anim_input, blend_coord);
}

float SyncedAnimationGroup::GetFrequency(const BlendMap& blendmap) const {
    float blend_coord = 0.0f;
    {
        BlendMap::const_iterator iter = blendmap.find(coord_label);
        if (iter != blendmap.end()) {
            blend_coord = (*iter).second;
        }
    }

    float curr_ground_speed = 0.5f;
    {
        BlendMap::const_iterator iter = blendmap.find("ground_speed");
        if (iter != blendmap.end()) {
            curr_ground_speed = (*iter).second;
        }
    }

    PrevNext nearest = NearestAnimations(blend_coord);
    float interp = GetInterpolationValue(nearest, blend_coord);

    // Compare animation movement speed to real movement speed, and speed
    // up or slow down animation as necessary.

    const SyncedAnimation& prev_anim = animations[nearest.prev_index];
    const SyncedAnimation& next_anim = animations[nearest.next_index];

    float prev_ground_speed;
    if (prev_anim.ground_speed == _no_ground_speed) {
        prev_ground_speed = (*prev_anim.animation).GetGroundSpeed(blendmap);
    } else {
        prev_ground_speed = prev_anim.ground_speed;
    }
    float next_ground_speed;
    if (next_anim.ground_speed == _no_ground_speed) {
        next_ground_speed = (*next_anim.animation).GetGroundSpeed(blendmap);
    } else {
        next_ground_speed = next_anim.ground_speed;
    }

    float prev_freq_mult = curr_ground_speed / prev_ground_speed;
    float next_freq_mult = curr_ground_speed / next_ground_speed;

    if (prev_anim.ground_speed == _no_ground_speed) {
        prev_freq_mult = 1.0f;
    }
    if (prev_anim.ground_speed == _no_ground_speed) {
        next_freq_mult = 1.0f;
    }

    float prev_frequency = (*prev_anim.animation).GetFrequency(blendmap) * prev_freq_mult;
    float next_frequency = (*next_anim.animation).GetFrequency(blendmap) * next_freq_mult;

    return mix(prev_frequency, next_frequency, interp);
}

float SyncedAnimationGroup::GetPeriod(const BlendMap& blendmap) const {
    return 1.0f / GetFrequency(blendmap);
}

int SyncedAnimationGroup::Load(const std::string& rel_path, uint32_t load_flags) {
    char abs_path[kPathSize];
    if (FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
        return kLoadErrorMissingFile;
    }

    TiXmlDocument doc(abs_path);
    if (!doc.LoadFile()) {
        return kLoadErrorCouldNotOpenXML;
    }

    clear();

    TiXmlHandle hDoc(&doc);
    TiXmlHandle hRoot = hDoc.FirstChildElement();
    TiXmlElement* coord_label_element = hRoot.FirstChild("CoordLabel").Element();
    coord_label = coord_label_element->GetText();

    TiXmlElement* overshoot_element = hRoot.FirstChild("Overshoot").Element();
    if (overshoot_element && !strcmp(overshoot_element->GetText(), "true")) {
        overshoot = true;
    }
    TiXmlElement* in_air_element = hRoot.FirstChild("InAir").Element();
    if (in_air_element && !strcmp(in_air_element->GetText(), "true")) {
        in_air = true;
    }

    TiXmlElement* animation_element = hRoot.FirstChild("Animations").FirstChild().Element();
    while (animation_element != NULL) {
        std::string anim_path = animation_element->Attribute("path");
        float weight_coord = 0.0f;
        animation_element->QueryFloatAttribute("coord", &weight_coord);
        std::string time_coord_label;
        if (animation_element->Attribute("time_coord_label")) {
            time_coord_label = animation_element->Attribute("time_coord_label");
        }
        float ground_speed = 0.0f;
        if (animation_element->QueryFloatAttribute("ground_speed", &ground_speed) == TIXML_NO_ATTRIBUTE) {
            ground_speed = _no_ground_speed;
        }

        float run_bounce = 0.0f;
        if (animation_element->QueryFloatAttribute("run_bounce", &run_bounce) == TIXML_NO_ATTRIBUTE) {
            run_bounce = _no_ground_speed;
        }

        AnimationAssetRef new_ref;
        if (extension(anim_path) == "xml") {
            // SyncedAnimationGroups *SAGs = SyncedAnimationGroups::Instance();
            SyncedAnimationGroupRef ref = Engine::Instance()->GetAssetManager()->LoadSync<SyncedAnimationGroup>(anim_path);
            new_ref = AnimationAssetRef(&(*ref));
        } else if (extension(anim_path) == "anm") {
            // Animations *anims = Animations::Instance();
            AnimationRef ref = Engine::Instance()->GetAssetManager()->LoadSync<Animation>(anim_path);
            new_ref = AnimationAssetRef(&(*ref));
        }

        AddAnimation(new_ref, weight_coord, ground_speed, run_bounce, time_coord_label);

        animation_element = animation_element->NextSiblingElement();
    }
    return kLoadOk;
}

const char* SyncedAnimationGroup::GetLoadErrorString() {
    return "";
}

void SyncedAnimationGroup::Unload() {
}

void SyncedAnimationGroup::clear() {
    animations.clear();
    looping = false;
    overshoot = false;
    in_air = false;
}

bool SyncedAnimationGroup::IsLooping() const {
    return looping;
}

SyncedAnimationGroup::SyncedAnimationGroup(AssetManager* owner, uint32_t asset_id) : AnimationAsset(owner, asset_id) {
    clear();
}

void SyncedAnimationGroup::Reload() {
    Load(path_, 0x0);
}

void SyncedAnimationGroup::ReportLoad() {
}

float SyncedAnimationGroup::GetGroundSpeed(const BlendMap& blendmap) const {
    float curr_ground_speed = 0.5f;
    {
        BlendMap::const_iterator iter = blendmap.find("ground_speed");
        if (iter != blendmap.end()) {
            curr_ground_speed = (*iter).second;
        }
    }
    return curr_ground_speed;
}

std::vector<NormalizedAnimationEvent> SyncedAnimationGroup::GetEvents(int& anim_id, bool mirrored) const {
    std::vector<NormalizedAnimationEvent> normalized_events;
    for (const auto& i : animations) {
        std::vector<NormalizedAnimationEvent> new_events;
        const AnimationAsset& animation = (*i.animation);
        new_events = animation.GetEvents(anim_id, mirrored);
        normalized_events.insert(normalized_events.end(),
                                 new_events.begin(),
                                 new_events.end());
    }

    return normalized_events;
}

int SyncedAnimationGroup::GetActiveID(const BlendMap& blendmap, int& anim_id) const {
    float blend_coord = 0.0f;
    BlendMap::const_iterator iter = blendmap.find(coord_label);
    if (iter != blendmap.end()) {
        blend_coord = (*iter).second;
    }
    unsigned which_anim = NearestAnimation(blend_coord);

    for (unsigned i = 0; i < animations.size(); i++) {
        if (i == which_anim) {
            // LOGD << (*animations[i].animation).path << std::endl;
            return (*animations[i].animation).GetActiveID(blendmap, anim_id);
        } else {
            (*animations[i].animation).GetActiveID(blendmap, anim_id);
        }
    }

    return -1;
}

float SyncedAnimationGroup::AbsoluteTimeFromNormalized(float normalized_time) const {
    return animations[0].animation->AbsoluteTimeFromNormalized(normalized_time);
}

void SyncedAnimationGroup::ReturnPaths(PathSet& path_set) {
    path_set.insert("synced_animation " + path_);
    for (auto& animation : animations) {
        animation.animation->ReturnPaths(path_set);
    }
}

AssetLoaderBase* SyncedAnimationGroup::NewLoader() {
    return new FallbackAssetLoader<SyncedAnimationGroup>();
}

/* This old forced order disposal of assets will not be necessary anymore due to the new reference counting delete method in asset manager
 * which will remove from the top, going down, removing remaining references.
void SyncedAnimationGroups::Dispose() {
    typedef std::list<SyncedAnimationGroup> SAGlist;
    SAGlist &animations = assets_;
    while(!animations.empty()){
        bool deleted_something = false;
        for(SAGlist::iterator iter = animations.begin(); iter != animations.end();){
            SyncedAnimationGroup& group = *iter;
            if(group.ref_count_ == 0){
                iter = animations.erase(iter);
                deleted_something = true;
            } else {
                ++iter;
            }
        }
        if(!deleted_something) {
            FatalError("Error", "Possible circular reference in Synced Animation Groups");
        }
    }
    AssetContainer<SyncedAnimationGroup>::Dispose();
}
*/
