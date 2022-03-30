//-----------------------------------------------------------------------------
//           Name: levelassetpreloadparser.h
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

#include <XML/Parsers/xmlparserbase.h>

#include <Asset/assettypes.h>

#include <vector>

class LevelAssetPreloadParser : public XMLParserBase {
public:
    class Asset {
    public:
        Asset();

        std::string path;
        std::string level_name;
        bool all_levels;
        AssetType asset_type;
        uint32_t load_flags;
    };

    std::vector<Asset> assets;
    virtual uint32_t Load( const std::string& path );
    virtual bool Save( const std::string& path );
    virtual void Clear();
};
