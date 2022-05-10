//-----------------------------------------------------------------------------
//           Name: character.cpp
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
#include "character.h"

#include <Asset/Asset/objectfile.h>
#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/attacks.h>
#include <Asset/Asset/animation.h>
#include <Asset/Asset/voicefile.h>
#include <Asset/Asset/material.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>
#include <Internal/error.h>
#include <Main/engine.h>

#include <tinyxml.h>

#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <set>

using std::endl;
using std::map;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;

Character::Character(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id) {
}

void Character::Reload() {
    Load(path_, 0x0);
}

void Character::ReportLoad() {
}

int Character::Load(const string& path, uint32_t load_flags) {
    TiXmlDocument doc;

    if (LoadXMLRetryable(doc, path, "Character")) {
        obj_path.clear();
        skeleton_path.clear();
        anim_paths.clear();
        attack_paths.clear();
        fur_path.clear();
        morph_path.clear();
        morph_info.clear();
        team.clear();
        voice_pitch = 1.0f;
        model_scale = 1.0f;
        default_scale = 1.0f;

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement()->FirstChildElement();
        for (; field; field = field->NextSiblingElement()) {
            string field_str(field->Value());
            if (field_str == "appearance") {
                obj_path = field->Attribute("obj_path");
                skeleton_path = field->Attribute("skeleton");
                const char* fur_path_cstr = field->Attribute("fur");
                if (fur_path_cstr) {
                    fur_path = fur_path_cstr;
                }
                const char* morph_path_cstr = field->Attribute("morph");
                if (morph_path_cstr) {
                    morph_path = morph_path_cstr;
                }
                for (int i = 0; i < 4; ++i) {
                    ostringstream oss;
                    oss << "channel_" << i;
                    const char* channel_cstr = field->Attribute(oss.str().c_str());
                    if (channel_cstr) {
                        channels[i] = channel_cstr;
                    }
                }
                field->QueryFloatAttribute("model_scale", &model_scale);
                field->QueryFloatAttribute("default_scale", &default_scale);
            } else if (field_str == "animations") {
                TiXmlAttribute* attrib = field->FirstAttribute();
                while (attrib) {
                    anim_paths[attrib->Name()] = attrib->Value();
                    attrib = attrib->Next();
                }
            } else if (field_str == "attacks") {
                TiXmlAttribute* attrib = field->FirstAttribute();
                while (attrib) {
                    attack_paths[attrib->Name()] = attrib->Value();
                    attrib = attrib->Next();
                }
            } else if (field_str == "tags") {
                TiXmlAttribute* attrib = field->FirstAttribute();
                while (attrib) {
                    tags[attrib->Name()] = attrib->Value();
                    attrib = attrib->Next();
                }
            } else if (field_str == "voice") {
                voice_path = field->Attribute("path");
                field->QueryFloatAttribute("pitch", &voice_pitch);
            } else if (field_str == "team") {
                team.insert(field->GetText());
            } else if (field_str == "clothing") {
                clothing_path = field->GetText();
            } else if (field_str == "soundmod") {
                sound_mod = field->GetText();
            } else if (field_str == "morphs") {
                MorphInfo new_morph_info;
                TiXmlElement* morph = field->FirstChildElement();
                while (morph) {
                    const char* default_str = morph->Attribute("default");
                    if (!default_str || strcmp(morph->Attribute("default"), "true") != 0) {
                        new_morph_info.label = morph->Value();
                        new_morph_info.num_steps = 1;
                        morph->QueryIntAttribute("num_steps", &new_morph_info.num_steps);
                        morph_info.push_back(new_morph_info);
                    }
                    vec3 start, end;
                    if (morph->QueryFloatAttribute("start_x", &start[0]) == TIXML_SUCCESS) {
                        morph->QueryFloatAttribute("start_y", &start[1]);
                        morph->QueryFloatAttribute("start_z", &start[2]);
                        morph->QueryFloatAttribute("end_x", &end[0]);
                        morph->QueryFloatAttribute("end_y", &end[1]);
                        morph->QueryFloatAttribute("end_z", &end[2]);
                        MorphMetaInfo meta_info;
                        meta_info.label = morph->Value();
                        meta_info.start = start;
                        meta_info.end = end;
                        morph_meta.push_back(meta_info);
                    }
                    morph = morph->NextSiblingElement();
                }
            } else if (field_str == "visemes") {
                TiXmlElement* viseme = field->FirstChildElement();
                string viseme_morph;
                while (viseme) {
                    viseme_morph = viseme->GetText();
                    vis2morph[viseme->Value()] = viseme_morph;
                    viseme = viseme->NextSiblingElement();
                }
            }
        }
    } else {
        return kLoadErrorMissingFile;
    }
    return kLoadOk;
}

