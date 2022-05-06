//-----------------------------------------------------------------------------
//           Name: material.cpp
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
#include "material.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/soundgroup.h>

#include <Graphics/shaders.h>
#include <XML/xml_helper.h>
#include <Main/engine.h>

#include <tinyxml.h>

Material::Material(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id)
    //context(NULL)
{}

Material::~Material()
{
//    delete context;
}


int Material::Load( const std::string &path, uint32_t load_flags )
{
    //delete context;
    //context = new ASContext();
    //context->LoadScript(path);

    //std::string xml_path = path.substr(0,path.size()-2)+"xml";
    if(path != "Data/Materials/base.xml"){
        //Materials::Instance()->ReturnRef("Data/Materials/base.xml");
        base_ref = Engine::Instance()->GetAssetManager()->LoadSync<Material>("Data/Materials/base.xml");
    }

    TiXmlDocument doc;
    LoadXMLRetryable(doc, path, "Material");
    TiXmlHandle h_doc(&doc);
    TiXmlHandle h_root = h_doc.FirstChildElement();
    TiXmlElement* events = h_root.ToElement()->FirstChild("events")->ToElement();
    while(events){
        const char *mod_cstr = events->Attribute("mod");
        std::string mod;
        if(mod_cstr){
            mod = mod_cstr;
        }
        TiXmlElement* event = events->FirstChildElement();
        for( ; event; event = event->NextSiblingElement()) {
            MaterialEvent me;
            me.max_distance = 0.0f;
            if(event->QueryFloatAttribute("max_distance", &me.max_distance) != TIXML_SUCCESS){
                if(base_ref.valid()){
                    me.max_distance = base_ref->GetEvent(event->Value()).max_distance;
                }
            }
            const char* sg_cstr = event->Attribute("soundgroup");
            if(sg_cstr){
                me.soundgroup = sg_cstr;
            }
            const char* c_str = event->Attribute("attached");
            std::string attached_string;
            if(c_str){
                attached_string = c_str;
            }
            me.attached = (attached_string == "true");
            event_map[mod][event->Value()] = me;
        }
        TiXmlNode *next = h_root.ToElement()->IterateChildren("events", events);
        if(next){
            events = next->ToElement();
        } else {
            events = NULL;
        }
    }

    TiXmlHandle decals_handle = h_root.ToElement()->FirstChild("decals");
    TiXmlElement* decals = decals_handle.ToElement();
    if(decals){
        TiXmlElement* decal = decals->FirstChildElement();
        for( ; decal; decal = decal->NextSiblingElement()) {
            MaterialDecal md;
            md.color_path = decal->Attribute("color");
            md.normal_path = decal->Attribute("normal");
            md.shader = decal->Attribute("shader");
            decal_map[decal->Value()] = md;
        }
    }

    TiXmlHandle particles_handle = h_root.ToElement()->FirstChild("particles");
    TiXmlElement* particles = particles_handle.ToElement();
    if(particles){
        TiXmlElement* particle = particles->FirstChildElement();
        for( ; particle; particle = particle->NextSiblingElement()) {
            MaterialParticle mp;
            mp.particle_path = particle->Attribute("path");
            particle_map[particle->Value()] = mp;
        }
    }

    hardness = 1.0f;
    friction = 1.0f;
    sharp_penetration = 0.0f;
    TiXmlHandle physics_handle = h_root.ToElement()->FirstChild("physics");
    TiXmlElement* physics_el = physics_handle.ToElement();
    if(physics_el){
        physics_el->QueryFloatAttribute("hardness",&hardness);
        physics_el->QueryFloatAttribute("friction",&friction);
        physics_el->QueryFloatAttribute("sharp_penetration",&sharp_penetration);
        //printf("Hardness of %s is %f\n",path.c_str(), hardness);
    } else {
        //printf("Hardness of %s not found, default is %f\n",path.c_str(), 1.0f);
    }

    return kLoadOk;
}

const char* Material::GetLoadErrorString() {
    return "";
}

void Material::Unload() {

}

float Material::GetHardness() const {
    return hardness;
}

void Material::Reload( )
{
    Load(path_,0x0);
}

void Material::ReportLoad()
{
    
}

void Material::HandleEvent( const std::string &the_event, const vec3 &pos )
{
    // Make local copies so they can be passed to Angelscript without const
    std::string event_string = the_event;
    //vec3 event_pos = pos;
    
    //Arglist args;
    //args.AddObject(&event_string);
    //args.AddObject(&event_pos);
    //context->CallScriptFunction("void HandleEvent(string, vec3)", args);
}


const MaterialEvent& Material::GetEvent( const std::string &the_event, const std::string &mod )
{
    if(event_map[mod].find(the_event) != event_map[mod].end()){
        return event_map[mod][the_event];
    } else {
        return event_map[""][the_event];
    }
}

const MaterialEvent& Material::GetEvent( const std::string &the_event )
{
    return event_map[""][the_event];
}

const MaterialDecal& Material::GetDecal( const std::string &type )
{
    return decal_map[type];
}

const MaterialParticle& Material::GetParticle( const std::string &type )
{
    return particle_map[type];
}

float Material::GetFriction() const
{
    return friction;
}

float Material::GetSharpPenetration() const
{
    return sharp_penetration;
}

void Material::ReturnPaths( PathSet & path_set )
{
    path_set.insert("material "+path_);

    for(std::map<std::string, std::map<std::string, MaterialEvent> >::const_iterator iter = event_map.begin();
        iter != event_map.end(); ++iter)
    {
        const std::map<std::string, MaterialEvent> &inner_map = iter->second;
        for(const auto & pair : inner_map)
        {
            const MaterialEvent &me = pair.second;
            if(!me.soundgroup.empty()){
                //SoundGroupInfoCollection::Instance()->ReturnRef(me.soundgroup)->ReturnPaths(path_set);
                Engine::Instance()->GetAssetManager()->LoadSync<SoundGroupInfo>(me.soundgroup)->ReturnPaths(path_set);
            }
        }
    }
    for(std::map<std::string, MaterialDecal >::const_iterator iter = decal_map.begin();
        iter != decal_map.end(); ++iter)
    {
        const MaterialDecal& md = iter->second;
        path_set.insert("texture "+md.color_path);
        path_set.insert("texture "+md.normal_path);
        path_set.insert("shader "+GetShaderPath(md.shader, Shaders::Instance()->shader_dir_path, _vertex));
        path_set.insert("shader "+GetShaderPath(md.shader, Shaders::Instance()->shader_dir_path, _fragment));
    }
    for(std::map<std::string, MaterialParticle >::const_iterator iter = particle_map.begin();
        iter != particle_map.end(); ++iter)
    {
        const MaterialParticle& mp = iter->second;
        if(!mp.particle_path.empty()){ //TODO: Why is this ever empty?
            path_set.insert("particle "+mp.particle_path);
        }
    }
}

AssetLoaderBase* Material::NewLoader() {
    return new FallbackAssetLoader<Material>();
}
