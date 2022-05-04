//-----------------------------------------------------------------------------
//           Name: character.h
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

#include <Asset/assetinfobase.h>
#include <Asset/assetbase.h>

#include <Math/vec3.h>

#include <set>
#include <map>
#include <string>
#include <vector>

using std::string;
using std::map;
using std::vector;
using std::set;

struct MorphInfo {
    string label;
    int num_steps;
};

struct MorphMetaInfo {
    string label;
    vec3 start, end;
};

class Character : public AssetInfo {
    typedef map<string, string> StringMap;
    StringMap vis2morph;
    StringMap tags;
    string obj_path;
    string skeleton_path;
    string clothing_path;
    vector<MorphInfo> morph_info;
    vector<MorphMetaInfo> morph_meta;
    StringMap anim_paths;
    StringMap attack_paths;
    string fur_path;
    string morph_path;
    set<string> team;
    string sound_mod;
    string voice_path;
    string channels[4];
    float voice_pitch;
    float default_scale;
    float model_scale;

public:
    Character( AssetManager* asset, uint32_t asset_id );
    static AssetType GetType() { return CHARACTER_ASSET; }
    static const char* GetTypeName() { return "CHARACTER_ASSET"; }
    static bool AssetWarning() { return true; }

    bool IsOnTeam(const string &_team);
    const set<string> &GetTeamSet();
    float GetModelScale();
    float GetDefaultScale();
    const string &GetObjPath();
    const string &GetVoicePath();
    const float &GetVoicePitch();
    const string &GetClothingPath();
    const string &GetSkeletonPath();
    const string &GetFurPath();
    const string &GetPermanentMorphPath();
    const string &GetAnimPath(const string &action);
    const string &GetAttackPath(const string &action);
    const map<string, string> &GetVisemeMorphs();
    int Load(const string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void ReportLoad() override;
    const string& GetSoundMod();
    const vector<MorphInfo>& GetMorphs();
    float GetHearing();
    void ReturnPaths(PathSet &path_set) override;
    const string & GetChannel( int which ) const;
    const string & GetTag( const string &action );
    bool GetMorphMeta(const string& label, vec3 & start, vec3 & end);

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<Character> CharacterRef;
