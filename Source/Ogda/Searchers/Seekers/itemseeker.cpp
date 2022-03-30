//-----------------------------------------------------------------------------
//           Name: itemseeker.cpp
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
#include "itemseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

enum
{
    ROOT = 1,
    ITEM,
    ATTACHMENT
};

std::vector<Item> ItemSeeker::SearchXML( const Item & item, TiXmlDocument& doc )
{
    std::vector<Item> items;

    std::vector<elempair> elems;
    elems.push_back( elempair( "item", "" ) );
    elems.push_back( elempair( "attachment", "" ) );
    
    std::vector<const char*> elems_ignore;
    
    ElementScanner::Do( items, item, &doc, elems, elems_ignore, this, (void*)ROOT );

    return items;
}

void ItemSeeker::HandleElementCallback( std::vector<Item>& items, TiXmlNode* eRoot, TiXmlElement* eElem, const Item& item, void* userdata )
{
    if( userdata == (void*)ROOT )
    {
        if( strmtch(eElem->Value(), "item") )
        {
            std::vector<elempair> elems;
            elems.push_back( elempair( "appearance", "" ) );
            elems.push_back( elempair( "grip", "" ) );
            elems.push_back( elempair( "anim_blend", "" ) );
            elems.push_back( elempair( "attack_override", "" ) );
            elems.push_back( elempair( "anim_override", "" ) );
            elems.push_back( elempair( "reaction_override", "" ) );
            elems.push_back( elempair( "sheathe",""));
            elems.push_back( elempair( "attachments",""));
            
            std::vector<const char*> elems_ignore;
            elems_ignore.push_back("type");
            elems_ignore.push_back("points");
            elems_ignore.push_back("lines");
            elems_ignore.push_back("physics");
            elems_ignore.push_back("label");
            elems_ignore.push_back("range");
            elems_ignore.push_back("anim_override_flags");
            
            ElementScanner::Do( items, item, eElem, elems, elems_ignore, this, (void*)ITEM );
        }
        else if( strmtch(eElem->Value(), "attachment") )
        {
            std::vector<elempair> elems;
            elems.push_back( elempair( "anim", "animation" ) );
            
            std::vector<const char*> elems_ignore;
            elems_ignore.push_back( "attach" );
            elems_ignore.push_back( "mirror" );
            
            ElementScanner::Do( items, item, eElem, elems, elems_ignore, this, (void*)ATTACHMENT );
        }
        else
        {
            LOGE << "Unknown item sub"  << eElem->Value() <<  " " << item << std::endl;
        }
    }
    else if( userdata == (void*)ITEM )
    {
        std::vector<attribpair> elems;
        std::vector<const char*> elems_ignore;

        if( strmtch(eElem->Value(), "appearance") )
        {
            elems.push_back(attribpair("obj_path", "object"));
        }
        else if( strmtch( eElem->Value(), "grip" ) )
        {
            elems.push_back(attribpair("anim", "animation"));
            elems.push_back(attribpair("anim_base", "animation"));
            
            elems_ignore.push_back("ik_attach");
            elems_ignore.push_back("hands");
        }
        else if( strmtch( eElem->Value(), "anim_blend" ) )
        {
            elems.push_back(attribpair("idle", "animation"));
            elems.push_back(attribpair("movement", "animation"));
        }
        else if( strmtch( eElem->Value(), "attack_override" ) )
        {
            elems.push_back(attribpair("stationary", "attack"));
            elems.push_back(attribpair("moving", "attack"));
            elems.push_back(attribpair("moving_close", "attack"));
            elems.push_back(attribpair("stationary_close", "attack"));
            elems.push_back(attribpair("low", "attack"));
        }
        else if( strmtch( eElem->Value(), "anim_override" ) )
        {
            elems.push_back(attribpair("idle", "animation"));
            elems.push_back(attribpair("movement", "animation"));
            elems.push_back(attribpair("medleftblock", "animation"));
            elems.push_back(attribpair("medrightblock", "animation"));
            elems.push_back(attribpair("highleftblock", "animation"));
            elems.push_back(attribpair("highrightblock", "animation"));
            elems.push_back(attribpair("lowleftblock", "animation"));
            elems.push_back(attribpair("lowrightblock", "animation"));
            elems.push_back(attribpair("blockflinch", "animation"));
        }
        else if( strmtch( eElem->Value(), "reaction_override" ) )
        {
            std::vector<elempair> e;
            e.push_back( elempair( "reaction", "" ) );
            
            std::vector<const char*> ei;
            
            ElementScanner::Do( items, item, eElem, e, ei, this, (void*)ITEM );
        }
        else if( strmtch( eElem->Value(), "reaction" ) )
        {
            elems.push_back(attribpair("old", "attack"));
            elems.push_back(attribpair("new", "attack"));
        }
        else if( strmtch( eElem->Value(), "sheathe" ) )
        {
            elems.push_back(attribpair("anim", "animation"));
            elems.push_back(attribpair("anim_base", "animation"));
            elems.push_back(attribpair("contains", "item"));

            elems_ignore.push_back("ik_attach"); 
        } 
        else if( strmtch( eElem->Value(), "attachments" ) )
        {
            std::vector<elempair> e;
            e.push_back( elempair( "attachment", "item" ) );
            
            std::vector<const char*> ei;
            
            ElementScanner::Do( items, item, eElem, e, ei, this, (void*)0 );
        }
        else
        {
            LOGE << "Unknown item sub"  << eElem->Value() <<  " " << item << std::endl;
        }

        AttributeScanner::Do( items, item, eElem, elems, elems_ignore);
    }
    else if( userdata == (void*)ATTACHMENT )
    {
        if( strmtch(eElem->Value(), "anim") )
        {
            //Ignore
        }
        else
        {
            LOGE << "Unknown item sub"  << eElem->Value() <<  " " << item << std::endl;
        }
    }
    else if( userdata == (void*)0 )
    {

    }
    else
    {
        XMLSeekerBase::HandleElementCallback( items, eRoot, eElem, item, userdata );
    }
}
