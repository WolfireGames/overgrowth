//-----------------------------------------------------------------------------
//           Name: levelxml.cpp
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
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
#include <Internal/levelxml.h>
#include <Internal/comma_separated_list.h>
#include <Internal/filesystem.h>
#include <Internal/returnpathutil.h>

#include <Asset/Asset/material.h>
#include <Asset/Asset/spawnpointinfo.h>

#include <Logging/logdata.h>
#include <Game/detailobjectlayer.h>
#include <Utility/strings.h>
#include <AI/navmeshparameters.h>

#include <tinyxml.h>

void ExtractLevelName(const std::string& path, std::string &str){
    int slash_position = path.rfind('/')+1;
    int dot_position = path.rfind('.');
    str = path.substr(slash_position, dot_position-slash_position);
}

void GetXMLVersionFromDoc(TiXmlDocument& doc, std::string &str){
    TiXmlHandle hDoc(&doc);
    TiXmlDeclaration* decl = ((hDoc.FirstChild()).ToNode())->ToDeclaration();
    str = decl->Version();
}

void HandleDetailMaps(std::vector<DetailMapInfo>& dmiv, const TiXmlElement* field){
    dmiv.clear();
    while(field) {
        dmiv.resize(dmiv.size()+1);
        DetailMapInfo& dmi = dmiv.back();
        dmi.colorpath = SanitizePath(field->Attribute("colorpath"));
        dmi.normalpath = SanitizePath(field->Attribute("normalpath"));
        const char* material = field->Attribute("materialpath");
        if(material){
            dmi.materialpath = SanitizePath(material);
        } else {
            dmi.materialpath = "Data/Materials/default.xml";
        }
        field = field->NextSiblingElement();
    }
    if(dmiv.size() < 4){
        FatalError("Error", "Detail textures are not specified");
    }
}

void HandleDetailObjects(std::vector<DetailObjectLayer>& dolv, const TiXmlElement* field){
    dolv.clear();
    while(field) {
        dolv.push_back(ReadDetailObjectLayerXML(field));
        field = field->NextSiblingElement();
    }
}


void HandleTerrain(TerrainInfo& ti, const TiXmlElement* old){
    for(const TiXmlElement* field = old; field; field = field->NextSiblingElement()){
        const char* val = field->Value();
        switch(val[0]){
            case 'C': 
                if(strcmp("ColorMap", val) == 0){ti.colormap = SanitizePath(field->GetText());}
                break;
            case 'D':
                switch(val[6]){
                    case 'M':
                        if(strcmp("DetailMaps", val)==0){ // Distinguish from "DetailMap"
                            HandleDetailMaps(ti.detail_map_info, field->FirstChildElement());
                        }
                        break;
                    case 'O': 
                        if(strcmp("DetailObjects", val) == 0){HandleDetailObjects(ti.detail_object_info, field->FirstChildElement());}
                        break;
                }
                break;
            case 'H':
                if(strcmp("Heightmap", val) == 0){ti.heightmap = SanitizePath(field->GetText());}
                break;
            case 'M':
                if(strcmp("ModelOverride", val) == 0){ti.model_override = SanitizePath(field->GetText());}
                break;
            case 'S':
                if(strcmp("ShaderExtra", val) == 0){
                    if(field->GetText() != NULL){
                        ti.shader_extra = field->GetText();
                    } else {
                        ti.shader_extra = "";
                    }
                }
                break;
            case 'W':
                if(strcmp("WeightMap", val) == 0){ti.weightmap = SanitizePath(field->GetText());}
                break;
        }
    }
}


void HandleSky(SkyInfo& si, const TiXmlElement* old){
    for(const TiXmlElement* field = old; field; field = field->NextSiblingElement()){
        const char* val = field->Value();
        switch(val[0]){
            case 'D':
                if(strcmp("DomeTexture", val) == 0){
                    const char *c = field->GetText();
                    if(c){
                        si.dome_texture_path = SanitizePath(c);
                    }
                }
                break;
            case 'R':
                if(strcmp("RayToSun", val) == 0){
                    field->QueryFloatAttribute("r0",&si.ray_to_sun[0]);
                    field->QueryFloatAttribute("r1",&si.ray_to_sun[1]);
                    field->QueryFloatAttribute("r2",&si.ray_to_sun[2]);
                }
                break;
            case 'S': 
                switch(val[1]){
                    case 'u':
                        switch(val[3]){
                            case 'A':
                                if(strcmp("SunAngularRad", val) == 0){si.sun_angular_rad = (float)atof(field->GetText());}
                                break;
                            case 'C':
                                if(strcmp("SunColorAngle", val) == 0){si.sun_color_angle = (float)atof(field->GetText());}
                                break;
                        }
                        break;
                }
                break;
        }
    }
}

