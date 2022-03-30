//-----------------------------------------------------------------------------
//           Name: decalseeker.cpp
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
#include "decalseeker.h"

#include <tinyxml.h>

#include <Logging/logdata.h>

std::vector<Item> DecalSeeker::SearchXML( const Item& item, TiXmlDocument& doc )
{
    std::vector<Item> items;

    TiXmlHandle root(&doc);

    TiXmlElement* eColormap = root.FirstChildElement("DecalObject").FirstChildElement("ColorMap").Element();
    TiXmlElement* eNormalmap = root.FirstChildElement("DecalObject").FirstChildElement("NormalMap").Element();

    if( eColormap )
    {
        const char* colormap = eColormap->GetText();

        if( colormap && strlen( colormap ) > 0 )
        {
            items.push_back(Item(item.input_folder,colormap,"texture",item.source));
        } 
        else
        {
            LOGE << "ColorMap text missing" << std::endl;
        }
    }
    else
    {
        LOGE << "ColorMap element missing in Decal" << std::endl;
    }

    if( eNormalmap )
    {
        const char* normalmap = eNormalmap->GetText();

        if( normalmap && strlen( normalmap ) > 0 )
        {
            items.push_back(Item(item.input_folder, normalmap,"texture",item.source));
        }
        else
        {
            LOGE << "NormalMap text missing" << std::endl;
        }
    }
    else
    {
        LOGE << "NormalMap element missing in Decal" << std::endl;
    }

    return items;
}
