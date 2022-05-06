//-----------------------------------------------------------------------------
//           Name: item.cpp
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
#include "item.h"

#include <Asset/Asset/animation.h>
#include <Asset/Asset/attacks.h>
#include <Asset/Asset/reactions.h>
#include <Asset/Asset/objectfile.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <XML/xml_helper.h>
#include <Graphics/animationflags.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>

#include <algorithm>

Item::Item( AssetManager* owner, uint32_t asset_id ) : AssetInfo( owner, asset_id ), sub_error(0) {

}

void Item::Reload() {
    Load(path_,0x0);
}

int Item::Load( const std::string &path, uint32_t load_flags ) {
    TiXmlDocument doc;

    if( LoadXMLRetryable(doc, path, "Item") ) {
        obj_path.clear();
        type = _item_no_type;
        mass = 1.0f;
        range_extender = 0.0f;
        range_multiplier = 1.0f;
        points.clear();
        hands = 1;
        sound_modifier_.clear();

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement()->FirstChildElement();
        for( ; field; field = field->NextSiblingElement()) {
            std::string field_str(field->Value());
            switch(field_str[0]){
                case 'a':
                    switch(field_str[1]){
                        case 'n':
                            if(field_str == "anim_override_flags"){
                                TiXmlAttribute* attrib = field->FirstAttribute();
                                while(attrib){
                                    std::string val = attrib->Value();
                                    char flags = 0;
                                    if(val == "mirror"){
                                        flags = _ANM_MIRRORED;
                                    }
                                    anim_override_flags[attrib->Name()] = flags;
                                    attrib = attrib->Next();
                                }
                            } else if(field_str == "anim_blend"){
                                TiXmlAttribute* attrib = field->FirstAttribute();
                                while(attrib){
                                    anim_blend[attrib->Name()] = attrib->Value();
                                    attrib = attrib->Next();
                                }
                            } else if(field_str == "anim_override"){
                                TiXmlAttribute* attrib = field->FirstAttribute();
                                while(attrib){
                                    anim_overrides[attrib->Name()] = attrib->Value();
                                    attrib = attrib->Next();
                                }
                            }
                            break;
                        case 'p':
                            if(field_str == "appearance"){
                                obj_path = field->Attribute("obj_path");
                            }
                            break;
                        case 't':
                            if(field_str == "attack_override"){
                                TiXmlAttribute* attrib = field->FirstAttribute();
                                while(attrib){
                                    attack_overrides[attrib->Name()] = attrib->Value();
                                    attrib = attrib->Next();
                                }
                            } else if(field_str == "attachments"){
                                TiXmlElement* attachment_el = field->FirstChildElement();
                                while(attachment_el){
                                    attachments.push_back(attachment_el->GetText());
                                    attachment_el = attachment_el->NextSiblingElement();
                                }
                            }
                            break;
                    }
                    break;
                case 'g':
                    if(field_str == "grip"){
                        attachment[_at_grip].ik_label = field->Attribute("ik_attach");
                        attachment[_at_grip].anim = field->Attribute("anim");
                        attachment[_at_grip].exists = true;
                        const char *str = field->Attribute("anim_base");
                        if(str){
                            attachment[_at_grip].anim_base = str;
                        }
                        field->QueryIntAttribute("hands", &hands);
                    } 
                    break;
                case 'l':
                    if(field_str == "lines"){
                        TiXmlElement* line_field = field->FirstChildElement();
                        for( ; line_field; line_field = line_field->NextSiblingElement()) {
                            std::string material(line_field->Value());
                            std::string start(line_field->Attribute("start"));
                            std::string end(line_field->Attribute("end"));
                            lines.resize(lines.size()+1);
                            WeaponLine &line = lines.back();
                            line.material = material;
                            line.start = start;
                            line.end = end;
                        }
                    } else if(field_str == "label"){
                        const char* label_cstr = field->GetText();
                        if(label_cstr){
                            label = field->GetText();
                        }
                    }
                    break;
                case 'p':
                    if(field_str == "points"){
                        TiXmlElement* point_field = field->FirstChildElement();
                        for( ; point_field; point_field = point_field->NextSiblingElement()) {
                            std::string point_name(point_field->Value());
                            vec3 point;
                            point_field->QueryFloatAttribute("x", &point[0]);
                            point_field->QueryFloatAttribute("y", &point[1]);
                            point_field->QueryFloatAttribute("z", &point[2]);
                            points.insert(std::pair<std::string, vec3>(point_name, point));
                        }
                    } else if(field_str == "physics"){
                        const char *sound_modifier_cstring = field->Attribute("sound_modifier");
                        if(sound_modifier_cstring){
                            sound_modifier_ = sound_modifier_cstring;
                        }
                        std::string mass_str = field->Attribute("mass");
                        std::transform(mass_str.begin(),
                            mass_str.end(), 
                            mass_str.begin(), 
                            tolower);
                        size_t first_space = mass_str.find(' ');
                        std::string num_str;
                        if(first_space != std::string::npos){
                            num_str = mass_str.substr(0,first_space);
                        } else {
                            num_str = mass_str;
                        }
                        std::string type_str;
                        float convert_mult = 1.0f;
                        if(first_space != std::string::npos){
                            type_str = mass_str.substr(first_space+1);
                            if(type_str[0] == 'l'){
                                const float _lb_to_kg = 0.454f;
                                convert_mult = _lb_to_kg;
                            }
                        }
                        mass = (float)atof(num_str.c_str()) * convert_mult;
                    }
                    break;
                case 'r':
                    if(field_str == "range"){
                        field->QueryFloatAttribute("extend", &range_extender);
                        field->QueryFloatAttribute("multiply", &range_multiplier);
                    } else if(field_str == "reaction_override"){
                        TiXmlElement* child = field->FirstChildElement();
                        for( ; child; child = child->NextSiblingElement()) {
                            TiXmlAttribute* attrib = child->FirstAttribute();
                            std::string old_path, new_path;
                            while(attrib){
                                if(strcmp(attrib->Name(), "old") == 0){
                                    old_path = attrib->Value();
                                } else if(strcmp(attrib->Name(), "new") == 0){
                                    new_path = attrib->Value();   
                                }
                                attrib = attrib->Next();
                            }
                            reaction_overrides[old_path] = new_path;
                        }
                    }
                    break;
                case 's':
                    if(field_str == "sheathe"){
                        attachment[_at_sheathe].ik_label = field->Attribute("ik_attach");
                        attachment[_at_sheathe].anim = field->Attribute("anim");
                        const char *str2 = field->Attribute("contains");
                        if(str2){
                            contains = str2;            
                        }
                        attachment[_at_sheathe].exists = true;
                        const char *str = field->Attribute("anim_base");
                        if(str){
                            attachment[_at_sheathe].anim_base = str;
                        }
                    }
                    break; 
                case 't': 
                    if(field_str == "type"){
                        if(strcmp(field->GetText(),"weapon")==0){
                            type = _weapon;    
                        } else if(strcmp(field->GetText(),"misc")==0){
                            type = _misc;    
                        } else if(strcmp(field->GetText(),"collectable")==0){
                            type = _collectable;    
                        }
                    } 
                    break;
            }
        }
        return kLoadOk;
    } else {
        return kLoadErrorMissingFile;
    }
}

