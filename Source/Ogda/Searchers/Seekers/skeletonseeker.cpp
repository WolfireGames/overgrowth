//-----------------------------------------------------------------------------
//           Name: skeletonseeker.cpp
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
#include "skeletonseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

std::vector<Item> SkeletonSeeker::SearchXML(const Item& item, TiXmlDocument& doc) {
    std::vector<Item> items;

    TiXmlHandle hRoot(&doc);

    const char* roots[] =
        {
            "rig"};

    TiXmlElement* eRoot = hRoot.FirstChildElement().Element();

    if (!eRoot) {
        LOGE << "Can't find anything in file listed " << item << std::endl;
    }

    while (eRoot) {
        if (FindStringInArray(roots, ARRLEN(roots), eRoot->Value()) < 0) {
            LOGE << "Unknown root " << eRoot->Value() << std::endl;
        }

        std::vector<attribpair> attribs;
        attribs.push_back(attribpair("bone_path", "skeleton"));
        attribs.push_back(attribpair("model_path", "model"));
        attribs.push_back(attribpair("mass_path", "skeleton"));

        std::vector<const char*> attribs_ignore;

        AttributeScanner::Do(items, item, eRoot, attribs, attribs_ignore);

        eRoot = eRoot->NextSiblingElement();
    }
    return items;
}
