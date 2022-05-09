//-----------------------------------------------------------------------------
//           Name: characterscript.cpp
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
#include "characterscript.h"

#include <Scripting/angelscript/ascontext.h>
#include <Objects/itemobject.h>
#include <Main/engine.h>

bool CharacterScriptGetter::Load(const std::string &_path) {
    // Characters::Instance()->ReturnRef(_path);
    character_ref = Engine::Instance()->GetAssetManager()->LoadSync<Character>(_path);
    if (character_ref.valid()) {
        return true;
    } else {
        return false;
    }
}

std::string CharacterScriptGetter::GetObjPath() {
    return character_ref->GetObjPath();
}

std::string CharacterScriptGetter::GetSkeletonPath() {
    return character_ref->GetSkeletonPath();
}

std::string CharacterScriptGetter::GetAnimPath(const std::string &action) {
    for (auto &item : items) {
        if (item->HasAnimOverride(action)) {
            return item->GetAnimOverride(action);
        }
    }
    CharAnimOverrideMap::iterator iter = char_anim_overrides_.find(action);
    if (iter == char_anim_overrides_.end()) {
        return character_ref->GetAnimPath(action);
    } else {
        return iter->second;
    }
}

const std::string &CharacterScriptGetter::GetTag(const std::string &key) {
    return character_ref->GetTag(key);
}

char CharacterScriptGetter::GetAnimFlags(const std::string &action) {
    for (auto &item : items) {
        if (item->HasAnimOverride(action)) {
            return item->GetAnimOverrideFlags(action);
        }
    }
    return 0;
}

std::string CharacterScriptGetter::GetAttackPath(const std::string &action) {
    for (auto &item : items) {
        if (item->HasAttackOverride(action)) {
            return item->GetAttackOverride(action);
        }
    }
    return character_ref->GetAttackPath(action);
}

int CharacterScriptGetter::OnSameTeam(const std::string &char_path) {
    // CharacterRef other = Characters::Instance()->ReturnRef(char_path);
    CharacterRef other = Engine::Instance()->GetAssetManager()->LoadSync<Character>(char_path);
    const std::set<std::string> &team_set = character_ref->GetTeamSet();
    for (const auto &iter : team_set) {
        if (other->IsOnTeam(iter)) {
            return 1;
        }
    }
    return 0;
}

void CharacterScriptGetter::GetTeamString(std::string &str) {
    str.clear();
    const std::set<std::string> &team_set = character_ref->GetTeamSet();
    for (std::set<std::string>::const_iterator iter = team_set.begin();
         iter != team_set.end();
         ++iter) {
        if (iter != team_set.begin()) {
            str += ", ";
        }
        str += (*iter);
    }
}

void CharacterScriptGetter::AttachToScript(ASContext *as_context, const std::string &as_name) {
    as_context->RegisterObjectType("CharacterScriptGetter", 0, asOBJ_REF | asOBJ_NOHANDLE, "Can load a character xml and provide access to its data");
    as_context->RegisterObjectMethod("CharacterScriptGetter", "bool Load(const string &in)", asMETHOD(CharacterScriptGetter, Load), asCALL_THISCALL, "Load a character xml file (e.g. \"Data/Characters/guard.xml\")");
    as_context->RegisterObjectMethod("CharacterScriptGetter", "string GetObjPath()", asMETHOD(CharacterScriptGetter, GetObjPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "string GetSkeletonPath()", asMETHOD(CharacterScriptGetter, GetSkeletonPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "string GetAnimPath(const string &in anim_label)", asMETHOD(CharacterScriptGetter, GetAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "const string& GetTag(const string &in key)", asMETHOD(CharacterScriptGetter, GetTag), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "string GetAttackPath(const string &in attack_label)", asMETHOD(CharacterScriptGetter, GetAttackPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "void GetTeamString(string &out team_string)", asMETHOD(CharacterScriptGetter, GetTeamString), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "float GetHearing()", asMETHOD(CharacterScriptGetter, GetHearing), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CharacterScriptGetter", "const string& GetChannel(int which_channel)", asMETHOD(CharacterScriptGetter, GetChannel), asCALL_THISCALL, "Get type of given color channel (e.g. \"fur\", \"cloth\")");
    as_context->RegisterObjectMethod("CharacterScriptGetter", "bool GetMorphMetaPoints(const string &in label, vec3 &out start, vec3 &out end)", asMETHOD(CharacterScriptGetter, GetMorphMetaPoints), asCALL_THISCALL);
    as_context->DocsCloseBrace();
    as_context->RegisterGlobalProperty(("CharacterScriptGetter " + as_name).c_str(), this);
}

const std::string &CharacterScriptGetter::GetChannel(int which) {
    return character_ref->GetChannel(which);
}

void CharacterScriptGetter::AttachExtraToScript(ASContext *as_context, const std::string &as_name) {
    as_context->RegisterGlobalProperty(("CharacterScriptGetter " + as_name).c_str(), this);
}

void CharacterScriptGetter::ItemsChanged(const std::vector<ItemRef> &_items) {
    items = _items;
}

const std::string CharacterScriptGetter::GetClothingPath() {
    return character_ref->GetClothingPath();
}

const std::string &CharacterScriptGetter::GetSoundMod() {
    return character_ref->GetSoundMod();
}

float CharacterScriptGetter::GetHearing() {
    return character_ref->GetHearing();
}

void CharacterScriptGetter::OverrideCharAnim(const std::string &label, const std::string &new_path) {
    char_anim_overrides_[label] = new_path;
}

float CharacterScriptGetter::GetDefaultScale() {
    return character_ref->GetDefaultScale();
}

float CharacterScriptGetter::GetModelScale() {
    return character_ref->GetModelScale();
}

bool CharacterScriptGetter::GetMorphMetaPoints(const std::string &label, vec3 &start, vec3 &end) {
    return character_ref->GetMorphMeta(label, start, end);
}
