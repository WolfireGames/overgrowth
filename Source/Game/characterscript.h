//-----------------------------------------------------------------------------
//           Name: characterscript.h
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

#include <Asset/Asset/character.h>
#include <Asset/Asset/item.h>

#include <vector>

class ASContext;

class CharacterScriptGetter {
    CharacterRef character_ref;
    std::vector<ItemRef> items;
    typedef std::map<std::string, std::string> CharAnimOverrideMap;
    CharAnimOverrideMap char_anim_overrides_;

public:
    void OverrideCharAnim(const std::string &label, const std::string &new_path);
    void AttachToScript( ASContext *as_context, const std::string& as_name );
    void AttachExtraToScript( ASContext *as_context, const std::string& as_name );
    void ItemsChanged( const std::vector<ItemRef> &_items );
    const std::string GetClothingPath();
    const std::string& GetSoundMod();
    bool Load( const std::string& path );
    std::string GetAttackPath( const std::string& action );
    std::string GetAnimPath( const std::string& action );
    std::string GetSkeletonPath( );
    std::string GetObjPath( );
    bool GetMorphMetaPoints(const std::string& label, vec3 &start, vec3 &end);
    float GetDefaultScale();
    float GetModelScale();
    int OnSameTeam( const std::string& character_path );
    float GetHearing();
    char GetAnimFlags( const std::string& path );
    void GetTeamString(std::string &str);
    const std::string &GetChannel(int which);
    const std::string &GetTag( const std::string &key );
};
