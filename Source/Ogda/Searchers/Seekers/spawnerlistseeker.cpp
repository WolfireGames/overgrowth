//-----------------------------------------------------------------------------
//           Name: spawnerlistseeker.cpp
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
#include "spawnerlistseeker.h"

#include <JSON/jsonhelper.h>
#include <Logging/logdata.h>
#include <string>

std::vector<Item> SpawnerListSeeker::SearchJSON(const Item& item, Json::Value& root) {
    std::vector<Item> items;

    Json::Value spawner_tab = root["spawner_tab"];
    Json::Value ogda_type = root["ogda_type"];
    Json::Value objects = root["objects"];

    if (spawner_tab.empty()) {
        LOGW << item << " is missing a spawner_tab value" << std::endl;
    }

    if (ogda_type.isString()) {
        std::string type_string = ogda_type.asString();

        if (objects.isArray()) {
            for (int i = 0; i < objects.size(); i++) {
                Json::Value json_item = objects[i];

                if (json_item.isArray()) {
                    Json::Value path = json_item[1];

                    if (path.isString()) {
                        std::string path_string = path.asString();
                        Item i = Item(item.input_folder, path_string, type_string, item.source);
                        items.push_back(i);
                    } else {
                        LOGE << "Unexpected path value on index " << i << " in " << item << std::endl;
                    }
                } else {
                    LOGE << "Unexpected item element on index " << i << " " << item << std::endl;
                }
            }
        } else {
            LOGE << "Malformed json file " << item << std::endl;
        }
    } else {
        LOGE << "Missing ogda_type value in " << item << std::endl;
    }

    return items;
}
