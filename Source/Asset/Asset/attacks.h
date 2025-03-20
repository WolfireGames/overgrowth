//-----------------------------------------------------------------------------
//           Name: attacks.h
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

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>

#include <Math/vec3.h>

#include <string>

using std::string;

enum AttackHeight {
    _high = 0,
    _medium = 1,
    _low = 2
};

enum AttackDirection {
    _right = 0,
    _front = 1,
    _left = 2
};

enum CutPlaneType {
    _heavy = 0,
    _light = 1
};

class AssetManager;

class Attack : public AssetInfo {
   public:
    string special;
    string unblocked_anim_path;
    string blocked_anim_path;
    string throw_anim_path;
    string thrown_anim_path;
    string thrown_counter_anim_path;
    AttackHeight height;
    AttackDirection direction;
    bool swap_stance;
    bool swap_stance_blocked;
    bool unblockable;
    bool flesh_unblockable;
    bool mobile;
    bool as_layer;
    string alternate;
    string reaction;
    vec3 impact_dir;
    float sharp_damage[2];
    float block_damage[2];
    float damage[2];
    float force[2];
    bool has_range_adjust;
    float range_adjust;
    string materialevent;
    vec3 cut_plane;
    bool has_cut_plane;
    CutPlaneType cut_plane_type;
    bool has_stab_dir;
    vec3 stab_dir;
    CutPlaneType stab_type;

    Attack(AssetManager* owner, uint32_t asset_id);
    void ReturnPaths(PathSet& path_set) override;
    int Load(const string& path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }

    void Unload();
    void Reload();
    void ReportLoad() override;
    void clear();
    static AssetType GetType() { return ATTACK_ASSET; }
    static const char* GetTypeName() { return "ATTACK_ASSET"; }
    static bool AssetWarning() { return true; }

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<Attack> AttackRef;
