//-----------------------------------------------------------------------------
//           Name: animationeffect.h
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

#include <Asset/Asset/texture.h>
#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>

#include <Math/quaternions.h>

#include <vector>
#include <string>

using std::string;
using std::vector;

class TheoraVideoClip;
class TheoraMemoryFileDataSource;
class AssetManager;

class AnimationEffect : public Asset {
public:
    string video_path;
    vector<TextureAssetRef> frames;
    int frame_rate;

    void clear();

    int sub_error;
    int Load(const string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    
    void Unload();
    void Reload();
    virtual void ReportLoad();

    ~AnimationEffect();
    AnimationEffect(AssetManager* owner, uint32_t asset_id);
    
    static AssetType GetType() { return ANIMATION_EFFECT_ASSET; }
    static const char* GetTypeName() { return "ANIMATION_EFFECT_ASSET"; }

    virtual AssetLoaderBase* NewLoader();
    static bool AssetWarning() { return true; }
};

typedef AssetRef<AnimationEffect> AnimationEffectRef;
class ASContext;

class AnimationEffectReader {
    float time;
    AnimationEffectRef ae_ref;
    bool use_theora;
    TheoraVideoClip *clip;
    TheoraMemoryFileDataSource* clip_mem;
    TextureRef clip_tex;
public:
    bool Done();
    void Dispose();
    void Update(float timestep);
    bool valid();
    void AttachTo(const AnimationEffectRef& _ae_ref);
    TextureRef &GetTextureAssetRef();
    AnimationEffectReader();
    AnimationEffectReader(const AnimationEffectReader& other);
    ~AnimationEffectReader();
};

