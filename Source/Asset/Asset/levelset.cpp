//-----------------------------------------------------------------------------
//           Name: levelset.cpp
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
#include "levelset.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <XML/xml_helper.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

LevelSet::LevelSet( AssetManager* owner, uint32_t asset_id ) : AssetInfo( owner, asset_id ), sub_error(0) {
}

void LevelSet::clear() {
    level_paths_.clear();
}

int LevelSet::Load( const std::string &path, uint32_t load_flags ) {
    sub_error = 0;
    TiXmlDocument doc;
    if( LoadXMLRetryable(doc, path, "LevelSet") )
    {
        clear();

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement("LevelSet").FirstChildElement("Levels").FirstChildElement("Level");
        TiXmlElement* field = h_root.ToElement();
        for( ; field; field = field->NextSiblingElement()) {
            std::string field_str(field->Value());
            if(field_str == "Level"){
                const char* path;
                path = field->Attribute("path");
                if(path){
                    level_paths_.push_back(path);
                }
            }
        }
    }
    else
    {
        return kLoadErrorMissingFile;
    }
    return kLoadOk;
}

const char* LevelSet::GetLoadErrorString() {
    switch(sub_error) {
        case 0: return "";
        default: return "Undefined error";
    }
}

void LevelSet::Unload() {

}

void LevelSet::Reload() {
    Load(path_,0x0);
}

void LevelSet::ReportLoad() {

}

void LevelSet::ReturnPaths( PathSet &path_set ) {
    for(std::list<std::string>::iterator iter = level_paths_.begin(); 
        iter != level_paths_.end(); ++iter)
    {
        path_set.insert("level "+(*iter));
    }
}

AssetLoaderBase* LevelSet::NewLoader() {
    return new FallbackAssetLoader<LevelSet>();
}
