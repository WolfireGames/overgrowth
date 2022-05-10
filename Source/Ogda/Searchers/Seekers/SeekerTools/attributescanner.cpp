//-----------------------------------------------------------------------------
//           Name: attributescanner.cpp
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
#include "attributescanner.h"

#include <string>
#include <cassert>

#include <Ogda/jobhandler.h>
#include <Internal/filesystem.h>
#include <Utility/strings.h>
#include <XML/xml_helper.h>

#include <tinyxml.h>

void AttributeScanner::Do(
    std::vector<Item>& items,
    const Item& item,
    TiXmlElement* eElem,
    const std::vector<attribpair>& attribs,
    const std::vector<const char*>& attribs_ignore) {
    if (eElem) {
        TiXmlAttribute* aV = eElem->FirstAttribute();
        while (aV) {
            int id = -1;
            if ((id = FindStringInArray(attribs, aV->Name())) >= 0) {
                if (aV->Value() && strlen(aV->Value()) > 0) {
                    items.push_back(Item(item.input_folder, aV->Value(), attribs[id].second, item.source));
                } else {
                    LOGD << "Value is empty in " << item << std::endl;
                }
            } else if (FindStringInArray(attribs_ignore, aV->Name()) < 0) {
                LOGE << "Unhandled attrib on row " << aV->Row() << " " << aV->Name() << " in " << item << std::endl;
            }

            aV = aV->Next();
        }
    } else {
        LOGE << "Root element is null for " << item << std::endl;
    }
}

void AttributeScanner::DoAllSame(
    std::vector<Item>& items,
    const Item& item,
    TiXmlElement* eElem,
    std::string type) {
    if (eElem) {
        TiXmlAttribute* aV = eElem->FirstAttribute();
        while (aV) {
            if (aV->Value() && strlen(aV->Value()) > 0) {
                items.push_back(Item(item.input_folder, aV->Value(), type, item.source));
            } else {
                LOGD << "Value is empty in " << item << std::endl;
            }

            aV = aV->Next();
        }
    } else {
        LOGE << "Root element is null for " << item << std::endl;
    }
}
