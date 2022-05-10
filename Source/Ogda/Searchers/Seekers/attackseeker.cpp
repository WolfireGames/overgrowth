//-----------------------------------------------------------------------------
//           Name: attackseeker.cpp
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
#include "attackseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

std::vector<Item> AttackSeeker::SearchXML(const Item& item, TiXmlDocument& doc) {
    const char* elems[] =
        {
            "reaction"};

    const char* elems_type[] =
        {
            "attack"};

    assert(ARRLEN(elems_type) == ARRLEN(elems));

    const char* ignored[] =
        {
            "flags",
            "impact_dir",
            "block_damage",
            "damage",
            "sharp_damage",
            "force",
            "materialevent",
            "stab_dir",
            "cut_plane"};

    std::vector<Item> items;

    TiXmlHandle hRoot(&doc);

    TiXmlElement* eRoot = hRoot.FirstChildElement().Element();

    if (!eRoot) {
        LOGE << "Can't find anything in file listed " << item << std::endl;
    }

    while (eRoot) {
        TiXmlElement* eElem = eRoot->FirstChildElement();

        while (eElem) {
            const char* name = eElem->Value();
            const char* text = eElem->GetText();
            if (name) {
                int id;
                if ((id = FindStringInArray(elems, ARRLEN(elems), name)) >= 0) {
                    if (text && strlen(text) > 0) {
                        items.push_back(Item(item.input_folder, text, elems_type[id], item.source));
                    } else {
                        LOGW << "String value in " << item << " for element " << elems[id] << " is empty" << std::endl;
                    }
                } else if ((id = FindStringInArray(ignored, ARRLEN(ignored), name)) >= 0) {
                    LOGD << "Ignored " << ignored[id] << " in " << item << std::endl;
                } else if (strmtch("animations", name)) {
                    std::vector<attribpair> attribs;
                    attribs.push_back(attribpair("unblocked", "animation"));
                    attribs.push_back(attribpair("blocked", "animation"));
                    attribs.push_back(attribpair("throw", "animation"));
                    attribs.push_back(attribpair("thrown", "animation"));
                    attribs.push_back(attribpair("thrown_counter", "animation"));

                    std::vector<const char*> ignore;

                    AttributeScanner::Do(items, item, eElem, attribs, ignore);
                } else {
                    LOGE << "Unahandled subvalue in Attack from " << item << " called " << name << std::endl;
                }
            } else {
                LOGE << "Generic warning" << std::endl;
            }

            eElem = eElem->NextSiblingElement();
        }
        eRoot = eRoot->NextSiblingElement();
    }

    return items;
}