bool GetBoolFieldAttribute(const TiXmlElement* field, const std::string& attr){
    const char* cstr = field->Attribute(attr.c_str());
    if(cstr && strcmp(cstr,"false") == 0){
        return false;
    }
    return true;
}

int GetIntFieldAttribute(const TiXmlElement* field, const std::string& attr){
    int v = 0;
    field->QueryIntAttribute(attr.c_str(),&v);
    return v;
}

void HandleOutOfDate(OutOfDateInfo& oodi, const TiXmlElement* field){
    oodi.shadow = GetBoolFieldAttribute(field, "Shadow");
    oodi.ao = GetBoolFieldAttribute(field, "AO");
    oodi.nav_mesh = GetBoolFieldAttribute(field, "NavMesh");
    oodi.nav_mesh_param_hash = GetIntFieldAttribute(field, "NavMeshParamHash");
}

void HandleAmbientSounds(std::vector<std::string> &as, const TiXmlElement* old){
    for(const TiXmlElement* field = old; field; field = field->NextSiblingElement()){
        as.push_back(SanitizePath(field->Attribute("path")));
    }
}

void ParseElementTextIntoVector(std::vector<std::string> &vec, const TiXmlElement* field){
    const char* text = field->GetText();
    if(!text){
        return;
    }
    CSLIterator iter(text);
    std::string next_token;
    while(iter.GetNext(&next_token)){
        vec.push_back(next_token);
    }
}

void HandleSpawnPoints(SpawnPointInfo& spi, const TiXmlElement* el) {
    GetTSRinfo(el, spi.translation, spi.scale, spi.rotation);
}

void HandleRecentItems(std::vector<SpawnerItem>& siv, const TiXmlElement* el) {
    const TiXmlElement* spawner = el->FirstChildElement("SpawnerItem");

    while( spawner != NULL ) {
        SpawnerItem si;
        si.display_name     = spawner->Attribute("display_name");
        si.path             = SanitizePath(spawner->Attribute("path"));
        si.thumbnail_path   = SanitizePath(spawner->Attribute("thumbnail_path"));
        siv.push_back(si);

        spawner             = spawner->NextSiblingElement("SpawnerItem");
    }
}

void HandleLoadingScreen(LoadingScreen& siv, const TiXmlElement* el) {
    const TiXmlElement* image = el->FirstChildElement("Image");
    if( image ) {
        const char* c_image = image->GetText();
        if( c_image ) {
            siv.image = c_image;  
        }
    }
}

