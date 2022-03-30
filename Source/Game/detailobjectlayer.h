//-----------------------------------------------------------------------------
//           Name: detailobjectlayer.h
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

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>

#include <Math/vec3.h>
#include <Graphics/palette.h>

#include <string>
#include <vector>

enum CollisionType {_none, _static, _plant};

struct DetailObjectLayer {
    DetailObjectLayer();
    std::string obj_path;
    std::string weight_path;
    float density;
    float normal_conform;
    float min_embed;
    float max_embed;
    float min_scale;
    float max_scale;
    float view_dist;
    float jitter_degrees;
    float overbright;
    float tint_weight;
    CollisionType collision_type;
    bool operator==( const DetailObjectLayer& other ) const;
    void ReturnPaths(PathSet& path_set);
};

DetailObjectLayer ReadDetailObjectLayerXML( const TiXmlElement* detail_object_element);
