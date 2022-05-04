//-----------------------------------------------------------------------------
//           Name: item.h
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
#include <Game/attachment_type.h>

#include <string>
#include <map>
#include <vector>

enum ItemType {
    _item_no_type,
    _weapon,
    _misc,
    _collectable
};

struct WeaponLine {
    std::string material;
    std::string start;
    std::string end;
};

struct ItemAttachment {
    bool exists;
    std::string ik_label;
    std::string anim;
    std::string anim_base;
    ItemAttachment();
};

class Item : public AssetInfo {
public:
    Item( AssetManager* owner, uint32_t asset_id );
    static AssetType GetType() { return ITEM_ASSET; }
    static const char* GetTypeName() { return "ITEM_ASSET"; }
    static bool AssetWarning() { return true; }

    int sub_error;
    int Load(const std::string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void ReportLoad() override;
    void ReturnPaths(PathSet &path_set) override;
    
    const std::string &GetIKAttach(AttachmentType type) const;
    const std::string &GetAttachAnimPath(AttachmentType type);
    const std::string &GetAttachAnimBasePath(AttachmentType type);
    const std::string &GetObjPath();
    bool HasAnimOverride( const std::string &anim );
    const std::string &GetAnimOverride( const std::string &anim );
    char GetAnimOverrideFlags( const std::string &anim );
    bool HasAnimBlend( const std::string &anim );
    const std::string &GetAnimBlend( const std::string &anim );
    bool HasAttackOverride( const std::string &anim );
    const std::string &GetAttackOverride( const std::string &anim );
    vec3 GetPoint(const std::string &name);
    size_t GetNumLines();
    const vec3& GetLineStart(int which);
    const vec3& GetLineEnd(int which);
    const std::string& GetLineMaterial(int which);
    float GetMass();
    float GetRangeExtender();
    ItemType GetItemType();
    float GetRangeMultiplier();
    bool HasAttachment(AttachmentType type);
    int GetNumHands();
    const std::string &GetLabel();
    const std::string & GetContains() const;
    const std::string& GetSoundModifier();
    const std::list<std::string> & GetAttachmentList();
    bool HasReactionOverride( const std::string &anim );
    const std::string & GetReactionOverride( const std::string &anim );

    AssetLoaderBase* NewLoader() override;

private:
    static const int kMaxAttachments = 4;
    ItemAttachment attachment[kMaxAttachments];
    std::list<std::string> attachments;
    std::string obj_path;
    std::map<std::string, vec3> points;
    std::vector<WeaponLine> lines;
    std::map<std::string, std::string> anim_overrides;
    std::map<std::string, std::string> reaction_overrides;
    std::map<std::string, char> anim_override_flags;
    std::map<std::string, std::string> anim_blend;
    std::map<std::string, std::string> attack_overrides;
    std::string contains;
    std::string sound_modifier_;
    std::string label;
    ItemType type;
    int hands;
    float mass;
    float range_extender;
    float range_multiplier;
};

typedef AssetRef<Item> ItemRef;
