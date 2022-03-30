//-----------------------------------------------------------------------------
//           Name: animationreader.h
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

#include <queue>

const int _max_animated_items = 4;

class AnimationReader {
        AnimationAssetRef anim;
        AnimationAssetRef blend_anim;
        float normalized_time;
        std::vector<NormalizedAnimationEvent> events;
        unsigned next_event;
        void ActivateEventsInRange(float start, float end, int active_id);
        std::queue<AnimationEvent> active_events;
        bool mobile;
        bool super_mobile;
        bool mirrored;
        vec3 last_offset;
        float last_rotation;
        bool skip_offset;
        float speed_mult;
        std::string callback_string;
        int animated_item_ids[_max_animated_items];

    public:
        AnimationReader();
        const std::string& GetCallbackString();
        int GetAnimatedItemID(int i);
        void SetCallbackString(const std::string& str);
        void SetMobile(bool _mobile);
        void SetSuperMobile(bool _super_mobile);
        void clear();
        void SetMirrored(bool _mirrored);
        bool valid();
        bool GetMirrored();
        bool GetMobile();
        bool GetSuperMobile();
        float GetLastRotation();
        void SetLastRotation(float val);
        float GetAbsoluteTime();
        void SetAbsoluteTime(float time);
        float GetTime() const;
        void SetTime(float time);
        void AppliedOffset(float amount);
        void AttachTo( const AnimationAssetRef &_ref );
        bool IncrementTime( const BlendMap &blendmap, float timestep);
        const std::string &GetPath();
        const AnimationReader& operator=(const AnimationReader& other);
        std::queue<AnimationEvent>& GetActiveEvents();
        void GetMatrices( AnimOutput &anim_output, AnimInput &anim_input);
        bool Finished();
        bool SameAnim(const AnimationAssetRef &_anim);
        void SetBlendAnim( AnimationAssetRef anim );
        void ClearBlendAnim();
        void GetPrevEvents( const BlendMap &blendmap, float timestep );
        float GetAnimationEventTime( const std::string & event_name, const BlendMap &blend_map ) const;
        void SetSpeedMult( float speed_mult );
        float GetTimeUntilEvent(const std::string &event);
        void SetAnimatedItemID( int index, int id );
        float GetSpeedNormalized(const BlendMap &blendmap) const;
};
