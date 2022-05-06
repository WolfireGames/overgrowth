//-----------------------------------------------------------------------------
//           Name: objectfile.cpp
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
#include "objectfile.h"

#include <Asset/Asset/character.h>
#include <Asset/Asset/material.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <XML/xml_helper.h>
#include <Internal/filesystem.h>
#include <Graphics/shaders.h>
#include <Scripting/scriptfile.h>
#include <Logging/logdata.h>
#include <Game/detailobjectlayer.h>
#include <Main/engine.h>

#include <tinyxml.h>

#include <cmath>
#include <map>
#include <string>

ObjectFile::ObjectFile(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner,asset_id), sub_error(0) {
    
}

int ObjectFile::Load( const std::string &rel_path, uint32_t load_flags ) {
    sub_error = 0;
    TiXmlDocument doc;

    char abs_path[kPathSize];
    ModID modsource;
	if(FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath, true, NULL, &modsource) == -1){
        return kLoadErrorMissingFile;
    } else {
        modsource_ = modsource;
        doc = TiXmlDocument(abs_path);
    }

    doc.LoadFile();
    if( doc.Error() ) {
        return kLoadErrorCouldNotOpenXML;
    }

    if (!XmlHelper::getNodeValue(doc, "Object/Model", model_name)) {
        sub_error = 1;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "Object/ColorMap", color_map)) {
        sub_error = 2;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "Object/NormalMap", normal_map)) {
        sub_error = 3;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "Object/ShaderName", shader_name)) {
        sub_error = 4;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "Object/MaterialPath", material_path)) {
        material_path = "Data/Materials/default.xml";
    }
    if (!XmlHelper::getNodeValue(doc, "Object/WeightMap", weight_map)) {
    }
    if (!XmlHelper::getNodeValue(doc, "Object/GroundOffset", ground_offset)) {
        ground_offset = 0.0f;
    }

    TiXmlHandle hDoc(&doc);
    TiXmlElement* palette_map_element = hDoc.FirstChildElement("Object").
        FirstChildElement("PaletteMap").Element();
    if (palette_map_element) {
        palette_map_path = palette_map_element->GetText();
        LoadAttribIntoString(palette_map_element, "label_red", palette_label[_pm_red]);
        LoadAttribIntoString(palette_map_element, "label_green", palette_label[_pm_green]);
        LoadAttribIntoString(palette_map_element, "label_blue", palette_label[_pm_blue]);
        LoadAttribIntoString(palette_map_element, "label_alpha", palette_label[_pm_alpha]);
        LoadAttribIntoString(palette_map_element, "label_other", palette_label[_pm_other]);
    }

    TiXmlElement* detail_map_element = hDoc.FirstChildElement("Object").
        FirstChildElement("DetailMaps").
        FirstChildElement("DetailMap").
        Element();
    while(detail_map_element) {
        m_detail_color_maps.push_back(detail_map_element->Attribute("colorpath"));
        m_detail_normal_maps.push_back(detail_map_element->Attribute("normalpath"));
        const char* material = detail_map_element->Attribute("materialpath");
        m_detail_map_scale.push_back(1.0f);
        detail_map_element->QueryFloatAttribute("scale",&(m_detail_map_scale.back()));
        if(material){
            m_detail_materials.push_back(material);
        } else {
            m_detail_materials.push_back("Data/Materials/default.xml");
        }
        detail_map_element=detail_map_element->NextSiblingElement();
    }

    TiXmlElement* detail_object_element = hDoc.FirstChildElement("Object").
        FirstChildElement("DetailObjects").
        FirstChildElement("DetailObject").
        Element();
    while(detail_object_element) {
        m_detail_object_layers.push_back(ReadDetailObjectLayerXML(detail_object_element));
        detail_object_element=detail_object_element->NextSiblingElement();
    }

    transparent = false;
    bush_collision = false;
    no_collision = false;
    terrain_fixed = false;
    double_sided = false;
    clamp_texture = false;
    dynamic = false;

    TiXmlElement* flags = hDoc.FirstChildElement("Object").
        FirstChildElement("flags").Element();
    if(flags){
        const char* tf;
        tf = flags->Attribute("transparent");
        if(tf && (strcmp(tf, "true") == 0)){
            transparent = true;
        }
        tf = flags->Attribute("no_collision");
        if(tf && (strcmp(tf, "true") == 0)){
            no_collision = true;
        }
        tf = flags->Attribute("clamp_texture");
        if(tf && (strcmp(tf, "true") == 0)){
            clamp_texture = true;
        }
        tf = flags->Attribute("bush_collision");
        if(tf && (strcmp(tf, "true") == 0)){
            bush_collision = true;
        }
        tf = flags->Attribute("terrain_fixed");
        if(tf && strcmp(tf, "true") == 0){
            terrain_fixed = true;
        }
        tf = flags->Attribute("dynamic");
        if(tf && strcmp(tf, "true") == 0){
            dynamic = true;
        }
        tf = flags->Attribute("double_sided");
        if(tf && strcmp(tf, "true") == 0){
            double_sided = true;
            LOGI << rel_path << " is double sided!" << std::endl;
        }
    }

	TiXmlElement* labelElem = hDoc.FirstChildElement("Object").
        FirstChildElement("label").Element();
	if(labelElem){
		const char* label_cstr = labelElem->GetText();
        if(label_cstr){
            label = labelElem->GetText();
        }
	}

    color_tint = vec3(1.0f);

    TiXmlElement* ct = hDoc.FirstChildElement("Object").
        FirstChildElement("ColorTint").Element();
    if(ct){
        ct->QueryFloatAttribute("r", &color_tint[0]);
        ct->QueryFloatAttribute("g", &color_tint[1]);
        ct->QueryFloatAttribute("b", &color_tint[2]);
    }

    TiXmlElement* ac = hDoc.FirstChildElement("Object").
        FirstChildElement("avg_color").Element();
    if(ac){
        avg_color.resize(3);
        avg_color_srgb.resize(3);
        ac->QueryIntAttribute("r", &avg_color[0]);
        ac->QueryIntAttribute("g", &avg_color[1]);
        ac->QueryIntAttribute("b", &avg_color[2]);
        avg_color_srgb[0] = (int)(pow(avg_color[0]/255.0f,2.2f)*255);
        avg_color_srgb[1] = (int)(pow(avg_color[1]/255.0f,2.2f)*255);
        avg_color_srgb[2] = (int)(pow(avg_color[2]/255.0f,2.2f)*255);
    } else {
        avg_color.resize(3, 200);
        avg_color_srgb.resize(3, 150);
    }

    XmlHelper::getNodeValue(doc, "Object/TranslucencyMap", translucency_map);
    XmlHelper::getNodeValue(doc, "Object/WindMap", wind_map);
    XmlHelper::getNodeValue(doc, "Object/SharpnessMap", sharpness_map);

	if(clamp_texture){
		Textures::Instance()->setWrap(GL_CLAMP_TO_EDGE);
	} else {
		Textures::Instance()->setWrap(GL_REPEAT);
	}
    color_map_texture = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(color_map, PX_SRGB, 0x0);
    if( color_map_texture.valid() == false ) {
        return kLoadErrorMissingSubFile;
    }

    normal_map_texture = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(normal_map);
    if( normal_map_texture.valid() == false ) {
        return kLoadErrorMissingSubFile;
    }

    if( translucency_map.empty() == false ) {
        translucency_map_texture = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(translucency_map);
        if( translucency_map_texture.valid() == false ) {
            return kLoadErrorMissingSubFile;
        }
    }

    return kLoadOk;
}

