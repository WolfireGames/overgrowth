//-----------------------------------------------------------------------------
//           Name: version.cpp
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
#include "version.h"

#include <Utility/strings.h>

#include <vector>
#include <sstream>
#include <string>

extern const int phoenix_build_id;
extern const char* phoenix_platform;
extern const char* phoenix_arch;
extern const char* phoenix_git_version_string;
extern const char* phoenix_build_timestamp;

const int GetBuildID() {
    return phoenix_build_id;
}

const char* GetBuildIDString() {
    static bool init = false;
    static std::string buf;

    if (phoenix_build_id == -1)
        return "none";

    if (!init) {
        std::stringstream ss;
        ss << "build-" << phoenix_build_id;
        buf = ss.str();
        init = true;
    }

    return buf.c_str();
}

const char* GetPlatform() {
    return phoenix_platform;
}

const char* GetArch() {
    return phoenix_arch;
}

const char* GetBuildVersion() {
    return phoenix_git_version_string;
}

const char* GetBuildTimestamp() {
    return phoenix_build_timestamp;
}

std::string GetShortBuildTag() {
    std::vector<std::string> split_string;
    std::string buildVersion(GetBuildVersion());
    split(buildVersion, '-', split_string);

    return split_string[0];
}

std::string GetVersionMajor() {
    std::vector<std::string> split_string;
    split(GetShortBuildTag(), '.', split_string);

    if (split_string.size() >= 1) {
        return split_string[0];
    } else {
        return std::string();
    }
}

std::string GetVersionMinor() {
    std::vector<std::string> split_string;
    split(GetShortBuildTag(), '.', split_string);

    if (split_string.size() >= 2) {
        return split_string[1];
    } else {
        return std::string();
    }
}

std::string GetVersionPatch() {
    std::vector<std::string> split_string;
    split(GetShortBuildTag(), '.', split_string);
    if (split_string.size() >= 3) {
        return split_string[2];
    } else {
        return std::string();
    }
}

std::string GetShortBuildTimestamp() {
    std::vector<std::string> split_string;
    std::string build_timestamp(GetBuildTimestamp());
    split(build_timestamp, ' ', split_string);

    return split_string[0];
}

const std::string& GetFullBuildString() {
    static bool init = false;
    static std::string buf;

    if (!init) {
        std::stringstream ss;

        ss << phoenix_git_version_string << "_" << phoenix_arch << "_" << phoenix_platform << "_" << phoenix_build_id << "_" << phoenix_build_timestamp;
        init = true;
        buf = ss.str();
    }

    return buf;
}
