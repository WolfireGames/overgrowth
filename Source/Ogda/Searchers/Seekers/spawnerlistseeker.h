//-----------------------------------------------------------------------------
//           Name: spawnerlistseeker.h
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
#pragma once

#include "jsonseekerbase.h"

namespace Json {
class Value;
}

class SpawnerListSeeker : public JSONSeekerBase {
   public:
    std::vector<Item> SearchJSON(const Item& item, Json::Value& root) override;
    const char* GetName() override { return "spawner_list_seeker"; }
};
