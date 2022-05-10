//-----------------------------------------------------------------------------
//           Name: manifestparser.h
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

#include <XML/Parsers/xmlparserbase.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>

class ManifestXMLParser {
   public:
    ManifestXMLParser();

    class Item {
       public:
        Item(std::string path, std::string type, std::string hash);
        std::string path, type, hash;
    };

    class BuilderResult {
       public:
        BuilderResult(std::string dest, std::string dest_hash, std::string builder, std::string builder_version, std::string type, bool success, bool fresh_built, std::vector<Item> items);

        std::string dest, dest_hash, builder, builder_version, type;
        bool success, fresh_built;

        std::vector<Item> items;
    };

    class GeneratorResult {
       public:
        GeneratorResult(std::string dest, std::string dest_hash, std::string generator, std::string generator_version, std::string type, bool success, bool fresh_built);

        std::string dest, dest_hash, generator, generator_version, type;
        bool success, fresh_built;
    };

    class Manifest {
       public:
        Manifest();
        Manifest(std::vector<BuilderResult> builder_results, std::vector<GeneratorResult> generator_results);

        std::vector<BuilderResult> builder_results;
        std::vector<GeneratorResult> generator_results;
    };

    Manifest manifest;

    virtual bool Load(const std::string& path);
    virtual bool Save(const std::string& path);
    virtual void Clear();
};
