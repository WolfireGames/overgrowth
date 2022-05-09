//-----------------------------------------------------------------------------
//           Name: path.h
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

#include <Internal/modid.h>

#include <Global/global_config.h>

enum PathFlags {
    kNoPath = 0,
    kDataPaths = 1 << 0,
    kWriteDir = 1 << 1,
    kModPaths = 1 << 2,
    kAbsPath = 1 << 3,
    kModWriteDirs = 1 << 4,
    kAnyPath = kDataPaths | kWriteDir | kModPaths | kAbsPath | kModWriteDirs
};

struct Path {
    Path();

    std::string GetAbsPathStr() const;
    std::string GetFullPathStr() const;
    const char* GetFullPath() const;
    std::string GetOriginalPathStr() const;
    const char* GetOriginalPath() const;
    ModID GetModsource() const;
    bool isValid() const;

    // What mod, if any, this file comes from.
    // This is valid only if source has the mod flag set.
    ModID mod_source;

    // The original path request value
    char original[kPathSize];
    // The found path
    char resolved[kPathSize];
    // Where the file was found
    PathFlags source;
    // Is file valid
    bool valid;

    bool operator==(const Path& rhs) const;
    bool operator<(const Path& rhs) const;
};

std::ostream& operator<<(std::ostream& out, const Path& path);
