//-----------------------------------------------------------------------------
//           Name: retargetfile.h
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
#pragma once

#include <Internal/datemodified.h>

#include <map>
#include <string>

class AnimationRetargeter {
    std::string path;
    int64_t date_modified;
    std::map<std::string, bool> anim_no_retarget;
    std::map<std::string, std::string> anim_skeleton;
public:
    static AnimationRetargeter *Instance() {
        static AnimationRetargeter anim_retargeter;
        return &anim_retargeter;
    }

    void Load(const char* path);
    void Reload();
    const std::string &GetSkeletonFile(const std::string &anim);
    bool GetNoRetarget(const std::string &anim);
};
