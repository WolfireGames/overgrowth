//-----------------------------------------------------------------------------
//           Name: songlistfile.cpp
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
#include "songlistfile.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/assetloaderrors.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

void SongListFile::Reload() {
    Load(path_, 0x0);
}

int SongListFile::Load(const std::string& _path, uint32_t load_flags) {
    song_paths.clear();
    path_ = "";

    TiXmlDocument doc;

    if (LoadXMLRetryable(doc, path_, "Song list")) {
        path_ = _path;
        if (doc.Error()) {
            return kLoadErrorCorruptFile;
        } else {
            TiXmlHandle h_doc(&doc);
            TiXmlHandle h_root = h_doc.FirstChildElement();
            TiXmlElement* field = h_root.ToElement()->FirstChildElement();
            for (; field; field = field->NextSiblingElement()) {
                std::string field_str(field->Value());
                song_paths[field_str] = field->Attribute("path");
                LOGI << "Loaded song " << field_str << " -> " << song_paths[field_str] << std::endl;
            }
            return kLoadOk;
        }
    } else {
        LOGE << "Unable to load SongListFile " << _path << std::endl;
        return kLoadErrorMissingFile;
    }
}

const char* SongListFile::GetLoadErrorString() {
    return "";
}

void SongListFile::Unload() {
}

const std::string& SongListFile::GetSongPath(const std::string song) {
    std::map<std::string, std::string>::iterator iter;
    iter = song_paths.find(song);
    if (iter != song_paths.end()) {
        return iter->second;
    } else {
        return null_string;
    }
}

const std::map<std::string, std::string>& SongListFile::GetSongPaths() {
    return song_paths;
}

AssetLoaderBase* SongListFile::NewLoader() {
    return new FallbackAssetLoader<SongListFile>();
}
