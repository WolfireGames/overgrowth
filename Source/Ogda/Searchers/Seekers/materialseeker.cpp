//-----------------------------------------------------------------------------
//           Name: materialseeker.cpp
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
#include "materialseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

enum {
    ROOT = 1,
    EVENT,
    MATERIAL,
    PARTICLES,
    DECALS
};

std::vector<Item> MaterialSeeker::SearchXML(const Item& item, TiXmlDocument& doc) {
    std::vector<Item> items;

    std::vector<elempair> elems;
    elems.push_back(elempair("material", ""));

    std::vector<const char*> elems_ignore;

    ElementScanner::Do(items, item, &doc, elems, elems_ignore, this, (void*)ROOT);

    return items;
}

void MaterialSeeker::HandleElementCallback(std::vector<Item>& items, TiXmlNode* eRoot, TiXmlElement* eElem, const Item& item, void* userdata) {
    if (userdata == (void*)ROOT) {
        if (strmtch(eElem->Value(), "material")) {
            std::vector<elempair> elems;

            elems.push_back(elempair("events", ""));
            elems.push_back(elempair("particles", ""));
            elems.push_back(elempair("decals", ""));

            std::vector<const char*> elems_ignore;
            elems_ignore.push_back("physics");

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)MATERIAL);
        } else {
            LOGE << "Unknown item sub " << eElem->Value() << " " << item << std::endl;
        }
    } else if (userdata == (void*)MATERIAL) {
        if (strmtch(eElem->Value(), "events")) {
            std::vector<elempair> elems;

            elems.push_back(elempair("leftrunstep", ""));
            elems.push_back(elempair("rightrunstep", ""));
            elems.push_back(elempair("leftwallstep", ""));
            elems.push_back(elempair("rightwallstep", ""));
            elems.push_back(elempair("leftwalkstep", ""));
            elems.push_back(elempair("rightwalkstep", ""));
            elems.push_back(elempair("leftcrouchwalkstep", ""));
            elems.push_back(elempair("rightcrouchwalkstep", ""));
            elems.push_back(elempair("leftcrouchwalkstep", ""));
            elems.push_back(elempair("rightcrouchwalkstep", ""));
            elems.push_back(elempair("land", ""));
            elems.push_back(elempair("land_soft", ""));
            elems.push_back(elempair("land_slide", ""));
            elems.push_back(elempair("slide", ""));
            elems.push_back(elempair("kick", ""));
            elems.push_back(elempair("sweep", ""));
            elems.push_back(elempair("jump", ""));
            elems.push_back(elempair("roll", ""));
            elems.push_back(elempair("edge_grab", ""));
            elems.push_back(elempair("edge_crawl", ""));
            elems.push_back(elempair("bodyfall", ""));
            elems.push_back(elempair("flip", ""));
            elems.push_back(elempair("choke_move", ""));
            elems.push_back(elempair("choke_grab", ""));
            elems.push_back(elempair("choke_full", ""));
            elems.push_back(elempair("choke_fall", ""));

            elems.push_back(elempair("weapon_drop_light", ""));
            elems.push_back(elempair("weapon_drop_medium", ""));
            elems.push_back(elempair("weapon_drop_heavy", ""));

            elems.push_back(elempair("bodyfall_light", ""));
            elems.push_back(elempair("bodyfall_medium", ""));
            elems.push_back(elempair("bodyfall_heavy", ""));

            elems.push_back(elempair("blood_spatter", ""));
            elems.push_back(elempair("blood_drip", ""));

            elems.push_back(elempair("physics", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)EVENT);
        } else if (strmtch(eElem->Value(), "particles")) {
            std::vector<elempair> elems;

            elems.push_back(elempair("step", ""));
            elems.push_back(elempair("skid", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)PARTICLES);
        } else if (strmtch(eElem->Value(), "decals")) {
            std::vector<elempair> elems;

            elems.push_back(elempair("step", ""));
            elems.push_back(elempair("skid", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)DECALS);
        } else {
            LOGE << "Unknown item sub " << eElem->Value() << " " << item << std::endl;
        }
    } else if (userdata == (void*)DECALS) {
        std::vector<attribpair> elems;
        std::vector<const char*> elems_ignore;

        elems.push_back(attribpair("color", "texture"));
        elems.push_back(attribpair("normal", "texture"));

        elems_ignore.push_back("shader");

        AttributeScanner::Do(items, item, eElem, elems, elems_ignore);
    } else if (userdata == (void*)EVENT) {
        std::vector<attribpair> elems;
        std::vector<const char*> elems_ignore;

        elems.push_back(attribpair("soundgroup", "sound"));

        elems_ignore.push_back("attached");
        elems_ignore.push_back("max_distance");

        AttributeScanner::Do(items, item, eElem, elems, elems_ignore);
    } else if (userdata == (void*)PARTICLES) {
        std::vector<attribpair> elems;
        std::vector<const char*> elems_ignore;

        elems.push_back(attribpair("path", "particle"));

        AttributeScanner::Do(items, item, eElem, elems, elems_ignore);
    } else {
        XMLSeekerBase::HandleElementCallback(items, eRoot, eElem, item, userdata);
    }
}
