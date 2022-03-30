//-----------------------------------------------------------------------------
//           Name: terraininfo.h
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

#include <Game/EntityDescription.h>
#include <Game/detailobjectlayer.h>

#include <Math/mat4.h>
#include <Math/quaternions.h>

#include <Asset/assets.h>
#include <Scripting/scriptparams.h>
#include <Graphics/detailmapinfo.h>

struct TerrainInfo {
    bool minimal;
    std::string heightmap;
    std::string colormap;
    std::string weightmap;
    std::string model_override;
    std::string shader_extra;
    std::vector<DetailMapInfo> detail_map_info;
    std::vector<DetailObjectLayer> detail_object_info;
    void Print();
    void SetDefaults();
    void ReturnPaths(PathSet &path_set);
};
