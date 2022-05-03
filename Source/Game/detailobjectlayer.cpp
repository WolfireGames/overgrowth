//-----------------------------------------------------------------------------
//           Name: detailobjectlayer.cpp
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
#include "detailobjectlayer.h"

#include <Asset/Asset/character.h>
#include <Internal/filesystem.h>

#include <tinyxml.h>

#include <string>

DetailObjectLayer ReadDetailObjectLayerXML( const TiXmlElement* detail_object_element) {
    DetailObjectLayer detail_object_layer;
    detail_object_layer.obj_path = SanitizePath(detail_object_element->Attribute("obj_path"));
    {
        const char *str = detail_object_element->Attribute("weight_path");
        if(str){
            detail_object_layer.weight_path = SanitizePath(detail_object_element->Attribute("weight_path"));
        }
    }
    detail_object_element->QueryFloatAttribute("density", &detail_object_layer.density);
    detail_object_element->QueryFloatAttribute("normal_conform", &detail_object_layer.normal_conform);
    detail_object_element->QueryFloatAttribute("min_embed", &detail_object_layer.min_embed);
    detail_object_element->QueryFloatAttribute("max_embed", &detail_object_layer.max_embed);
    detail_object_element->QueryFloatAttribute("min_scale", &detail_object_layer.min_scale);
    detail_object_element->QueryFloatAttribute("max_scale", &detail_object_layer.max_scale);
    detail_object_element->QueryFloatAttribute("view_distance", &detail_object_layer.view_dist);
    detail_object_element->QueryFloatAttribute("overbright", &detail_object_layer.overbright);
    detail_object_element->QueryFloatAttribute("jitter_degrees", &detail_object_layer.jitter_degrees);
    detail_object_element->QueryFloatAttribute("tint_weight", &detail_object_layer.tint_weight);
    {
        const char *chr = detail_object_element->Attribute("collision_type");
        if(chr){
            std::string str(chr);
            if(str == "static") {
                detail_object_layer.collision_type = _static;
            } else if(str == "plant") {
                detail_object_layer.collision_type = _plant;
            }
        }
    }
    return detail_object_layer;
}

bool DetailObjectLayer::operator==( const DetailObjectLayer& other ) const {
    return obj_path == other.obj_path && weight_path == other.weight_path &&
        density == other.density && min_embed == other.min_embed && 
        max_embed == other.max_embed && min_scale == other.min_scale && 
        max_scale == other.max_scale && view_dist == other.view_dist &&
        normal_conform == other.normal_conform && 
        overbright == other.overbright && 
        jitter_degrees == other.jitter_degrees;
}

DetailObjectLayer::DetailObjectLayer():
    density(1.0f),
    normal_conform(0.0f),
    min_embed(0.0f),
    max_embed(0.0f),
    min_scale(1.0f),
    max_scale(1.0f),
    view_dist(30.0f),
    jitter_degrees(0.0f),
    overbright(0.0f),
    tint_weight(1.0f),
    collision_type(_none)
{
}

void DetailObjectLayer::ReturnPaths( PathSet& path_set ) {
    path_set.insert("object "+obj_path);
    path_set.insert("image_sample "+weight_path);
}
