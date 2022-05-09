//-----------------------------------------------------------------------------
//           Name: elementscanner.cpp
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
#include "elementscanner.h"

#include <string>
#include <cassert>

#include <Ogda/jobhandler.h>
#include <Internal/filesystem.h>
#include <Utility/strings.h>
#include <XML/xml_helper.h>
#include <Ogda/Searchers/Seekers/xmlseekerbase.h>

#include <tinyxml.h>

void ElementScanner::Do(
    std::vector<Item>& items,
    const Item& item,
    TiXmlNode* eRoot,
    const std::vector<elempair>& elems,
    const std::vector<const char*>& elems_ignore,
    XMLSeekerBase* callback,
    void* userdata) {
    if (eRoot) {
        TiXmlElement* eElem = eRoot->FirstChildElement();

        while (eElem) {
            const char* name = eElem->Value();
            const char* text = eElem->GetText();
            if (name) {
                int id;
                if ((id = FindStringInArray(elems, name)) >= 0) {
                    if (strlen(elems[id].second) > 0) {
                        if (text && strlen(text) > 0) {
                            items.push_back(Item(item.input_folder, text, elems[id].second, item.source));
                        } else {
                            LOGW << "String value in " << item << " for element " << elems[id].first << " is empty, row " << eElem->Row() << std::endl;
                        }
                    }

                    if (callback)
                        callback->HandleElementCallback(items, eRoot, eElem, item, userdata);
                } else if ((id = FindStringInArray(elems_ignore, name)) >= 0) {
                    LOGD << "Ignored " << elems_ignore[id] << " in " << item << " row " << eElem->Row() << std::endl;
                } else {
                    LOGE << "Unahandled subvalue from " << item << " called " << name << " row " << eElem->Row() << std::endl;
                }
            } else {
                LOGE << "Generic warning" << std::endl;
            }

            eElem = eElem->NextSiblingElement();
        }
    }
}
