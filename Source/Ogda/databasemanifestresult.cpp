//-----------------------------------------------------------------------------
//           Name: databasemanifestresult.cpp
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
#include "databasemanifestresult.h"

#include <string>

#include <Internal/filesystem.h>
#include <Ogda/jobhandler.h>
#include <Ogda/Builders/actionbase.h>
#include <Ogda/Generators/creatorbase.h>
#include <Ogda/ogda_hash.h>

DatabaseManifestResult::DatabaseManifestResult(const Item& item, const std::string& dest_hash, const std::string& dest, const std::string& name, const std::string& version, const std::string& type)
    : item(item), dest(dest), dest_hash(dest_hash), name(name), version(version), type(type) {
}

void DatabaseManifestResult::CalculateHash(const char* base_path) {
    current_dest_hash = GetFileHash(AssemblePath(base_path, AssemblePath("files", AssemblePath(item.hash, dest_hash))).c_str()).ToString();
}

const std::string& DatabaseManifestResult::GetCurrentDestHash(const std::string& base_path) {
    if (current_dest_hash.empty())
        CalculateHash(base_path.c_str());
    return current_dest_hash;
}
