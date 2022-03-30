//-----------------------------------------------------------------------------
//           Name: database_manifest.h
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
#include "databasemanifestresult.h"

#include <vector>
#include <set>

class JobHandler;
class Builder;
class Item;

class DatabaseManifest
{
    int thread;
    std::vector<DatabaseManifestResult> results;
    std::set<uint64_t> hash_set;
public:
    DatabaseManifest();

    bool Load(const std::string& manifest);
    bool Save(const std::string& manifest);

    void AddResult( const DatabaseManifestResult& results );

    bool HasBuiltResultFor( JobHandler& jh, const Item& item, const Builder& builder );
    DatabaseManifestResult GetPreviouslyBuiltResult( const Item& item, const Builder& builder);
};