const char* Character::GetLoadErrorString() {
    return "";
}

void Character::Unload() {
}

const string& Character::GetAnimPath(const string& action) {
    return anim_paths[action];
}

const string& Character::GetTag(const string& action) {
    return tags[action];
}

const string& Character::GetObjPath() {
    return obj_path;
}

const string& Character::GetClothingPath() {
    return clothing_path;
}

const string& Character::GetSkeletonPath() {
    return skeleton_path;
}

const string& Character::GetAttackPath(const string& action) {
    return attack_paths[action];
}

bool Character::IsOnTeam(const string& _team) {
    /*
    LOGI << "Checking if " << _team << " is on team:" << endl;
    for(set<string>::const_iterator iter = team.begin();
        iter != team.end();
        ++iter)
    {
        LOGI << (*iter) << endl;
    }
    bool found = (team.find(_team) != team.end());
    if(found){
        LOGI << "It is." << endl;
        return true;
    }
    LOGI << "It is not." << endl;
    return false;
    */
    return (team.find(_team) != team.end());
}

const set<string>& Character::GetTeamSet() {
    return team;
}

const string& Character::GetSoundMod() {
    return sound_mod;
}

const vector<MorphInfo>& Character::GetMorphs() {
    return morph_info;
}

const map<string, string>& Character::GetVisemeMorphs() {
    return vis2morph;
}

const string& Character::GetVoicePath() {
    return voice_path;
}

const float& Character::GetVoicePitch() {
    return voice_pitch;
}

float Character::GetHearing() {
    return 0.4f;
}

void Character::ReturnPaths(PathSet& path_set) {
    path_set.insert("character " + path_);
    // ObjectFiles::Instance()->ReturnRef(obj_path);
    ObjectFileRef ofr = Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(obj_path);
    ofr->ReturnPaths(path_set);
    path_set.insert("skeleton " + skeleton_path);
    if (!clothing_path.empty()) {
        // Materials::Instance()->ReturnRef(clothing_path)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Material>(clothing_path)->ReturnPaths(path_set);
    }
    for (auto& info : morph_info) {
        if (info.num_steps == 1) {
            path_set.insert("morph " + GetMorphPath(ofr->model_name, info.label));
        } else {
            for (int j = 0; j < info.num_steps; ++j) {
                ostringstream oss;
                oss << info.label << j;
                path_set.insert("morph " + GetMorphPath(ofr->model_name, oss.str()));
            }
        }
    }
    for (auto& anim_path : anim_paths) {
        ReturnAnimationAssetRef(anim_path.second)->ReturnPaths(path_set);
    }
    for (auto& attack_path : attack_paths) {
        // Attacks::Instance()->ReturnRef(iter->second)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Attack>(attack_path.second)->ReturnPaths(path_set);
    }
    if (!voice_path.empty()) {
        // VoiceFiles::Instance()->ReturnRef(voice_path)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<VoiceFile>(voice_path)->ReturnPaths(path_set);
    }
}

const string& Character::GetFurPath() {
    return fur_path;
}

const string& Character::GetPermanentMorphPath() {
    return morph_path;
}

const string& Character::GetChannel(int which) const {
    if (which < 0 || which > 3) {
        FatalError("Error", "Can only check channels 0-3");
    }
    return channels[which];
}

float Character::GetModelScale() {
    return model_scale;
}

float Character::GetDefaultScale() {
    return default_scale;
}

bool Character::GetMorphMeta(const string& label, vec3& start, vec3& end) {
    for (auto& meta : morph_meta) {
        if (meta.label == label) {
            start = meta.start;
            end = meta.end;
            return true;
        }
    }
    return false;
}

AssetLoaderBase* Character::NewLoader() {
    return new FallbackAssetLoader<Character>();
}
