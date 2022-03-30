//-----------------------------------------------------------------------------
//           Name: scriptfile.h
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

#include <Internal/datemodified.h>
#include <Internal/path.h>
#include <Internal/modid.h>
#include <Internal/path.h>

#include <Asset/assets.h>

#include <map>
#include <string>
#include <list>
#include <vector>

struct Dependency {
    Path path;
    int64_t modified;
};

struct IncludeFileRange {
    unsigned start;
    Path file_path;
    unsigned offset;
    unsigned length;
    IncludeFileRange(unsigned _start, 
                     unsigned _length, 
                     unsigned _offset, 
                     const Path &_file_path)
        :start(_start),
         file_path(_file_path),
         offset(_offset),
         length(_length)
    {}
};

typedef std::list<IncludeFileRange> FileRangeList;

struct LineFile {
    unsigned line_number;
    const Path file;

    LineFile(unsigned _line_number,
             const Path &_file)
        :line_number(_line_number),
         file(_file)
    {}
};

struct ScriptFile {
    const ScriptFile* parent;
    std::string unexpanded_contents;
    std::string contents;
    Path file_path;
    std::vector<Dependency> dependencies;
    FileRangeList file_range;
    int64_t latest_modification;
    unsigned long hash;

    void ExpandIncludePaths();
    LineFile GetCorrectedLine(unsigned line) const;

    std::string GetModPollutionInformation() const;

private:
    bool AlreadyAddedIncludeFile(const Path &path);
};

typedef std::map<std::string, ScriptFile> ScriptFileMap;

struct ScriptFileCache {
    ScriptFileMap files;

    static ScriptFileCache* Instance() {
        static ScriptFileCache instance;
        return &instance;
    }

    void Dispose();
};

namespace ScriptFileUtil {
    const ScriptFile* GetScriptFile(const Path& path);
    bool DetectScriptFileChanged(const Path &path);
    int64_t GetLatestModification(const Path &path);
    void ReturnPaths(const Path &path, PathSet &path_set);
}
