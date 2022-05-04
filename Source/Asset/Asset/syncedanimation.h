//-----------------------------------------------------------------------------
//           Name: syncedanimation.h
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
#pragma once

#include <Asset/Asset/animation.h>
#include <Asset/assettypes.h>
#include <Internal/integer.h>

class AssetManager;

struct PrevNext { 
    int prev_index;
    int next_index;
};

const float _no_ground_speed = -1.0f;

struct SyncedAnimation {
    AnimationAssetRef animation;
    float blend_coord;
    float ground_speed;
    float run_bounce;
    std::string time_coord_label;
};

class SyncedAnimationGroup: public AnimationAsset {
        std::vector<SyncedAnimation> animations;
        std::string coord_label;
        bool looping;
        bool overshoot;
        bool in_air;

        // Get the two animations closest to the blend_coord
        PrevNext NearestAnimations(float blend_coord) const;

        // Returns the weight of the next index. For example, returns
        // 0 if blend_coord is exactly on the previous index, and
        // returns 1 if it is exactly on the next index. 
        float GetInterpolationValue(const PrevNext &indices, 
                                    float blend_coord) const;
    public:
        SyncedAnimationGroup(AssetManager* owner, uint32_t asset_id);
        void AddAnimation( AnimationAssetRef animation, float blend_coord , float ground_speed, float run_bounce, const std::string& time_coord_label );
        void GetMatrices(float normalized_time,
                         AnimOutput &anim_output,
                         const AnimInput &anim_input) const override;
        void GetMatrices(float normalized_time,
                         AnimOutput &anim_output,
                         const AnimInput &anim_input,
                         float blend_coord) const;
        void GetMatrices(int first, 
                         int second, 
                         float weight, 
                         float normalized_time, 
                         AnimOutput &anim_output, 
                         const AnimInput &anim_input) const;
        float GetFrequency(const BlendMap& blendmap) const override;
        float GetGroundSpeed(const BlendMap& blendmap) const override;
        float GetPeriod(const BlendMap& blendmap) const override;
        std::vector<NormalizedAnimationEvent> GetEvents(int &anim_id, bool mirrored) const override;
        int Load(const std::string &path, uint32_t load_flags);
        const char* GetLoadErrorString();
        const char* GetLoadErrorStringExtended() { return ""; }
        void Unload();
        void Reload();
        void ReportLoad() override;
        void clear();
        int GetActiveID(const BlendMap& blendmap, int &anim_id) const override;
        bool IsLooping() const override;
        void CalcMirrored(bool cache, const std::string &skeleton_path);
        unsigned NearestAnimation(float blend_coord) const;
        float AbsoluteTimeFromNormalized( float normalized_time ) const override;
        void ReturnPaths(PathSet& path_set) override;

        static AssetType GetType() { return SYNCED_ANIMATION_GROUP_ASSET; }
        static const char* GetTypeName() { return "SYNCED_ANIMATION_GROUP_ASSET"; }
        static bool AssetWarning() { return true; }

        AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<SyncedAnimationGroup> SyncedAnimationGroupRef;
