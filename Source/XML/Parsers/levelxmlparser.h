//-----------------------------------------------------------------------------
//           Name: levelxmlparser.h
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

//Note that this isn't used for level loading currently, only for extracting information from a levelxml file.
//ofc this comment should be remove if this changes. The actual loading is in LevelLoader.cpp /Max
class LevelXMLParser : public XMLParserBase
{
public:
    struct Terrain
    {
        class DetailMap
        {
            std::string colorpath;
            std::string normalpath;
            std::string materialpath;
        };

        std::string heightmap;
        std::string detailmap;
        std::string colormap;
        std::string weightmap;

    };

    struct LoadingScreen
    {
        std::string image;
    };

    uint32_t Load( const std::string& path ) override;
    bool Save( const std::string& path ) override;
    void Clear() override;

    std::string hash;
    std::string name;
    std::string description;
    std::string shader;
    std::string script;
    std::string player_script;
    std::string enemy_script;

    LoadingScreen loading_screen;
};
