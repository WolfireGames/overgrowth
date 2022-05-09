//-----------------------------------------------------------------------------
//           Name: lipsyncsystem.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "lipsyncsystem.h"

#include <Internal/filesystem.h>
#include <Compat/fileio.h>
#include <Internal/error.h>

#include <iostream>

LipSyncSystem::LipSyncSystem() {
    const char* rel_path = "Data/Sounds/voice/phonemes.txt";
    char abs_path[kPathSize];
    FindFilePath(rel_path, abs_path, kPathSize, kDataPaths | kModPaths);
    std::ifstream file;
    my_ifstream_open(file, abs_path);
    if (file.fail()) {
        FatalError("Error", "Could not open phonemes file: %s", rel_path);
    }

    // Create 3 maps that map a phoneme string to a viseme id
    // for 9, 12 or 17 visemes
    phn2vis.resize(3);
    vis2phn.resize(3);
    char the_char;
    std::string label;
    int level = 0;
    int group[3] = {-1, -1, -1};
    while (!file.eof()) {
        file.get(the_char);
        if (the_char == '\n' || the_char == ' ') {
            continue;
        }
        if (the_char == '[') {
            ++group[level];
            ++level;
            continue;
        }
        if (the_char == ']' || the_char == ',') {
            if (!label.empty()) {
                for (int i = 0; i < 3; ++i) {
                    phn2vis[i][label] = group[i];
                    vis2phn[i][group[i]] = label;
                }
                label.clear();
            }
            if (the_char == ']') {
                --level;
            }
            continue;
        }
        if (the_char == '\r') {
            continue;
        }
        label.push_back(the_char);
    }
}
