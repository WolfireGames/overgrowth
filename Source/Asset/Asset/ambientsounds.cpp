//-----------------------------------------------------------------------------
//           Name: ambientsounds.cpp
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

#include "ambientsounds.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/soundgroup.h>

#include <XML/xml_helper.h>
#include <Math/enginemath.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>
#include <string>

using std::string;

void AmbientSound::Unload() {
}

void AmbientSound::Reload() {
    Load(path_, 0x0);
}

AmbientSound::AmbientSound(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id), sub_error(0) {
}

int AmbientSound::Load(const string& path, uint32_t load_flags) {
    sub_error = 0;
    TiXmlDocument doc;
    if (LoadXMLRetryable(doc, path, "Ambient sound")) {
        sound_path.clear();
        type = _continuous;

        TiXmlHandle h_doc(&doc);
        if (doc.Error()) {
            return kLoadErrorCorruptFile;
        }

        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement();

        const char* c_str;
        c_str = field->Attribute("path");
        if (c_str) {
            sound_path = c_str;
        } else {
            sub_error = 1;
            return kLoadErrorIncompleteXML;
        }
        c_str = field->Attribute("type");
        if (c_str) {
            if (strcmp(c_str, "continuous") == 0) {
                type = _continuous;
            } else if (strcmp(c_str, "occasional") == 0) {
                type = _occasional;
            }
        } else {
            sub_error = 2;
            return kLoadErrorIncompleteXML;
        }

        field->QueryFloatAttribute("delay_min", &delay_min);
        field->QueryFloatAttribute("delay_max", &delay_max);
    } else {
        return kLoadErrorMissingFile;
    }
    return kLoadOk;
}

const char* AmbientSound::GetLoadErrorString() {
    switch (sub_error) {
        case 0:
            return "";
        case 1:
            return "path attribute missing in xml";
        case 2:
            return "type attribute missing in xml";
        default:
            return "Undefined error";
    }
}

void AmbientSound::ReportLoad() {
}

const string& AmbientSound::GetPath() {
    return sound_path;
}

AmbientSoundType AmbientSound::GetSoundType() {
    return type;
}

float AmbientSound::GetDelay() {
    return RangedRandomFloat(delay_min, delay_max);
}

float AmbientSound::GetDelayNoLower() {
    return RangedRandomFloat(0.0f, delay_max);
}

void AmbientSound::ReturnPaths(PathSet& path_set) {
    path_set.insert("ambientsound " + path_);
    // SoundGroupInfoCollection::Instance()->ReturnRef(sound_path)->ReturnPaths(path_set);
    Engine::Instance()->GetAssetManager()->LoadSync<SoundGroupInfo>(sound_path)->ReturnPaths(path_set);
}

AssetLoaderBase* AmbientSound::NewLoader() {
    return new FallbackAssetLoader<AmbientSound>();
}
