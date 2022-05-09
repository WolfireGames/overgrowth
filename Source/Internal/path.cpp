//-----------------------------------------------------------------------------
//           Name: path.cpp
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
#include "path.h"

#include <Utility/strings.h>

#include <Compat/compat.h>

#include <iostream>
#include <string>
#include <cstring>

using std::endl;
using std::ostream;
using std::string;

Path::Path() {
    memset(original, '\0', kPathSize * sizeof(char));
    memset(resolved, '\0', kPathSize * sizeof(char));
    source = kNoPath;
    valid = false;
}

string Path::GetAbsPathStr() const {
    return GetAbsPath(resolved);
}

string Path::GetFullPathStr() const {
    return string(resolved);
}

const char* Path::GetFullPath() const {
    return resolved;
}

string Path::GetOriginalPathStr() const {
    return string(original);
}

const char* Path::GetOriginalPath() const {
    return original;
}

ModID Path::GetModsource() const {
    return mod_source;
}

bool Path::isValid() const {
    return valid;
}

bool Path::operator==(const Path& rhs) const {
    return strmtch(resolved, rhs.resolved) && strmtch(original, rhs.original) && source == rhs.source && valid == rhs.valid;
}

bool Path::operator<(const Path& rhs) const {
    return strcmp(resolved, rhs.resolved);
}

ostream& operator<<(ostream& out, const Path& path) {
    out << "\"" << path.GetFullPath() << "\" (" << (path.valid ? "valid" : "invalid") << ")" << endl;
    return out;
}
