//-----------------------------------------------------------------------------
//           Name: terrainlevelseeker.cpp
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
#include "terrainlevelseeker.h"

#include <Utility/strings.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

std::vector<Item> TerrainLevelSeeker::SearchLevelRoot( const Item& item, TiXmlHandle& hRoot )
{
    std::vector<Item> items;
    const char* simpleTextures[] = {
        "Heightmap",
        "DetailMap",
        "ColorMap",
        "WeightMap",
        "ModelOverride"
    };

    const char* type[] = 
    {
        "heightmap",
        "texture",
        "raw_texture",
        "texture",
        "model"
    };

    assert( ARRLEN(type) == ARRLEN(simpleTextures) );
    
    TiXmlElement *eTerrain = hRoot.FirstChildElement("Terrain").Element();

    if( eTerrain )
    {
        TiXmlElement *eElem = eTerrain->FirstChildElement();

        while( eElem )
        {
            const char* name = eElem->Value();
            int id;
            if( (id = FindStringInArray( simpleTextures, ARRLEN(simpleTextures), name )) >= 0 )
            {
                const char* v = eElem->GetText();
                if(v != NULL && strlen(v) > 0)
                {
                    items.push_back(Item(item.input_folder, v,type[id], item.source));
                }
            }
            else if( strmtch( name, "DetailMaps" ) )
            {
                TiXmlElement *eDetailMap = eElem->FirstChildElement();

                while( eDetailMap )
                {
                    std::vector<attribpair> attribs;
                    attribs.push_back(attribpair("colorpath",   "texture"));
                    attribs.push_back(attribpair("normalpath",  "texture"));
                    attribs.push_back(attribpair("materialpath","material"));

                    std::vector<const char*> ignore;

                    AttributeScanner::Do( items, item, eDetailMap, attribs, ignore );

                    eDetailMap = eDetailMap->NextSiblingElement();
                }
            }
            else if( strmtch( name, "DetailObjects" ) )
            {
                TiXmlElement *eDetailObject = eElem->FirstChildElement();

                while( eDetailObject )
                {
                    //<DetailObject obj_path="Data/Objects/Plants/Groundcover/Grass/WildGrass.xml" weight_path="Data/Textures/Terrain/scrubby_hills/scrubby_hills_grass.png" normal_conform="0.5" density="7" min_embed="0" max_embed="0.4" min_scale="0.7" max_scale="2" view_distance="30" jitter_degrees="10" overbright="0" />

                    {
                        const char* obj_path = eDetailObject->Attribute("obj_path");
                        if( obj_path && strlen( obj_path ) > 0 )
                        {
                            items.push_back(Item(item.input_folder,obj_path,"object",item.source));
                        }
                        else
                        {
                            LOGW << "Missing expected attribute obj_path." << std::endl;
                        }
                    }
       
                    { 
                        const char* weight_path = eDetailObject->Attribute("weight_path");
                        if( weight_path && strlen( weight_path ) > 0 )
                        {
                            items.push_back(Item(item.input_folder,weight_path,"texture",item.source));
                        }
                        else
                        {
                            LOGW << "Missing expected attribute weight_path." << std::endl;
                        }
                    }

                    eDetailObject = eDetailObject->NextSiblingElement(); 
                }
            }
            else if( strmtch( name, "ShaderExtra" ) ) 
            {

            }
            else
            {
                LOGW << "Missing subhandler for " << name << " in Terrain under level" << item << std::endl;
            }

            eElem = eElem->NextSiblingElement();
        }
    }

    return items;
}
