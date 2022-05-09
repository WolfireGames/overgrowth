//-----------------------------------------------------------------------------
//           Name: ambientsoundlevelseeker.cpp
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
#include "ambientsoundlevelseeker.h"

#include <tinyxml.h>

#include <Logging/logdata.h>

std::vector<Item> AmbientSoundLevelSeeker::SearchLevelRoot(const Item& item, TiXmlHandle& hRoot) {
    std::vector<Item> items;
    TiXmlElement* eElem = hRoot.FirstChildElement("AmbientSounds").FirstChildElement().Element();

    while (eElem) {
        const char* path = eElem->Attribute("path");
        if (path) {
            {
                std::stringstream ss;
                ss << path << ".xml";

                Item i(item.input_folder, ss.str(), "sound", item.source);

                if (i.FileAccess()) {
                    items.push_back(i);
                }
            }

            bool found_file = false;
            int counter = 1;
            do {
                std::stringstream ss;
                ss << path << "_" << counter << ".wav";

                Item i(item.input_folder, ss.str(), "sound", item.source);

                if (i.FileAccess()) {
                    found_file = true;
                    items.push_back(i);
                } else {
                    found_file = false;
                }
                counter++;
            } while (found_file);
        }
        eElem = eElem->NextSiblingElement();
    }

    return items;
}
