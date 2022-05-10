//-----------------------------------------------------------------------------
//           Name: hotspotfile.cpp
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
#include "hotspotfile.h"

#include <Asset/Asset/character.h>
#include <Asset/Asset/material.h>

#include <XML/xml_helper.h>
#include <Internal/filesystem.h>
#include <Graphics/shaders.h>
#include <Scripting/scriptfile.h>
#include <Logging/logdata.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <tinyxml.h>

#include <map>
#include <string>
#include <cmath>

HotspotFile::HotspotFile(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id), sub_error(0) {
}

void HotspotFile::ReturnPaths(PathSet& path_set) {
    path_set.insert("hotspot " + path_);
    path_set.insert("texture " + billboard_color_map);
    path_set.insert("script " + script);
    // Commented out so that 'loadlevel' works properly
    // ScriptFileUtil::ReturnPaths("Data/Scripts/"+script, path_set);
}

int HotspotFile::Load(const std::string& rel_path, uint32_t load_flags) {
    sub_error = 0;
    char abs_path[kPathSize];
    if (FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) == -1) {
        return kLoadErrorMissingFile;
    }
    TiXmlDocument doc(abs_path);
    doc.LoadFile();

    if (!XmlHelper::getNodeValue(doc, "Hotspot/BillboardColorMap", billboard_color_map)) {
        sub_error = 1;
        return kLoadErrorMissingSubFile;
    }
    if (!XmlHelper::getNodeValue(doc, "Hotspot/Script", script)) {
        sub_error = 2;
        return kLoadErrorMissingSubFile;
    }
    return kLoadOk;
}

const char* HotspotFile::GetLoadErrorString() {
    switch (sub_error) {
        case 0:
            return "";
        case 1:
            return "BillboardColorMap not found. Aborting hotspot load.";
        case 2:
            return "Script not found. Aborting hotspot load.";
        default:
            return "Undefined error";
    }
}

void HotspotFile::Unload() {
}

void HotspotFile::Reload() {
}

void HotspotFile::ReportLoad() {
}

AssetLoaderBase* HotspotFile::NewLoader() {
    return new FallbackAssetLoader<HotspotFile>();
}
