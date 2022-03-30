//-----------------------------------------------------------------------------
//           Name: particletype.cpp
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
#include "particletype.h"

#include <Graphics/particles.h>
#include <Graphics/graphics.h>
#include <Graphics/camera.h>
#include <Graphics/textures.h>
#include <Graphics/camera.h>
#include <Graphics/shaders.h>

#include <Internal/common.h>
#include <Internal/timer.h>

#include <Math/enginemath.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Physics/physics.h>
#include <Physics/bulletworld.h>

#include <Objects/movementobject.h>
#include <Objects/decalobject.h>

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/material.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Online/online_datastructures.h>
#include <XML/xml_helper.h>
#include <Sound/sound.h>
#include <Logging/logdata.h>

#include "tinyxml.h"

#include <assert.h>

ParticleType::ParticleType( AssetManager* owner, uint32_t asset_id ) : Asset( owner, asset_id ) 
{

}

TextureAssetRef LoadXMLTex(TiXmlElement* el, const char* label){
    const char* str = el->Attribute(label);
    if(str){
        return Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(str);
    } else {
        return TextureAssetRef();
    }
}

AnimationEffectRef LoadXMLAERef(TiXmlElement* el, const char* label){
    const char* str = el->Attribute(label);
    if(str){
        //return AnimationEffects::Instance()->ReturnRef(str);
        return Engine::Instance()->GetAssetManager()->LoadSync<AnimationEffect>(str);
    } else {
        return AnimationEffectRef();
    }
}

int ParticleType::Load( const std::string &path, uint32_t load_flags ) {
    sub_error = 0;
    TiXmlDocument doc;

    if(LoadXMLRetryable(doc, path, "Particle"))
    {
        clear();

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement()->FirstChildElement();
        for( ; field; field = field->NextSiblingElement()) {
            std::string field_str(field->Value());
            if(field_str == "textures"){
                ae_ref = LoadXMLAERef(field, "animation_effect");
                color_map = LoadXMLTex(field, "color_map");
                normal_map = LoadXMLTex(field, "normal_map");
                const char* shader_str = field->Attribute("shader");
                const char* soft_shader_str = field->Attribute("soft_shader");
                if(soft_shader_str){
                    FormatString(shader_name, kMaxNameLen, "%s", soft_shader_str);
                } else {
                    FormatString(shader_name, kMaxNameLen, "%s", shader_str);
                }
            } else if(field_str == "size"){
                GetRange(field, "val", "min", "max", size_range[0], size_range[1]);
            }  else if(field_str == "rotation"){
                GetRange(field, "val", "min", "max", rotation_range[0], rotation_range[1]);
            } else if(field_str == "color"){
                GetRange(field, "red", "red_min", "red_max", 
                    color_range[0][0], color_range[1][0]);
                GetRange(field, "green", "green_min", "green_max", 
                    color_range[0][1], color_range[1][1]);
                GetRange(field, "blue", "blue_min", "blue_max", 
                    color_range[0][2], color_range[1][2]);
                GetRange(field, "alpha", "alpha_min", "alpha_max", 
                    color_range[0][3], color_range[1][3]);
            } else if(field_str == "behavior"){
                field->QueryFloatAttribute("inertia", &inertia);
                field->QueryFloatAttribute("gravity", &gravity);
                field->QueryFloatAttribute("wind", &wind);
                field->QueryFloatAttribute("size_decay_rate", &size_decay_rate);
                field->QueryFloatAttribute("opacity_decay_rate", &opacity_decay_rate);
                opacity_ramp_time = 0.0f;
                field->QueryFloatAttribute("opacity_ramp_time", &opacity_ramp_time);
            } else if(field_str == "quadratic_expansion"){
                quadratic_expansion = true;
                field->QueryFloatAttribute("speed", &qe_speed);
            } else if(field_str == "quadratic_dispersion"){
                quadratic_dispersion = true;
                field->QueryFloatAttribute("persistence", &qd_mult);
            } else if(field_str == "stretch"){
                const char* tf;
                tf = field->Attribute("velocity_axis");
                velocity_axis = (tf && strcmp(tf, "true")==0);
                tf = field->Attribute("speed_stretch");
                speed_stretch = (tf && strcmp(tf, "true")==0);
                tf = field->Attribute("min_squash");
                min_squash = (tf && strcmp(tf, "true")==0);
                tf = field->Attribute("speed_mult");
                speed_mult = 1.0f;
                field->QueryFloatAttribute("speed_mult", &speed_mult);
            } else if(field_str == "no_rotation"){
                no_rotation = true;
            } else if(field_str == "collision"){
                collision = true;
                const char* tf;
                tf = field->Attribute("destroy");
                collision_destroy = (tf && strcmp(tf, "true")==0);
                tf = field->Attribute("character_collide");
                character_collide = (tf && strcmp(tf, "true")==0);
                tf = field->Attribute("character_add_blood");
                character_add_blood = (tf && strcmp(tf, "true")==0);
                const char* c_str = field->Attribute("decal");
                if(c_str){
                    collision_decal = c_str;
                }
                c_str = field->Attribute("materialevent");
                if(c_str){
                    collision_event = c_str;
                }
                field->QueryFloatAttribute("decal_size_mult", &collision_decal_size_mult);
            }
        }
        return kLoadOk;
    } else {
        return kLoadErrorMissingFile;
    }
}

const char* ParticleType::GetLoadErrorString() {
    return "";
}

void ParticleType::Unload() {

}

void ParticleType::ReportLoad()
{

}

void ParticleType::clear() {
    size_decay_rate = 1.0f; 
    opacity_decay_rate = 1.0f;
    quadratic_expansion = false;
    qe_speed = 1.0f;
    quadratic_dispersion = false;
    no_rotation = false;
    qd_mult = 1.0f;
    gravity = 0.0f;
    inertia = 1.0f;
    wind = 0.0f;
    velocity_axis = false;
    speed_stretch = false;
    min_squash = false;
    speed_mult = 0.0f;
    collision_decal_size_mult = 1.0f;
    collision_decal.clear();
    collision_event.clear();
    collision_destroy = false;
    collision = false;
    character_collide = false;
    character_add_blood = false;
    rotation_range[0] = -360.0f;
    rotation_range[1] = 360.0f;
    opacity_ramp_time = 0.0f;
}

void ParticleType::Reload() {
    Load(path_,0x0);
}

AssetLoaderBase* ParticleType::NewLoader() {
    return new FallbackAssetLoader<ParticleType>();
}
