//-----------------------------------------------------------------------------
//           Name: animation.h
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

#include <Graphics/bonetransform.h>
#include <Graphics/ikbone.h>

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>
#include <Asset/assettypes.h>

#include <map>
#include <string>

using std::vector;
using std::map;
using std::string;

struct AnimationConfig {
	bool kDisableIK;
	bool kDisableModifiers;
	bool kDisableSoftAnimation;
	bool kDisableInterpolation;
	bool kDisableAnimationLayers;
	bool kDisableAnimationMix;
	bool kDisableAnimationTransition;
	bool kDisablePhysicsInterpolation;
	bool kForceIdleAnim;
	AnimationConfig();
};

class Skeleton;
struct SkeletonFileData;

typedef map<string, float> BlendMap;

// Animation event, for example "Punch impact" at right hand bone
struct AnimationEvent {
    int which_bone;
    string event_string;
};

struct NormalizedAnimationEvent {
    AnimationEvent event;
    float time;
    unsigned anim_id;
};

class NormalizedAnimationEventCompare {
   public:
       bool operator()(const NormalizedAnimationEvent &a, 
                       const NormalizedAnimationEvent &b) {
            return a.time < b.time;
        }
};

struct ShapeKey {
    float weight;
    string label;
};

struct StatusKey {
    float weight;
    string label;
};

struct ShapeKeyBlend {
    float weight;
    float weight_weight;
    string label;
};

struct StatusKeyBlend {
    float weight;
    float weight_weight;
    string label;
};

struct WeapAnimInfo {
    int relative_id;
    float relative_weight;
    float weight;
    BoneTransform bone_transform;
    mat4 matrix;
};

WeapAnimInfo mix(const WeapAnimInfo &a, const WeapAnimInfo &b, float b_weight);

struct Keyframe {
    // Per-bone info
    vector<BoneTransform> bone_mats;
    vector<BoneTransform> invert_bone_mats;
    vector<bool> uses_ik;
    vector<float> weights;
    // Per-weapon info
    vector<BoneTransform> weapon_mats;
    vector<int> weapon_relative_id;
    vector<float> weapon_relative_weight;
    // Misc
    vector<IKBone> ik_bones;
    vector<ShapeKey> shape_keys;
    vector<StatusKey> status_keys;
    bool use_mobility;
    mat4 mobility_mat;
    vec3 center_offset;
    float rotation;
    vector<AnimationEvent> events;
    int time;
};

const int _animation_version = 11;

struct AnimInput {
    const BlendMap& blendmap;
    const vector<int> *parents;
    bool mirrored;
    string retarget_new;
    AnimInput(const BlendMap &_blendmap, const vector<int> *_parents)
        :blendmap(_blendmap),
         parents(_parents),
         mirrored(false)
    {}
};

struct AnimOutput {
    // Get directly from animations
    vector<BoneTransform> weapon_matrices;
    vector<float> weapon_weight_weights;
    vector<float> weapon_weights; // 
    vector<float> weapon_relative_weights; // How much is this weapon relative to the bone, and how much absolute
    vector<int> weapon_relative_ids; // What bone is this weapon relative to?
    vector<BoneTransform> matrices;
    vector<float> physics_weights;
    vector<BlendedBonePath> ik_bones;
    vector<ShapeKeyBlend> shape_keys;
    vector<StatusKeyBlend> status_keys;
	string old_path;
    vec3 center_offset;
    float rotation;

    // Used for blending animations
    map<int, WeapAnimInfo> weap_anim_info_map;

    // Added by animation client
    vector<BoneTransform> unmodified_matrices;

    // Added by animation reader
    vec3 delta_offset;
    float delta_rotation;
    
    AnimOutput()
    {}
};

AnimOutput mix(const AnimOutput &a, const AnimOutput &b, float alpha);
AnimOutput add_mix( const AnimOutput &a, const AnimOutput &b, float alpha );
void MirrorBT(BoneTransform &bt, bool xy_flip);

class AnimationAsset:public AssetInfo {
public:
    AnimationAsset( AssetManager* owner, uint32_t asset_id );
    virtual void GetMatrices(float time, 
                             AnimOutput &anim_output,
                             const AnimInput &anim_input) const =0;

    virtual float GetFrequency(const BlendMap& blendmap) const =0;
    virtual float GetPeriod(const BlendMap& blendmap) const =0;
    virtual float GetGroundSpeed(const BlendMap& blendmap) const =0;
    virtual int GetActiveID(const BlendMap& blendmap, int &anim_id) const =0;
    virtual vector<NormalizedAnimationEvent> GetEvents(int& anim_id, bool mirrored) const =0;
    virtual bool IsLooping() const =0;
    virtual float AbsoluteTimeFromNormalized(float normalized_time) const=0;

    static AssetType GetType() { return ANIMATION_ASSET; }
    static const char* GetTypeName() { return "ANIMATION_ASSET"; }
    static bool AssetWarning() { return true; }
};

class Animation:public AnimationAsset {
public:
    Animation( AssetManager * owner, uint32_t asset_id );
    virtual ~Animation();

    int GetActiveID( const BlendMap& blendmap, int &anim_id ) const;
    void clear();
    void GetMatrices( float normalized_time, AnimOutput &anim_output, const AnimInput& anim_input) const;
    void WriteToFile( FILE * file );
    int ReadFromFile( FILE * file );

    int sub_error;
    int Load(const string& path, uint32_t load_flags);

    void Unload();
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }


    float GetGroundSpeed(const BlendMap& blendmap) const;
    float GetFrequency(const BlendMap& blendmap) const;
    float GetPeriod(const BlendMap& blendmap) const;
    vector<NormalizedAnimationEvent> GetEvents(int& anim_id, bool mirrored) const;
    float LocalFromNormalized(float normalized_time) const;
    float NormalizedFromLocal(float local_time) const;
    bool IsLooping() const;
    void UpdateIKBones();
    void Reload();
    virtual void ReportLoad();
    float AbsoluteTimeFromNormalized(float normalized_time) const;
    void ReturnPaths( PathSet& path_set );

    virtual AssetLoaderBase* NewLoader();
private:
    int length; // in ms
    vector<Keyframe> keyframes;
    bool looping;
    ModID modsource_;
    
    void CalcInvertBoneMats();
    void RecalcCaches();
    //void SaveCache(unsigned short checksum);
    //bool LoadCache(unsigned short checksum);
    void Center();
};

typedef AssetRef<Animation> AnimationRef;
typedef AssetRef<AnimationAsset> AnimationAssetRef;

float GetAnimationEventTime(const string& anim_path, const string& event_str);
AnimationAssetRef ReturnAnimationAssetRef(const string &path);
string extension(const string &path);
string filename(const string &path);
void Retarget(const AnimInput& anim_input, AnimOutput & anim_output, const string& old_path);
