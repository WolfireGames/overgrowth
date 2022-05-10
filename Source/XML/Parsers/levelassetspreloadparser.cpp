//-----------------------------------------------------------------------------
//           Name: levelassetpreloadparser.cpp
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
#include "levelassetspreloadparser.h"

#include <Utility/strings.h>
#include <Utility/serialize.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

uint32_t LevelAssetPreloadParser::Load(const std::string& path) {
    TiXmlDocument doc(path.c_str());
    doc.LoadFile();
    if (!doc.Error()) {
        TiXmlElement* pRoot = doc.RootElement();
        if (pRoot) {
            TiXmlElement* e = pRoot->FirstChildElement("Asset");
            while (e) {
                Asset a;

                a.path = nullAsEmpty(e->Attribute("path"));
                a.level_name = nullAsEmpty(e->Attribute("level_name"));
                a.asset_type = GetAssetTypeValue(nullAsEmpty(e->Attribute("asset_type")));

                int all_levels = saysTrue(e->Attribute("all_levels"));
                if (all_levels == SAYS_TRUE) {
                    a.all_levels = true;
                } else if (all_levels == SAYS_FALSE) {
                    a.all_levels = false;
                } else if (all_levels == SAYS_TRUE_NO_MATCH) {
                    LOGW << "Unexpected string in all_levels attribute" << std::endl;
                } else if (all_levels == SAYS_TRUE_NULL_INPUT) {
                    a.all_levels = false;
                }

                string_flags_to_uint32(&a.load_flags, nullAsEmpty(e->Attribute("load_flags")));

                assets.push_back(a);

                e = e->NextSiblingElement("Asset");
            }
        }
    }
    return 0;
}

bool LevelAssetPreloadParser::Save(const std::string& path) {
    return false;
}

void LevelAssetPreloadParser::Clear() {
    assets.clear();
}

LevelAssetPreloadParser::Asset::Asset() : load_flags(0x0) {
    asset_type = UNKNOWN;
}
