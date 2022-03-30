//-----------------------------------------------------------------------------
//           Name: searcher.cpp
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
#include "searcher.h"

#include <string>

#include <Ogda/jobhandler.h>

#include <tinyxml.h>
#include <XML/xml_helper.h>
#include <Utility/strings.h>


//Small convinence function so i don't write this block multiple times.
void ai(JobHandler& j, const char* t, const char* type)
{
    if(t)
    {
        std::string ts(t);
        if( !ts.empty() )
        {
            //j.AddItem(ts,type);
            return;
        }
    }
    LOGW << "Skipping ItemTextureHandler as string value is nonexistant or empty" << std::endl;
}

//Small convinence function so i don't write this block multiple times.
void aie( JobHandler& j, TiXmlElement* e, const char* type )
{
    if(e)
    {
        ai(j, e->GetText(), type);
    }
}

Searcher::Searcher( SeekerBase* seeker, const std::string& _path_ending, const std::string& type_pattern_re )
: seeker(seeker), path_ending(_path_ending)
{
    try
    {
        type_pattern->Compile(type_pattern_re.c_str());  
    } 
    catch( const TRexParseException& pe )
    {
        LOGE << "Failed to compile the type_pattern regex " << type_pattern_re << " reason: " << pe.desc << std::endl;
    }
}

bool Searcher::IsMatch(const Item& t)
{
    if( endswith(t.GetPath().c_str(), path_ending.c_str()) && type_pattern->Match(t.type.c_str()) )
        return true;
    else
        return false;
}

std::vector<Item> Searcher::TrySearch( JobHandler& jh, const Item& item, int* matchcounter )
{
    if( IsMatch(item) )
    {
        LOGD << "Running " << seeker->GetName() << " on " << item << std::endl;
        (*matchcounter)++;
        return seeker->Search(item);
    }
    else
    {
        LOGD << "Skipping " << seeker->GetName() << " on " << item << ", doesn't match pattern." << std::endl;
        return std::vector<Item>();
    }
}

std::string Searcher::GetSeekerName()
{
    return std::string(seeker->GetName()); 
}
