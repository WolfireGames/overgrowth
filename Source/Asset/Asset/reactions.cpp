//-----------------------------------------------------------------------------
//           Name: reactions.cpp
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
#include "reactions.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/animation.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>
#include <Internal/error.h>

#include <tinyxml.h>

Reaction::Reaction( AssetManager* owner, uint32_t asset_id ) : AssetInfo( owner, asset_id ), sub_error(0) {
}

void Reaction::Reload() {
    Load(path_,0x0);
}

void Reaction::ReportLoad() {

}

int Reaction::Load( const std::string &path, uint32_t load_flags ) {
    sub_error = 0;
    TiXmlDocument doc;

    if( LoadXMLRetryable(doc, path, "Reaction") ) {
        anim_paths.clear();
        mirrored = 0;

        TiXmlHandle h_doc(&doc);
        TiXmlElement* root = h_doc.FirstChildElement().ToElement();
        std::string label = root->Value();
        if(label != "reaction"){
            FatalError("Error", "Reaction xml has incorrect type: %s", label.c_str());
        }
        TiXmlElement* field = root->FirstChildElement();
        for( ; field; field = field->NextSiblingElement()) {
            std::string field_str(field->Value());
            if(field_str == "reaction"){
                anim_paths.push_back(field->GetText());
            } else if(field_str == "flags"){
                const char* tf = field->Attribute("mirrored");
                if(tf && strcmp(tf, "true")==0){
                    mirrored = 1;
                } else if(tf && strcmp(tf, "maybe")==0){
                    mirrored = 2;
                }
            }
        }
    } else {
        return kLoadErrorMissingFile;
    }
    
    return kLoadOk;
}

const char* Reaction::GetLoadErrorString() {
    switch(sub_error) { 
        case 0: return "";
        default: return "Undefined error";
    }
}

void Reaction::Unload() {

}

const std::string &Reaction::GetAnimPath( float severity )
{
    float choose_val = severity * 0.999f * anim_paths.size(); 
    int choice = (int)choose_val;
    return anim_paths[choice];
}

int Reaction::IsMirrored()
{
    return mirrored;
}

void Reaction::ReturnPaths( PathSet &path_set )
{
    path_set.insert("reaction "+path_);
    for(unsigned i=0; i<anim_paths.size(); ++i){
        ReturnAnimationAssetRef(anim_paths[i]);
    }
}

AssetLoaderBase* Reaction::NewLoader() {
    return new FallbackAssetLoader<Reaction>();
}
