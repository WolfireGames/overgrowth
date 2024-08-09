//-----------------------------------------------------------------------------
//           Name: decalfile.cpp
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
#include "decalfile.h"

#include <Asset/Asset/character.h>
#include <Asset/Asset/material.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <XML/xml_helper.h>
#include <Internal/filesystem.h>
#include <Graphics/shaders.h>
#include <Scripting/scriptfile.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

#include <cmath>
#include <map>
#include <string>

DecalFile::DecalFile(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id), sub_error(0) {
}

int DecalFile::Load(const std::string& path, uint32_t load_flags) {
    sub_error = 0;
    TiXmlDocument doc;
    char abs_path[kPathSize];
    if (FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
        FatalError("Error", "Could not find decal file: %s", path.c_str());
    }
    doc = TiXmlDocument(abs_path);
    doc.LoadFile();
    special_type = 0;

    if (!XmlHelper::getNodeValue(doc, "DecalObject/ColorMap", color_map)) {
        sub_error = 1;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "DecalObject/NormalMap", normal_map)) {
        sub_error = 2;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "DecalObject/ShaderName", shader_name)) {
        sub_error = 3;
        return kLoadErrorMissingSubFile;
    }

    is_shadow = false;

    TiXmlHandle hDoc(&doc);
    TiXmlElement* flags = hDoc.FirstChildElement("DecalObject").FirstChildElement("flags").Element();
    if (flags) {
        const char* tf;
        tf = flags->Attribute("is_shadow");

        if (tf && (strcmp(tf, "true") == 0)) {
            is_shadow = true;
        }
    }

    float val;
    if (XmlHelper::getNodeValue(doc, "DecalObject/SpecialType", val)) {
        special_type = (int)(val + 0.5f);
    }
    return kLoadOk;
}

const char* DecalFile::GetLoadErrorString() {
    switch (sub_error) {
        case 0:
            return "";
        case 1:
            return "Color map not found";
        case 2:
            return "Normal map not found";
        case 3:
            return "Shader file not found";
        default:
            return "Undefined error";
    }
}

void DecalFile::Unload() {
}

void DecalFile::Reload() {
    Load(path_, 0x0);
}

void DecalFile::ReportLoad() {
}

void DecalFile::ReturnPaths(PathSet& path_set) {
    path_set.insert("decal " + path_);
    if (!color_map.empty()) {
        path_set.insert("texture " + color_map);
    }
    if (!normal_map.empty()) {
        path_set.insert("texture " + normal_map);
    }
    if (!shader_name.empty()) {
        path_set.insert("shader " + GetShaderPath(shader_name, Shaders::Instance()->shader_dir_path, _vertex));
        path_set.insert("shader " + GetShaderPath(shader_name, Shaders::Instance()->shader_dir_path, _fragment));
    }
}

AssetLoaderBase* DecalFile::NewLoader() {
    return new FallbackAssetLoader<DecalFile>();
}
