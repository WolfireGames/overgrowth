//-----------------------------------------------------------------------------
//           Name: levelinfo.cpp
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
#include "levelinfo.h"

#include <Internal/levelxml.h>
#include <Internal/comma_separated_list.h>
#include <Internal/filesystem.h>
#include <Internal/returnpathutil.h>
#include <Internal/profiler.h>

#include <Asset/Asset/material.h>
#include <Logging/logdata.h>
#include <Game/detailobjectlayer.h>

#include <tinyxml.h>

void LevelInfo::Print()
{
    LOGI << "XML version: " << xml_version_ << std::endl;
    LOGI << "Level name: " << level_name_ << std::endl;
    LOGI << "Shader: " << shader_ << std::endl;
    LOGI << "Script: " << script_ << std::endl;
    terrain_info_.Print();

    for(unsigned i=0; i<ambient_sounds_.size(); ++i){
        LOGI << "Ambient sound: " << ambient_sounds_[i] << std::endl;
    }

    sky_info_.Print();

    for(unsigned i=0; i<desc_list_.size(); ++i){
        LOGI << "EntityDesc" << std::endl;
    }
}

void LevelInfo::SetDefaults()
{
    xml_version_.clear();
    level_name_.clear();
    shader_ = "post";
    ambient_sounds_.clear();
    terrain_info_.SetDefaults();
    out_of_date_info_.ao = true;
    out_of_date_info_.nav_mesh = true;
    out_of_date_info_.shadow = true;
    sky_info_.SetDefaults();
    desc_list_.clear();
    script_.clear();
    nav_mesh_parameters_ = NavMeshParameters();      
    shadows_ = true;
}

void LevelInfo::ReturnPaths( PathSet &path_set )
{
    static const int kBufSize = 256;
    char buf[kBufSize];
    path_set.insert("level "+path_);
    for(unsigned i=0; i<ambient_sounds_.size(); ++i){
        path_set.insert("sound "+ambient_sounds_[i]+"_1.wav");
        path_set.insert("sound "+ambient_sounds_[i]+"_2.wav");
        path_set.insert("sound "+ambient_sounds_[i]+"_3.wav");
    }
    terrain_info_.ReturnPaths(path_set);
    sky_info_.ReturnPaths(path_set);
    for(unsigned i=0; i<script_paths_.size(); ++i){
        FormatString(buf, kBufSize, "Finding script paths: %s", script_paths_[i].first.c_str());
        PROFILER_ZONE_DYNAMIC_STRING(g_profiler_ctx, buf);
        ReturnPathUtil::ReturnPathsFromPath(script_paths_[i].second, path_set);
    }
    for(unsigned i=0; i<desc_list_.size(); ++i){
        FormatString(buf, kBufSize, "Finding desc_list paths: %d", i);
        PROFILER_ZONE_DYNAMIC_STRING(g_profiler_ctx, buf);
        desc_list_[i].ReturnPaths(path_set);
    }
}
