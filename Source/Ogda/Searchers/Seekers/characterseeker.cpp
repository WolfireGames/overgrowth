//-----------------------------------------------------------------------------
//           Name: characterseeker.cpp
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
#include "characterseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

std::vector<Item> CharacterSeeker::SearchXML( const Item & item, TiXmlDocument& doc )
{
    const char* elems[] = 
    {
        "clothing"
    };

    const char* elems_type[] =
    {
        "material"
    };

    assert(ARRLEN(elems_type) == ARRLEN(elems));

    const char* ignored[] = 
    {
        "morphs",
        "tags",
        "visemes",
        "morphs",
        "team",
        "soundmod"
    };

    std::vector<Item> items;

    TiXmlHandle hRoot(&doc);

    const char* roots[] =
    {
        "character"
    };
    
    TiXmlElement *eRoot = hRoot.FirstChildElement().Element();
    
    if( !eRoot )
    {
        LOGE << "Can't find anything in file listed " << item << std::endl;
    }

    while( eRoot )
    {
        if( FindStringInArray( roots, ARRLEN(roots), eRoot->Value() ) < 0 )
        {
            LOGE << "Unknown root " << eRoot->Value() << " from " << item << std::endl;
        }

        TiXmlElement *eElem = eRoot->FirstChildElement();

        while( eElem )
        {
            const char* name = eElem->Value();
            const char* text = eElem->GetText();
            if( name )
            {
                int id;
                if( (id = FindStringInArray( elems, ARRLEN(elems), name )) >= 0 )
                {
                    if( text && strlen(text) > 0 )
                    {
                        items.push_back(Item(item.input_folder, text,elems_type[id],item.source));
                    }
                    else
                    {
                        LOGW << "String value in " << item << " for element " << elems[id] << " is empty" << std::endl;
                    }
                }
                else if( (id = FindStringInArray( ignored, ARRLEN(ignored), name )) >= 0 )
                {
                    LOGD << "Ignored " << ignored[id] << " in " << item << std::endl;
                }
                else if( strmtch( "animations", name ) )
                {
                    AttributeScanner::DoAllSame( items, item, eElem, "animation" );
                }
                else if( strmtch( "attacks", name ) )
                {
                    AttributeScanner::DoAllSame( items, item, eElem, "attack");
                }
                else if( strmtch( "appearance", name ) )
                {
                    std::vector<attribpair> a;
                    a.push_back(attribpair("obj_path", "object"));
                    a.push_back(attribpair("morph",    "model"));
                    a.push_back(attribpair("skeleton", "skeleton"));
                    a.push_back(attribpair("fur",      "model"));

                    std::vector<const char*> i;
                    i.push_back("model_scale");
                    i.push_back("default_scale");
                    i.push_back("channel_0");
                    i.push_back("channel_1");
                    i.push_back("channel_2");
                    i.push_back("channel_3");
                    i.push_back("channel_4");
                    
                    AttributeScanner::Do( items, item, eElem, a, i );
                }
                else if( strmtch( "voice", name ) )
                {
                    std::vector<attribpair> a;
                    a.push_back(attribpair("path", "voice"));

                    std::vector<const char*> i;
                    i.push_back("pitch");

                    AttributeScanner::Do( items, item, eElem, a, i );
                }
                else
                {
                    LOGE << "Unahandled subvalue in Character from " << item << " called " << name << std::endl;
                }
            }
            else
            {
                LOGE << "Generic warning" << std::endl;
            }

            eElem = eElem->NextSiblingElement();
        }
        eRoot = eRoot->NextSiblingElement();
    }
    return items;
}
