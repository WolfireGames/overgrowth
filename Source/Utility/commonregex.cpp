//-----------------------------------------------------------------------------
//           Name: commonregex.cpp
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
#include "commonregex.h"

#include <trex/trex.h>

#include <string>

static const std::string true_regex_string("^[Tt]rue|[Yy]es$");
static TRexpp true_regex;
static const std::string false_regex_string("^[Ff]alse|[Nn]o$");
static TRexpp false_regex;

static bool compiled_regexes = false;

static void try_compile() {
    if (!compiled_regexes) {
        true_regex.Compile(true_regex_string.c_str());
        false_regex.Compile(false_regex_string.c_str());
        compiled_regexes = true;
    }
}

CommonRegex::CommonRegex() {
    try_compile();
}

bool CommonRegex::saysTrue(const std::string& str) {
    return true_regex.Match(str.c_str());
}

bool CommonRegex::saysFalse(const std::string& str) {
    return false_regex.Match(str.c_str());
}

bool CommonRegex::saysTrue(const char* str) {
    if (str) {
        return saysTrue(std::string(str));
    } else {
        return false;
    }
}

bool CommonRegex::saysFalse(const char* str) {
    if (str) {
        return saysFalse(std::string(str));
    } else {
        return false;
    }
}
