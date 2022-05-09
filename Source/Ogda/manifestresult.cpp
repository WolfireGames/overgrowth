//-----------------------------------------------------------------------------
//           Name: manifestresult.cpp
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
#include "manifestresult.h"

#include <string>

#include <Internal/filesystem.h>
#include <Ogda/jobhandler.h>
#include <Ogda/Builders/actionbase.h>
#include <Ogda/Generators/creatorbase.h>
#include <Ogda/ogda_hash.h>

ManifestResult::ManifestResult(const JobHandler& jh, const std::string& dest, bool success, const CreatorBase& creator, const std::string& type)
    : dest(dest), success(success), name(creator.GetName()), version(creator.GetVersion()), fresh_built(true), mr_type(GENERATED), type(type) {
    std::string full_dest_path = AssemblePath(jh.output_folder, dest);
    dest_hash = OgdaGetFileHash(full_dest_path.c_str());
}

ManifestResult::ManifestResult(const JobHandler& jh, const Item& item, const std::string& dest, bool success, const ActionBase& action, const std::string& type)
    : dest(dest), success(success), name(action.GetName()), version(action.GetVersion()), fresh_built(true), mr_type(BUILT), type(type) {
    items.push_back(item);
    std::string full_dest_path = AssemblePath(jh.output_folder, dest);
    dest_hash = OgdaGetFileHash(full_dest_path.c_str());
}

ManifestResult::ManifestResult(const std::string& dest_hash, const std::vector<Item>& items, const std::string& dest, bool success, const std::string& name, const std::string& version, const ManifestResultType mr_type, const std::string& type)
    : items(items), dest(dest), success(success), dest_hash(dest_hash), name(name), version(version), fresh_built(false), mr_type(mr_type), type(type) {
}

ManifestResult::ManifestResult(const std::string& dest_hash, Item& item, const std::string& dest, bool success, const std::string& name, const std::string& version, const ManifestResultType mr_type, const std::string& type)
    : dest(dest), success(success), dest_hash(dest_hash), name(name), version(version), fresh_built(false), mr_type(mr_type), type(type) {
    items.push_back(item);
}

ManifestResult::ManifestResult(const std::string& dest_hash, const std::string& dest, bool success, const std::string& name, const std::string& version, const ManifestResultType mr_type, const std::string& type)
    : dest(dest), success(success), dest_hash(dest_hash), name(name), version(version), fresh_built(false), mr_type(mr_type), type(type) {
}

void ManifestResult::CalculateHash(const char* base_path) {
    current_dest_hash = GetFileHash(AssemblePath(std::string(base_path), dest).c_str()).ToString();
}

const std::string& ManifestResult::GetCurrentDestHash(const std::string& base_path) {
    if (current_dest_hash.empty())
        CalculateHash(base_path.c_str());
    return current_dest_hash;
}

std::ostream& operator<<(std::ostream& out, const ManifestResult& mr) {
    out << "ManifestResult( items { ";
    for (const auto& item : mr.items) {
        out << item << ",";
    }
    out << "}," << mr.dest << "," << mr.dest_hash << "," << mr.name << "," << mr.version << "," << mr.type << ")";
    return out;
}
