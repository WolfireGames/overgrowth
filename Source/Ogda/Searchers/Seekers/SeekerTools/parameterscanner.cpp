//-----------------------------------------------------------------------------
//           Name: parameterscanner.cpp
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
#include "parameterscanner.h"

#include <string>
#include <cassert>

#include <Ogda/jobhandler.h>
#include <Internal/filesystem.h>
#include <Utility/strings.h>
#include <XML/xml_helper.h>
#include <Ogda/Searchers/Seekers/xmlseekerbase.h>

#include <tinyxml.h>

void ParameterScanner::Do(
    std::vector<Item>& items, 
    const Item& item, 
    TiXmlNode *eRoot, 
    const std::vector<parampair>& params, 
    const std::vector<const char*>& params_ignore
    )
{
    if( eRoot )
    {
        TiXmlElement* eElem = eRoot->FirstChildElement();

        while( eElem )
        {
            const char* name = eElem->Attribute("name");
            const char* text = eElem->Attribute("val");

            if( name )
            {
                int id;
                if( (id = FindStringInArray( params, name )) >= 0 )
                {
                    if( strlen( params[id].second ) > 0 )
                    {
                        if( text && strlen(text) > 0 )
                        {
                            items.push_back(Item(item.input_folder,text,params[id].second,item.source));
                        }
                        else
                        {
                            LOGW << "String value in " << item << " for element " << params[id].first << " is empty, row " << eElem->Row() << std::endl;
                        }
                    }
                }
                else if( (id = FindStringInArray( params_ignore, name )) >= 0 )
                {
                    LOGD << "Ignored " << params_ignore[id] << " in " << item << " row " << eElem->Row() << std::endl;
                }
                else
                {
                    LOGE << "Unahandled subvalue from " << item << " called " << name << " row " << eElem->Row() << std::endl;
                }
            }
            else
            {
                LOGE << "Generic warning" << std::endl;
            }

            eElem = eElem->NextSiblingElement();
        }
    }
}