void ParseLevelXML(const std::string &path, LevelInfo &li) {
    for(int i=0, len=path.size(); i<len; ++i) {
        if(path[i] == '\\') {
            std::stringstream ss;
            ss << "Path to ParseLevelXML should not contain \\ \"" << path.c_str() << "\"";
            DisplayError("Error", ss.str().c_str());
        }
    }

    if(!CheckFileAccess(path.c_str())) {
        FatalError("Error", "Could not find level file: %s", path.c_str());
    }

    TiXmlDocument doc;
    if (!doc.LoadFile(path.c_str())) {
        FatalError("Error", "Bad xml data in level file %s\n%s on row %d", path.c_str(), doc.ErrorDesc(), doc.ErrorRow());
    }

    li.SetDefaults();

    GetXMLVersionFromDoc(doc, li.xml_version_);
    ExtractLevelName(path, li.level_name_);
    li.path_ = path;

    const TiXmlElement* root = doc.RootElement();

    //Check if this is the newer level format with the "correct" Level root node
    //If it is, reassign, otherwise iterator on the "bottom" of the document.
    //LOGW << "test:" << root->Value() << std::endl;
    if(root && strcmp(root->Value(), "Level") == 0 ) {
        root = root->FirstChildElement();
    }

    if( !root ) {
        LOGE << "Level Root is null!" << std::endl;
    }
     
    for(const TiXmlElement* field = root; field; field = field->NextSiblingElement()){
        const char* val = field->Value();
        switch(val[0]){
            case 'A':
                switch(val[1]){
                    case 'm'://AmbientSounds
                        if(strcmp("AmbientSounds", val) == 0){HandleAmbientSounds(li.ambient_sounds_, field->FirstChildElement());}
                        break;
                    case 'c':
                        switch(val[2]){
                            case 't'://ActorObjects
                                if(strcmp("ActorObjects", val) == 0){LoadEntityDescriptionListFromXML(li.desc_list_, field);}
                                break;
                            case 'h'://Achievements
                                if(strcmp("Achievements", val) == 0){
                                    if(field->GetText()){
                                        ScriptParam sp;
                                        sp.SetString(field->GetText());
                                        li.spm_["Achievements"] = sp;
                                    }
                                }
                                break;
                        }
                        break;
                }
                break;
            case 'D':
                switch(val[2]){
                    case 'c': //Decals
                        if(strcmp("Decals", val) == 0){LoadEntityDescriptionListFromXML(li.desc_list_, field);}
                        break;
                    case 's': //Description;
                        if(strcmp("Description", val) == 0){if(field->GetText()){li.visible_description_ = field->GetText();}}
                        break;
                }
                break;
            case 'E':
                if(strcmp("EnvObjects", val) == 0){
                    LoadEntityDescriptionListFromXML(li.desc_list_, field);
                }
                break;
            case 'G'://Groups
                if(strcmp("Groups", val) == 0){LoadEntityDescriptionListFromXML(li.desc_list_, field);}
                break;
            case 'H'://Hotspots
                if(strcmp("Hotspots", val) == 0){LoadEntityDescriptionListFromXML(li.desc_list_, field);}
                break;
            case 'L'://LevelScriptParameters
                if(strcmp("LevelScriptParameters", val) == 0){
                    ReadScriptParametersFromXML(li.spm_, field);
                }
                if(strcmp("LoadingScreen", val) == 0){
                    HandleLoadingScreen(li.loading_screen_,field); 
                }
                break;
            case 'N':
                if(strmtch("NavMeshParameters", val)) {
                    ReadNavMeshParametersFromXML(li.nav_mesh_parameters_, field);
                } else if(strmtch("Name", val)) {
                    if(field->GetText()) {
                        li.visible_name_ = field->GetText();
                    }
                } else if(strcmp("NPCScript", val) == 0) {
                    const char* txt = field->GetText();
                    if(txt) {
                        li.npc_script_ = txt;
                    } else {
                        li.npc_script_ = "";
                    }
                }
                break;
            case 'O':
                switch(val[1]){
                    case 'u'://OutOfDate
                        if(strcmp("OutOfDate", val) == 0){HandleOutOfDate(li.out_of_date_info_, field);}
                        break;
                    case 'b'://Objectives
                        if(strcmp("Objectives", val) == 0){
                            if(field->GetText()){
                                ScriptParam sp;
                                sp.SetString(field->GetText());
                                li.spm_["Objectives"] = sp;
                            }
                        }
                        break;
                }
                break;
            case 'P':
                if(strcmp("PCScript", val) == 0){
                    const char* txt = field->GetText(); 
                    if(txt){
                        li.pc_script_ = txt;
                    } else {
                        li.pc_script_ = "";
                    }
                }
                break;
            case 'R':
                if(strcmp("RecentItems", val) == 0){ HandleRecentItems(li.recently_created_items_, field); }
                break;
            case 'S':
                switch(val[1]){
                    case 'h'://Shader
                        if(strcmp("Shader", val) == 0){li.shader_ = field->GetText();}
                        else if(strcmp("Shadows", val) == 0){li.shadows_ = (strcmp(field->GetText(), "1") == 0 || strcmp(field->GetText(), "true") == 0);}
                        break;
                    case 'c'://Shader
                        if(strcmp("Script", val) == 0){
                            const char* txt = field->GetText(); 
                            if(txt){
                                li.script_ = txt;
                            } else {
                                li.script_ = "";
                            }
                        }
                        break;
                    case 'k'://Sky
                        if(strcmp("Sky", val) == 0){HandleSky(li.sky_info_, field->FirstChildElement());}
                        break;
                    case 'p'://SpawnPoints
                        LOGW << "SpawnPoints is outdated" << std::endl;
                        break;
                }
                break;
            case 'T':
                switch(val[1]){
                    case 'e'://Terrain
                        if(strcmp("Terrain", val) == 0){HandleTerrain(li.terrain_info_, field->FirstChildElement());}
                        break;
                    case 'y'://Type
                        break; 
                }         
                break;              
        }
    }

    if( li.out_of_date_info_.nav_mesh_param_hash != (int)HashNavMeshParameters(li.nav_mesh_parameters_) ) {
        LOGW << "Mismatch between navmesh build data and navmesh level parameters" << std::endl;
        li.out_of_date_info_.nav_mesh = true;
    }
}
