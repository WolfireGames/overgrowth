//-----------------------------------------------------------------------------
//           Name: animationclient.h
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

#include <Graphics/animationreader.h>
#include <Graphics/animationflags.h>

#include <Asset/Asset/animation.h>

class Skeleton;
class ASContext;
const float _default_fade_speed = 5.0f;
typedef int32_t ASFunctionHandle;

struct AnimationFadeOut {
    AnimationReader reader;
    float opac;
    BlendMap blendmap;
    float fade_speed;
    bool overshoot;
};

struct FadeCollection {
    std::vector<AnimationFadeOut> fade_out;
    void Update(const BlendMap &blendmap, ASContext *as_context, float timestep);
    float GetOpac();
    void ApplyAngular(AnimInput &ang_anim_input, AnimOutput &ang_anim_output, AnimOutput &old_ang_anim_output, const std::string &retarget_new, const std::vector<int> *parents);
    void AddFadingAnimation(AnimationReader &reader, const BlendMap &blendmap, float fade_speed);
    void clear();
};

struct AnimationLayer {
    FadeCollection fade_out;
    AnimationReader reader;
    float fade_opac;
    float opac;
    float opac_mult;
    float fade_speed;
    bool fading;
    int id;
};

class AnimationClient {
    // Keeping these here instead of local just to avoid having to do as much
    // memory allocation/deallocation
    AnimOutput ang_anim_output;
    AnimOutput temp_ang_anim_output;
    AnimOutput temp_ang_anim_output2;
    AnimOutput lin_anim_output;
    AnimOutput temp_lin_anim_output;
    AnimOutput temp_lin_anim_output2;

    FadeCollection fade_out;
    std::vector<AnimationLayer> layers;
    AnimationReader reader;
    BlendMap blendmap;
    std::string current_anim;
    mat4 rotation_matrix;
    std::vector<float> bone_masses;
    vec3 delta_offset_overflow;
    float delta_rotation_overflow;
    float special_rotation;

    ASContext *as_context;
    Skeleton *skeleton;
    char flags;

    std::string retarget_new;

    struct {
        ASFunctionHandle layer_removed;
    } as_funcs;

    void ApplyAnimationFlags(AnimationReader &reader, AnimationAssetRef &new_ref, char flags);

   public:
    void SetAnimation(const std::string &path, float fade_speed = _default_fade_speed, char flags = 0);
    int AddLayer(const std::string &path, float fade_speed = _default_fade_speed, char flags = 0);
    void SetBlendCoord(const std::string &label, float coord);
    void Update(float timestep);

    void SetBoneMasses(const std::vector<float> &bone_masses);

    void SetRotationMatrix(const mat4 &matrix);
    void GetMatrices(AnimOutput &anim_output, const std::vector<int> *parents);
    void SetASContext(ASContext *_as_context);
    void SetCallbackString(const std::string &_callback_string);
    void SwapAnimation(const std::string &path, char flags = 0);
    const mat4 &GetRotationMatrix();
    vec3 GetAvgTranslation(const std::vector<BoneTransform> &matrices);
    void Reset();
    void SetSkeleton(Skeleton *_skeleton);
    std::queue<AnimationEvent> GetActiveEvents();
    void SetRetargeting(const std::string &new_path);
    void UpdateLayers(float timestep);
    void SetLayerOpacity(int layer, float opac);
    void RemoveLayer(int which, float fade_speed);
    void SetBlendAnimation(const std::string &path);
    void AddAnimationOffset(const vec3 &offset);
    void AddAnimationRotOffset(float offset);
    void ClearBlendAnimation();
    float GetAnimationEventTime(const std::string &event_name) const;
    AnimationClient();
    void SetSpeedMult(float speed_mult);
    float GetTimeUntilEvent(const std::string &event);
    void SetAnimatedItemID(int index, int id);
    void SetLayerItemID(int layer_id, int index, int item_id);
    const std::string &GetCurrAnim() const;
    float GetNormalizedAnimTime() const;
    float GetAnimationSpeed() const;
    void RemoveAllLayers();
};
