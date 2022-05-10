//-----------------------------------------------------------------------------
//           Name: casecorrectpath.cpp
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
#include "casecorrectpath.h"

#include <Compat/filepath.h>
#include <Internal/filesystem.h>

#include <cstdio>
#include <string>
#include <cstring>

using std::string;

bool IsPathCaseCorrect(const string& input_path, string* correct_case) {
    char correct_case_buf[kPathSize];
    GetCaseCorrectPath(input_path.c_str(), correct_case_buf);
    *correct_case = correct_case_buf;
    if (input_path.length() != correct_case->length()) {
        return true;  // Something weird happened, don't bother the user with error messages
    }
    if (input_path != *correct_case) {
        return false;
    } else {
        return true;
    }
}
