//-----------------------------------------------------------------------------
//           Name: particleseeker.cpp
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
#include "particleseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

enum {
    ROOT = 1,
    PARTICLE
};

std::vector<Item> ParticleSeeker::SearchXML(const Item& item, TiXmlDocument& doc) {
    std::vector<Item> items;

    std::vector<elempair> elems;
    elems.push_back(elempair("particle", ""));

    std::vector<const char*> elems_ignore;

    ElementScanner::Do(items, item, &doc, elems, elems_ignore, this, (void*)ROOT);

    return items;
}

void ParticleSeeker::HandleElementCallback(std::vector<Item>& items, TiXmlNode* eRoot, TiXmlElement* eElem, const Item& item, void* userdata) {
    if (userdata == (void*)ROOT) {
        if (strmtch(eElem->Value(), "particle")) {
            std::vector<elempair> elems;
            elems.push_back(elempair("textures", ""));
            elems.push_back(elempair("collision", ""));

            std::vector<const char*> elems_ignore;
            elems_ignore.push_back("size");
            elems_ignore.push_back("rotation");
            elems_ignore.push_back("stretch");
            elems_ignore.push_back("color");
            elems_ignore.push_back("behaviour");
            elems_ignore.push_back("behavior");
            elems_ignore.push_back("no_rotation");
            elems_ignore.push_back("quadratic_expansion");
            elems_ignore.push_back("quadratic_dispersion");

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)PARTICLE);
        } else {
            LOGE << "Unknown item sub" << eElem->Value() << " " << item << std::endl;
        }
    } else if (userdata == (void*)PARTICLE) {
        std::vector<attribpair> elems;
        std::vector<const char*> elems_ignore;

        if (strmtch(eElem->Value(), "textures")) {
            elems.push_back(attribpair("color_map", "texture"));
            elems.push_back(attribpair("normal_map", "texture"));
            // Ignoring this as it never seems to actually point to any real object.
            // elems.push_back(attribpair("animation_effect", "animation_effect"));

            elems_ignore.push_back("animation_effect");
            elems_ignore.push_back("shader");
            elems_ignore.push_back("soft_shader");
        } else if (strmtch(eElem->Value(), "collision")) {
            elems.push_back(attribpair("decal", "decal_object"));

            elems_ignore.push_back("decal_size_mult");
            elems_ignore.push_back("destroy");
            elems_ignore.push_back("character_collide");
            elems_ignore.push_back("character_add_blood");
            elems_ignore.push_back("material_event");
            elems_ignore.push_back("materialevent");
        } else {
            LOGE << "Unknown item sub" << eElem->Value() << " " << item << std::endl;
        }

        AttributeScanner::Do(items, item, eElem, elems, elems_ignore);
    } else if (userdata == (void*)0) {
    } else {
        XMLSeekerBase::HandleElementCallback(items, eRoot, eElem, item, userdata);
    }
}
