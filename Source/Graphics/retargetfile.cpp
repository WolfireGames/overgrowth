//-----------------------------------------------------------------------------
//           Name: retargetfile.cpp
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
#include "retargetfile.h"

#include <Internal/error.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Memory/allocation.h>
#include <XML/xml_helper.h>

#include <tinyxml.h>

void AnimationRetargeter::Reload() {
    int64_t new_date_modified = GetDateModifiedInt64(path.c_str());
    if (new_date_modified > date_modified) {
        Load(path.c_str());
    }
}

void AnimationRetargeter::Load(const char* _path) {
    static const int kBufSize = 512;
    char buf[kBufSize];
    if (FindFilePath(_path, buf, kBufSize, kDataPaths | kModPaths) == -1) {
        FatalError("Error", "Could not find: %s", _path);
    }
    void* mem;
    if (!LoadText(mem, buf)) {
        FatalError("Error", "Could not read: %s", _path);
    }
    TiXmlDocument doc;
    doc.Parse((const char*)mem);
    OG_FREE(mem);

    if (doc.Error()) {
        LOGE << "Unable to load xml file " << _path << ". Error: \"" << doc.ErrorDesc() << "\" on row " << doc.ErrorRow() << std::endl;
    } else {
        path = buf;
        date_modified = GetDateModifiedInt64(buf);

        anim_skeleton.clear();

        TiXmlHandle h_doc(&doc);
        TiXmlHandle h_root = h_doc.FirstChildElement();
        TiXmlElement* field = h_root.ToElement()->FirstChildElement();
        if (!field) {
            FatalError("Error", "Retargeter file has no first field");
        }
        for (; field; field = field->NextSiblingElement()) {
            std::string field_str(field->Value());
            if (field_str == "rig") {
                const char* path_str = field->Attribute("path");
                if (!path_str) {
                    FatalError("Error", "Retargeter field has no path attribute");
                }
                std::string skeleton_path = path_str;
                TiXmlElement* anim = field->FirstChildElement();
                while (anim) {
                    anim_skeleton[anim->GetText()] = skeleton_path;
                    const char* path_str = anim->Attribute("no_retarget");
                    if (path_str && strcmp(path_str, "true") == 0) {
                        anim_no_retarget[anim->GetText()] = true;
                    }
                    anim = anim->NextSiblingElement();
                }
            }
        }
    }
}

const std::string& AnimationRetargeter::GetSkeletonFile(const std::string& anim) {
    std::map<std::string, std::string>::iterator iter = anim_skeleton.find(anim);
    if (iter == anim_skeleton.end()) {
        iter = anim_skeleton.find("Data/Animations/r_ledge.anm");
        if (iter == anim_skeleton.end()) {
            iter = anim_skeleton.begin();
        }
    }
    return iter->second;
}

bool AnimationRetargeter::GetNoRetarget(const std::string& anim) {
    std::map<std::string, bool>::iterator iter = anim_no_retarget.find(anim);
    if (iter != anim_no_retarget.end()) {
        return iter->second;
    } else {
        return false;
    }
}
