//-----------------------------------------------------------------------------
//           Name: attachementasset.cpp
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
#include "attachmentasset.h"

#include <XML/xml_helper.h>
#include <Logging/logdata.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <tinyxml.h>

#include <string>

using std::string;

Attachment::Attachment(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id) {
}

void Attachment::clear() {
    anim.clear();
    ik_chain_label.clear();
    ik_chain_bone = -1;
    mirror_allowed = false;
}

int Attachment::Load(const string& path, uint32_t load_flags) {
    TiXmlDocument doc;
    if (LoadXMLRetryable(doc, path, "Attachment")) {
        clear();

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement()->FirstChildElement();
        for (; field; field = field->NextSiblingElement()) {
            string field_str(field->Value());
            if (field_str == "anim") {
                anim = field->GetText();
            } else if (field_str == "attach") {
                const char* tf;
                tf = field->Attribute("ik_chain");
                if (tf) {
                    ik_chain_label = tf;
                }
                field->QueryIntAttribute("bone", &ik_chain_bone);
            } else if (field_str == "mirror") {
                const char* tf;
                tf = field->Attribute("allow");
                if (tf && strcmp(tf, "true") == 0) {
                    mirror_allowed = true;
                }
            }
        }
    } else {
        return kLoadErrorMissingFile;
    }
    return kLoadOk;
}

const char* Attachment::GetLoadErrorString() {
    return "";
}

void Attachment::Unload() {
}

void Attachment::Reload() {
    Load(path_, 0x0);
}

void Attachment::ReportLoad() {
}

AssetLoaderBase* Attachment::NewLoader() {
    return new FallbackAssetLoader<Attachment>();
}
