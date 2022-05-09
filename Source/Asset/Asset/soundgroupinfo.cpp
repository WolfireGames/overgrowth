//-----------------------------------------------------------------------------
//           Name: soundgroupinfo.cpp
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
#include "soundgroupinfo.h"

#include <Internal/scoped_buffer.h>
#include <Internal/integer.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>
#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Utility/strings.h>
#include <Memory/allocation.h>

#include <tinyxml.h>

#include <sstream>

SoundGroupInfo::SoundGroupInfo(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id) {
}

bool SoundGroupInfo::ParseXML(const char* data) {
    TiXmlDocument doc;
    doc.Parse(data);
    bool ret_value = false;

    if (!doc.Error()) {
        TiXmlHandle hDoc(&doc);
        TiXmlHandle hRoot = hDoc.FirstChildElement();
        TiXmlElement* root = hRoot.ToElement();
        if (root) {
            if (root->QueryIntAttribute("variants", &num_variants) != TIXML_SUCCESS) {
                num_variants = 1;
                LOGW << "Missing element \"variants\" in SoundGroupInfo asset: " << path_ << std::endl;
            }

            if (root->QueryFloatAttribute("delay", &delay) != TIXML_SUCCESS) {
                delay = 0.0f;
                LOGW << "Missing element \"delay\" in SoundGroupInfo asset: " << path_ << std::endl;
            }

            if (root->QueryFloatAttribute("volume", &volume) != TIXML_SUCCESS) {
                volume = 1.0f;
                LOGW << "Missing element \"volume\" in SoundGroupInfo asset: " << path_ << std::endl;
            }

            if (root->QueryFloatAttribute("max_distance", &max_distance) != TIXML_SUCCESS) {
                max_distance = 30.0f;
                // LOGW << "Missing element \"max_distance\" in SoundGroupInfo asset: " << path_ << std::endl;
            }

            ret_value = true;
        } else {
            LOGE << "root node was null in SoundGroupInfo ParseXML" << std::endl;
        }
    } else {
        LOGE << "Unable to parse soundgroupinfo from given data" << std::endl;
    }

    return ret_value;
}

int SoundGroupInfo::Load(const std::string& _path, uint32_t load_flags) {
    path_ = _path;
    if (path_.size() < 4) {
        return kLoadErrorMissingFile;
    }
    const char* suffix = &path_[path_.size() - 4];
    if (strmtch(suffix, ".xml")) {
        size_t size_out;
        uint8_t* data = StackLoadText(path_.c_str(), &size_out);
        if (data) {
            if (ParseXML((const char*)data)) {
            } else {
                LOGE << "Failed to parse xml contents from file: " << path_ << std::endl;
            }
            alloc.stack.Free(data);
        } else {
            LOGE << "LoadText on " << _path << " failed, will not parse" << std::endl;
            return kLoadErrorMissingFile;
        }
    } else {
        return kLoadErrorInvalidFileEnding;
    }
    return kLoadOk;
}

const char* SoundGroupInfo::GetLoadErrorString() {
    return "";
}

void SoundGroupInfo::Unload() {
}

void SoundGroupInfo::Reload() {
}

void SoundGroupInfo::ReportLoad() {
}

void SoundGroupInfo::ReturnPaths(PathSet& path_set) {
    path_set.insert("soundgroup " + path_);
    for (int i = 0; i < num_variants; ++i) {
        path_set.insert("sound " + GetSoundPath(i));
    }
}

std::string SoundGroupInfo::GetSoundPath(int choice) const {
    std::ostringstream oss;
    oss << path_.substr(0, path_.size() - 4) << "_" << choice + 1 << ".wav";
    return oss.str();
}

AssetLoaderBase* SoundGroupInfo::NewLoader() {
    return new FallbackAssetLoader<SoundGroupInfo>();
}
