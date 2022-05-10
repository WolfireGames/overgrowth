//-----------------------------------------------------------------------------
//           Name: item.h
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
#include <XML/Parsers/jobxmlparser.h>

class JobHandler;

class Item {
   public:
    bool only_parse;
    bool delete_on_exit;
    bool is_overshadowed;

   private:
    void SetPath(std::string path);
    std::string overshadows;

   public:
    // Which of the input folders the item was found in
    std::string input_folder;
    // Relative path beneath the Data/ path
    std::string path;

    std::string type;
    std::string hash;

    // Original root source to this items existance, could be from a hierarchy of adds.
    JobXMLParser::Item source;

    Item(const std::string& input_folder, const std::string& path, const std::string& type, const JobXMLParser::Item& source);
    Item(const std::string& input_folder, const std::string& path, const std::string& type, const std::string& hash, const JobXMLParser::Item& source);

    bool operator==(const JobXMLParser::Item& rhs) const;
    bool operator==(const Item& rhs) const;
    bool operator<(const Item& rhs) const;

    // Faster way of checking if the file is valid in case hash isn't calculated.
    bool FileAccess();
    void CalculateHash();
    void VerifyPath();
    std::string GetPath() const;
    std::string GetAbsPath() const;

    // Remove file when program shuts down
    bool IsDeleteOnExit();
    void SetDeleteOnExit(bool value);
    // Used for inlined scripts in XML files that are stored to a temp place on disk.
    bool IsOnlySearch();
    void SetOnlySearch(bool value);

    bool IsOvershadowed();
    void SetOvershadowed(bool value);

    bool Overshadows(const Item& item);
    void SetOvershadows(const Item& item);

    friend std::ostream& operator<<(std::ostream& out, const Item& item);

    // Extract a hash based on the first part. this is endian sensitive, so it shouldn't be stored, only used in runtime for first order identification.
    uint64_t GetSubHash() const;
};

std::ostream& operator<<(std::ostream& out, const Item& item);