const char* Item::GetLoadErrorString() {
    switch(sub_error) {
        case 0: return "";
        default: return "Unknown error";
    }
}

void Item::Unload() {

}

void Item::ReportLoad()
{

}

const std::string & Item::GetObjPath() {
    return obj_path;
}

ItemType Item::GetItemType() {
    return type;
}

int Item::GetNumHands() {
    return hands;
}

bool Item::HasAttachment(AttachmentType type)
{
    return attachment[type].exists;
}

const std::string & Item::GetIKAttach(AttachmentType type) const
{
    return attachment[type].ik_label;
}

const std::string & Item::GetContains() const
{
    return contains;
}

const std::string & Item::GetAttachAnimPath(AttachmentType type)
{
    return attachment[type].anim;
}

float Item::GetMass()
{
    return mass;
}

const std::string & Item::GetAnimOverride( const std::string &anim )
{
    return anim_overrides[anim];
}

bool Item::HasAnimOverride( const std::string &anim )
{
    return anim_overrides.find(anim) != anim_overrides.end();
}

const std::string & Item::GetAnimBlend( const std::string &anim )
{
    return anim_blend[anim];
}

bool Item::HasAnimBlend( const std::string &anim )
{
    return anim_blend.find(anim) != anim_blend.end();
}

bool Item::HasAttackOverride( const std::string &anim )
{
    return attack_overrides.find(anim) != attack_overrides.end();
}