const char* ObjectFile::GetLoadErrorString() {
    switch(sub_error) {
        case 0: return "";
        case 1: return "Model name not found. Aborting object load.";
        case 2: return "Color map not found. Aborting object load.";
        case 3: return "Normal map not found. Aborting object load.";
        case 4: return "Shader name not found. Aborting object load.";
        default: return "Undefined error";
    }
}

void ObjectFile::Unload() {

}

void ObjectFile::Reload() {
    Load(path_,0x0);
}

void ObjectFile::ReportLoad() {

}

void ObjectFile::ReturnPaths( PathSet& path_set )
{
    path_set.insert("object "+path_);
    if(!model_name.empty()){
        path_set.insert("model "+model_name);
    }
    if(!color_map.empty()){
        path_set.insert("texture "+color_map);
    }
    if(!normal_map.empty()){
        path_set.insert("texture "+normal_map);
    }
    if(!translucency_map.empty()){
        path_set.insert("texture "+translucency_map);
    }
    if(!shader_name.empty()){
        path_set.insert("shader "+GetShaderPath(shader_name, Shaders::Instance()->shader_dir_path, _vertex));
        path_set.insert("shader "+GetShaderPath(shader_name, Shaders::Instance()->shader_dir_path, _fragment));
    }
    if(!wind_map.empty()){
        path_set.insert("image_sample "+wind_map);
    }
    if(!sharpness_map.empty()){
        path_set.insert("image_sample "+sharpness_map);
    }
    if(!material_path.empty()){
        //Materials::Instance()->ReturnRef(material_path)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Material>(material_path)->ReturnPaths(path_set); 
    }
    if(!weight_map.empty()){
        path_set.insert("texture "+weight_map);
    }
    if(!palette_map_path.empty()){
        path_set.insert("texture "+palette_map_path);
    }
    for(auto & detail_color_map : m_detail_color_maps){
        path_set.insert("texture "+detail_color_map);
    }
    for(auto & detail_normal_map : m_detail_normal_maps){
        path_set.insert("texture "+detail_normal_map);
    }
    for(auto & detail_material : m_detail_materials){
        //Materials::Instance()->ReturnRef(m_detail_materials[i])->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<Material>(detail_material)->ReturnPaths(path_set);
    }
    for(auto & detail_object_layer : m_detail_object_layers){
        detail_object_layer.ReturnPaths(path_set);
    }
}

AssetLoaderBase* ObjectFile::NewLoader() {
    return new FallbackAssetLoader<ObjectFile>();
}
