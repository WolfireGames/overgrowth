//-----------------------------------------------------------------------------
//           Name: attacks.cpp
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
#include "attacks.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/animation.h>
#include <Asset/Asset/reactions.h>

#include <XML/xml_helper.h>
#include <Math/vec3math.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>

#include <string>

using std::string;

Attack::Attack(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id) {
}

void Attack::clear() {
    unblocked_anim_path.clear();
    blocked_anim_path.clear();
    height = _medium;
    direction = _front;
    swap_stance = false;
    swap_stance_blocked = false;
    reaction.clear();
    impact_dir = vec3(0.0f, 0.0f, 1.0f);
    has_cut_plane = false;
    has_stab_dir = false;
    cut_plane_type = _heavy;
    stab_type = _heavy;
    sharp_damage[0] = 0.0f;
    sharp_damage[1] = 0.0f;
    block_damage[0] = 0.0f;
    block_damage[1] = 0.0f;
    damage[0] = 0.0f;
    damage[1] = 0.0f;
    force[0] = 0.0f;
    force[1] = 0.0f;
    has_range_adjust = false;
    range_adjust = 0.0f;
    unblockable = false;
    flesh_unblockable = false;
    mobile = false;
    materialevent.clear();
}

int Attack::Load(const string& path, uint32_t load_flags) {
    TiXmlDocument doc;
    if (LoadXMLRetryable(doc, path, "Attack")) {
        clear();

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement()->FirstChildElement();
        for (; field; field = field->NextSiblingElement()) {
            string field_str(field->Value());
            if (field_str == "animations") {
                const char* tf;
                tf = field->Attribute("unblocked");
                if (tf) {
                    unblocked_anim_path = tf;
                }
                tf = field->Attribute("blocked");
                if (tf) {
                    blocked_anim_path = tf;
                }
                tf = field->Attribute("throw");
                if (tf) {
                    throw_anim_path = tf;
                }
                tf = field->Attribute("thrown");
                if (tf) {
                    thrown_anim_path = tf;
                }
                tf = field->Attribute("thrown_counter");
                if (tf) {
                    thrown_counter_anim_path = tf;
                }
            } else if (field_str == "flags") {
                const char* tf;
                tf = field->Attribute("swap_stance");
                swap_stance = (tf && strcmp(tf, "true") == 0);
                tf = field->Attribute("swap_stance_blocked");
                swap_stance_blocked = (tf && strcmp(tf, "true") == 0);
                tf = field->Attribute("unblockable");
                unblockable = (tf && strcmp(tf, "true") == 0);
                tf = field->Attribute("flesh_unblockable");
                flesh_unblockable = (tf && strcmp(tf, "true") == 0);
                tf = field->Attribute("as_layer");
                as_layer = (tf && strcmp(tf, "true") == 0);
                tf = field->Attribute("alternate");
                if (tf) {
                    alternate = tf;
                }
                tf = field->Attribute("mobile");
                mobile = (tf && strcmp(tf, "true") == 0);
                tf = field->Attribute("height");
                if (tf) {
                    if (strcmp(tf, "high") == 0) {
                        height = _high;
                    } else if (strcmp(tf, "medium") == 0) {
                        height = _medium;
                    }
                    if (strcmp(tf, "low") == 0) {
                        height = _low;
                    }
                }
                tf = field->Attribute("direction");
                if (tf) {
                    if (strcmp(tf, "right") == 0) {
                        direction = _right;
                    } else if (strcmp(tf, "front") == 0) {
                        direction = _front;
                    }
                    if (strcmp(tf, "left") == 0) {
                        direction = _left;
                    }
                }
                tf = field->Attribute("special");
                if (tf) {
                    special = tf;
                }
            } else if (field_str == "reaction") {
                reaction = field->GetText();
            } else if (field_str == "materialevent") {
                const char* tf;
                tf = field->GetText();
                if (tf) {
                    materialevent = tf;
                }
            } else if (field_str == "impact_dir") {
                field->QueryFloatAttribute("x", &impact_dir[0]);
                field->QueryFloatAttribute("y", &impact_dir[1]);
                field->QueryFloatAttribute("z", &impact_dir[2]);
            } else if (field_str == "cut_plane") {
                has_cut_plane = true;
                field->QueryFloatAttribute("x", &cut_plane[0]);
                field->QueryFloatAttribute("y", &cut_plane[1]);
                field->QueryFloatAttribute("z", &cut_plane[2]);
                const char* tf = field->Attribute("type");
                if (tf && strcmp(tf, "light")) {
                    cut_plane_type = _light;
                } else if (tf && strcmp(tf, "heavy")) {
                    cut_plane_type = _heavy;
                }
                cut_plane = normalize(cut_plane);
            } else if (field_str == "stab_dir") {
                has_stab_dir = true;
                field->QueryFloatAttribute("x", &stab_dir[0]);
                field->QueryFloatAttribute("y", &stab_dir[1]);
                field->QueryFloatAttribute("z", &stab_dir[2]);
                const char* tf = field->Attribute("type");
                if (tf && strcmp(tf, "light")) {
                    stab_type = _light;
                } else if (tf && strcmp(tf, "heavy")) {
                    stab_type = _heavy;
                }
                stab_dir = normalize(stab_dir);
            } else if (field_str == "block_damage") {
                GetRange(field, "val", "min", "max",
                         block_damage[0], block_damage[1]);
            } else if (field_str == "sharp_damage") {
                GetRange(field, "val", "min", "max",
                         sharp_damage[0], sharp_damage[1]);
            } else if (field_str == "damage") {
                GetRange(field, "val", "min", "max", damage[0], damage[1]);
            } else if (field_str == "force") {
                GetRange(field, "val", "min", "max", force[0], force[1]);
            }
            else if (field_str == "range_adjust") 
            {
                has_range_adjust = true;
                field->QueryFloatAttribute("val", &range_adjust);
                //GetRange(field, "val", "min", "max", range_adjust[0], range_adjust[1]);
            }
        }
    } else {
        return kLoadErrorMissingFile;
    }
    return kLoadOk;
}

const char* Attack::GetLoadErrorString() {
    return "";
}

void Attack::Unload() {
}

void Attack::Reload() {
    Load(path_, 0x0);
}

void Attack::ReportLoad() {
}

void Attack::ReturnPaths(PathSet& path_set) {
    path_set.insert("attack " + path_);
    if (!unblocked_anim_path.empty()) {
        ReturnAnimationAssetRef(unblocked_anim_path)->ReturnPaths(path_set);
    }
    if (!blocked_anim_path.empty()) {
        ReturnAnimationAssetRef(blocked_anim_path)->ReturnPaths(path_set);
    }
    if (!throw_anim_path.empty()) {
        ReturnAnimationAssetRef(throw_anim_path)->ReturnPaths(path_set);
    }
    if (!thrown_anim_path.empty()) {
        ReturnAnimationAssetRef(thrown_anim_path)->ReturnPaths(path_set);
    }
    if (!thrown_counter_anim_path.empty()) {
        ReturnAnimationAssetRef(thrown_counter_anim_path)->ReturnPaths(path_set);
    }
    if (!reaction.empty()) {
        // Reactions::Instance()->ReturnRef(reaction)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Reaction>(reaction)->ReturnPaths(path_set);
    }
}

AssetLoaderBase* Attack::NewLoader() {
    return new FallbackAssetLoader<Attack>();
}
