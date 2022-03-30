//-----------------------------------------------------------------------------
//           Name: animationretargetseeker.cpp
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
#include "animationretargetseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

std::vector<Item> AnimationRetargetSeeker::SearchXML( const Item & item, TiXmlDocument& doc )
{
    std::vector<Item> items;

    TiXmlHandle hRoot(&doc);
    TiXmlElement *eRoot = hRoot.FirstChildElement().Element();

    std::vector<elempair> elems;
    elems.push_back( elempair( "rig", "" ) );
    
    std::vector<const char*> elems_ignore;
    
    if( !eRoot )
    {
        LOGE << "Can't find anything in file listed " << item << std::endl;
    }
    else
    {
        ElementScanner::Do( items, item, eRoot, elems, elems_ignore, this, (void*)1 );
    }

    return items;
}

void AnimationRetargetSeeker::HandleElementCallback( std::vector<Item>& items, TiXmlNode* eRoot, TiXmlElement* eElem, const Item& item, void* userdata )
{
    if( userdata == (void*)1 && strmtch(eElem->Value(), "rig") )
    {
        {
            std::vector<elempair> elems;
            elems.push_back( elempair( "anim", "animation" ) );
            std::vector<const char*> elems_ignore;
            ElementScanner::Do( items, item, eElem, elems, elems_ignore, this, (void*)2 );
        }
        {
            std::vector<attribpair> elems;
            elems.push_back( attribpair( "path", "skeleton" ) );
            std::vector<const char*> elems_ignore;
            AttributeScanner::Do(items, item, eElem, elems, elems_ignore);
        }
    }
    else if( userdata == (void*)2 )
    {
    }
    else
    {
        XMLSeekerBase::HandleElementCallback( items, eRoot, eElem, item, userdata );
    }
}
