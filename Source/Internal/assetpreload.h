//-----------------------------------------------------------------------------
//           Name: assetpreload.h
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
#pragma once

#include <Internal/path.h>
#include <XML/Parsers/levelassetspreloadparser.h>


#include <vector>
#include <map>
#include <string>

/* This class isn't a complete one, but it does do partial resolves 
 * It also supports retriving assets that should be pre-loaded, commonly used in all or some levels 
 */
class AssetPreload {

    std::map<std::string,std::string> id_asset_map;

    LevelAssetPreloadParser amp;

public:
    Path ResolveID(const char* id);
    std::vector<LevelAssetPreloadParser::Asset> &GetPreloadFiles();

    static AssetPreload& Instance();

    void Reload();
    void Initialize();
    void Dispose();
};