bool Item::HasReactionOverride( const std::string &anim )
{
    return reaction_overrides.find(anim) != reaction_overrides.end();
}

const std::string & Item::GetAttackOverride( const std::string &anim )
{
    return attack_overrides[anim];
}

const std::string & Item::GetReactionOverride( const std::string &anim )
{
    return reaction_overrides[anim];
}

float Item::GetRangeExtender()
{
    return range_extender;
}

float Item::GetRangeMultiplier()
{
    return range_multiplier;
}

const std::string &Item::GetAttachAnimBasePath(AttachmentType type)
{
    return attachment[type].anim_base;
}

vec3 Item::GetPoint( const std::string &name )
{
    std::map<std::string, vec3>::const_iterator iter;
    iter = points.find(name);
    if(iter != points.end()){
        return iter->second;
    } else {
        return vec3(0.0f,0.0f,0.0f);
    }
}

size_t Item::GetNumLines()
{
    return lines.size();
}

const vec3& Item::GetLineStart( int which )
{
    return points[lines[which].start];
}

const vec3& Item::GetLineEnd( int which )
{
    return points[lines[which].end];
}

const std::string& Item::GetLineMaterial( int which )
{
    return lines[which].material;
}

char Item::GetAnimOverrideFlags( const std::string &anim )
{
    std::map<std::string, char>::const_iterator iter = anim_override_flags.find(anim);
    if(iter != anim_override_flags.end()){
        return anim_override_flags[anim];
    } else {
        return 0;
    }
}

void Item::ReturnPaths( PathSet &path_set )
{
    path_set.insert("item "+path_);
    for(auto & i : attachment){
        if(i.exists){
            ReturnAnimationAssetRef(i.anim)->ReturnPaths(path_set);
            if(!i.anim_base.empty()){
                ReturnAnimationAssetRef(i.anim_base)->ReturnPaths(path_set);
            }
        }
    }
    //ObjectFiles::Instance()->ReturnRef(obj_path)->ReturnPaths(path_set);
    Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(obj_path)->ReturnPaths(path_set); 
    for(auto & anim_override : anim_overrides)
    {
        ReturnAnimationAssetRef(anim_override.second)->ReturnPaths(path_set);
    }
    for(auto & iter : anim_blend)
    {
        ReturnAnimationAssetRef(iter.second)->ReturnPaths(path_set);
    }
    for(auto & attack_override : attack_overrides)
    {
        //Attacks::Instance()->ReturnRef(iter->second)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Attack>(attack_override.second)->ReturnPaths(path_set);
    }
    for(auto & reaction_override : reaction_overrides)
    {
        //Reactions::Instance()->ReturnRef(iter->second)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Reaction>(reaction_override.second)->ReturnPaths(path_set);
    }
}

const std::string& Item::GetSoundModifier() {
    return sound_modifier_;
}

const std::list<std::string> & Item::GetAttachmentList() {
    return attachments;
}

const std::string & Item::GetLabel() {
    return label;
}

AssetLoaderBase* Item::NewLoader() {
    return new FallbackAssetLoader<Item>();
}

ItemAttachment::ItemAttachment():
    exists(false)
{

}
