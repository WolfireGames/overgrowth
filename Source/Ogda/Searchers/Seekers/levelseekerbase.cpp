//-----------------------------------------------------------------------------
//           Name: levelseekerbase.cpp
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
#include "levelseekerbase.h"

#include <tinyxml.h>

#include <Logging/logdata.h>

std::vector<Item> LevelSeekerBase::SearchXML( const Item& item, TiXmlDocument& doc )
{
    TiXmlElement* eRoot = doc.RootElement();
    TiXmlHandle hRoot(eRoot);
    
    if( strcmp(eRoot->Value(), "Level") == 0 )
    {
        return this->SearchLevelRoot(item, hRoot);
    }
    else
    {
        TiXmlHandle root(&(doc));
        return this->SearchLevelRoot(item, root);
    }
}
