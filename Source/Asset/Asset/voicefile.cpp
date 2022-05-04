//-----------------------------------------------------------------------------
//           Name: voicefile.cpp
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
#include "voicefile.h"

#include <Asset/Asset/soundgroup.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>

VoiceFile::VoiceFile(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner,asset_id) {

}

void VoiceFile::Reload() {
    Load(path_,0x0);
}

void VoiceFile::ReportLoad() {

}

int VoiceFile::Load( const std::string &path, uint32_t load_flags ) {
    TiXmlDocument doc;
    if( LoadXML(doc, path, "Voice") ) {
        if( doc.Error() == false ) { 
            voice_paths.clear();
            TiXmlHandle h_doc(&doc);
            TiXmlHandle h_root = h_doc.FirstChildElement();
            if( h_root.ToElement() ) {
                TiXmlElement* field = h_root.ToElement()->FirstChildElement();
                for( ; field; field = field->NextSiblingElement()) {
                    std::string field_str(field->Value());
                    voice_paths[field_str] = field->Attribute("path");
                }
            } else {
                LOGE << "Error reading data out of XML state, missing root element in: " << path << std::endl;
                return kLoadErrorCorruptFile;
            }
        } else {
            LOGE << "Error parsing VoiceFile: " << path << ". Parser error: " << doc.ErrorDesc() << std::endl;
            return kLoadErrorCouldNotOpenXML;
        }
    } else {
        return kLoadErrorMissingFile;
    }

    return kLoadOk;
}

const char* VoiceFile::GetLoadErrorString() {
    return "";
}

void VoiceFile::Unload() {

}

const std::string & VoiceFile::GetVoicePath( const std::string voice ) {
    std::map<std::string, std::string>::iterator iter;
    iter = voice_paths.find(voice);
    if(iter != voice_paths.end()){
        return iter->second;
    } else {
        return null_string;
    }
}

void VoiceFile::ReturnPaths( PathSet &path_set )
{
    path_set.insert("voice "+path_);
    for(auto & voice_path : voice_paths)
    {
        //SoundGroupInfoCollection::Instance()->ReturnRef(iter->second)->ReturnPaths(path_set);
        Engine::Instance()->GetAssetManager()->LoadSync<SoundGroupInfo>(voice_path.second)->ReturnPaths(path_set);
    }
}

AssetLoaderBase* VoiceFile::NewLoader() {
    return new FallbackAssetLoader<VoiceFile>();
}
