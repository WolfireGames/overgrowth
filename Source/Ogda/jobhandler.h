//-----------------------------------------------------------------------------
//           Name: jobhandler.h
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

#include <string>
#include <set>
#include <vector>

#include "item.h"
#include <Ogda/Builders/builder.h>
#include <Ogda/Searchers/searcher.h>
#include <Ogda/Generators/generator.h>

class JobHandler {
    std::map<std::string, int> typeSearcherCount;
    std::map<std::string, int> typeBuilderCount;

    std::vector<Item> items;
    std::vector<Item> searchedItems;
    std::set<Item> foundItems;

    std::vector<Builder> builders;
    std::vector<Searcher> searchers;
    std::vector<Generator> generators;

    int threads;

    void RunRecursiveSearchOn(const Item& item);

   public:
    std::vector<std::string> input_folders;
    std::string output_folder, manifest_dest, manifest_source, databasedir;
    bool perform_removes;
    bool force_removes;

    JobHandler(std::string output_folder, std::string manifest_dest, std::string manifest_source, std::string databasedir, bool perform_removes, bool force_removes, int threads);
    bool Run(const std::string& path);
    void AddItem(const std::string& input_folder, const std::string& path, const std::string& type, const JobXMLParser::Item& source);

    std::vector<Item>::const_iterator ItemsBegin() const;
    std::vector<Item>::const_iterator ItemsEnd() const;

    bool HasItemWithPath(const std::string& path);
};
