//-----------------------------------------------------------------------------
//           Name: assetpreload.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
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
#include "assetpreload.h"

#include <Internal/assetmanifest.h>
#include <Internal/filesystem.h>
#include <Internal/path.h>

#include <Images/stbimage_wrapper.h>
#include <XML/Parsers/levelassetspreloadparser.h>

#include <cstring>

Path AssetPreload::ResolveID(const char* id) {
    return Path();
}

std::vector<LevelAssetPreloadParser::Asset>& AssetPreload::GetPreloadFiles() {
    return amp.assets;
}

static AssetPreload asset_manifest;

AssetPreload& AssetPreload::Instance() {
    return asset_manifest;
}

void AssetPreload::Initialize() {
    // Before we load any assets (in preload or otherwise):
    // We set stb_image to flip images vertically to load them how Overgrowth expects (positive Y)
    stbi_set_flip_vertically_on_load(true);
    
    // Now we can finish preloading
    Reload();
}

void AssetPreload::Reload() {
    Path p = FindFilePath("Data/preload.xml", kDataPaths, true);
    if (p.isValid()) {
        amp.Load(p.GetFullPath());
    }
}

void AssetPreload::Dispose() {
}
