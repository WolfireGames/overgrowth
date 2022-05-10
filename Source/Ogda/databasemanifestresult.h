//-----------------------------------------------------------------------------
//           Name: databasemanifestresult.h
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

#include <Utility/hash.h>
#include "item.h"

class ActionBase;
class CreatorBase;
class JobHandler;

class DatabaseManifestResult {
   private:
    std::string current_dest_hash;

   public:
    DatabaseManifestResult(const Item& item, const std::string& dest_hash, const std::string& dest, const std::string& name, const std::string& version, const std::string& type);

    void CalculateHash(const char* base_path);
    const std::string& GetCurrentDestHash(const std::string& base_path);

    std::string dest;
    std::string dest_hash;

    std::string name;     // Either creator or action
    std::string version;  // Either creator or action
    std::string type;

    // Only in built data does this come in as relevant.
    // And even then, we don't have support for more than one source at this time.
    Item item;
};
