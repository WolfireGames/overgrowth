//-----------------------------------------------------------------------------
//           Name: actorobjectlevelseeker.cpp
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

#include "levelseekerbase.h"

class TiXmlElement;

class ActorObjectLevelSeeker : public LevelSeekerBase {
   public:
    virtual void SearchGroup(std::vector<Item>& items, const Item& item, TiXmlElement* root);
    std::vector<Item> SearchLevelRoot(const Item& item, TiXmlHandle& hRoot) override;

    inline const char* GetName() override {
        return "actor_object_level_seeker";
    }
};
