//-----------------------------------------------------------------------------
//           Name: terraininfo.cpp
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
#include "terraininfo.h"

#include <Internal/comma_separated_list.h>
#include <Internal/filesystem.h>
#include <Internal/returnpathutil.h>
#include <Internal/levelxml.h>

#include <Asset/Asset/material.h>
#include <Logging/logdata.h>
#include <Game/detailobjectlayer.h>

#include <tinyxml.h>

void TerrainInfo::Print() {
    LOGI << "Minimal: " << minimal << std::endl;
    LOGI << "Heightmap: " << heightmap << std::endl;
    LOGI << "Colormap: " << colormap << std::endl;
    LOGI << "Weightmap: " << weightmap << std::endl;
    LOGI << "Model Override: " << model_override << std::endl;
    LOGI << "Shader Extra: " << shader_extra << std::endl;
    LOGI << "Detail maps:" << std::endl;
    for (auto &i : detail_map_info) {
        i.Print();
    }
    LOGI << "Detail objects: " << std::endl;
    for (auto &i : detail_object_info) {
        LOGI << "Obj path: " << i.obj_path << std::endl;
    }
}

void TerrainInfo::SetDefaults() {
    minimal = false;
    colormap.clear();
    detail_map_info.clear();
    detail_object_info.clear();
    heightmap.clear();
    model_override.clear();
    weightmap.clear();
    shader_extra.clear();
}

void TerrainInfo::ReturnPaths(PathSet &path_set) {
    path_set.insert("heightmap " + heightmap);
    path_set.insert("texture " + colormap);
    if (!weightmap.empty()) {
        path_set.insert("texture " + weightmap);
        path_set.insert("image_sample " + weightmap);
    }
    if (!model_override.empty()) {
        path_set.insert("model " + model_override);
    }
    for (auto &i : detail_map_info) {
        i.ReturnPaths(path_set);
    }
    for (auto &i : detail_object_info) {
        i.ReturnPaths(path_set);
    }
}
