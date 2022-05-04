//-----------------------------------------------------------------------------
//           Name: jsonhelper.cpp
//      Developer: Wolfire Games LLC
//         Author: Micah J Best
//           Date: 2015-10-09.
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
#include "jsonhelper.h"

#include <Logging/logdata.h>
#include <Compat/fileio.h>
#include <Internal/filesystem.h>

#include <iostream>

void testJSON() {
    
    std::string testString = "{ \"encoding\" : \"UTF-8\", \"plug-ins\" : [ \"python\", \"c++\", \"ruby\" ],\"indent\" : { \"length\" : 3, \"use_space\": true } }";
    
    Json::Value root;
    
    //Json::Value::Members members = root.getMemberNames();

    Json::Reader reader;
    
    if( reader.parse(testString, root ) ) {
        std::cout << "Parse ok" << std::endl;
    }
    else {
        std::cout << "Parse NOT ok" << std::endl;
    }
    
    Json::ValueType type = root.type();
    
    std::cout << "Type is: " << type << std::endl;
    
    //std::cout << root.asString() << std::endl;
    
    Json::Value::Members members = root.getMemberNames();
    
    for(auto & member : members)
    {
        std::cout << member << std::endl;
    }
    
    
}

/*******
 *
 * SimpleJSONWrapper
 *
 */
bool SimpleJSONWrapper::parseString( std::string& sourceString ) {
    
    Json::Reader reader;
    
    return reader.parse( sourceString, root );

}

bool SimpleJSONWrapper::parseFile( std::string& sourceFile ) {
    char path[kPathSize];
    if( FindFilePath( sourceFile.c_str(), path, kPathSize, kAnyPath, true ) == -1 )
    {
        return false;
    }
    else
    {
        std::string err;
        std::ifstream f; 
        my_ifstream_open(f, path, std::ifstream::in | std::ifstream::binary);
        if( parseIstream(f,err) )
        {
            return true;
        }
        else
        {
            LOGE << "Error parsing json file: " << path << " error: " << err << std::endl;
            return false;
        }
    }
}

bool SimpleJSONWrapper::parseIstream( std::istream& is, std::string& errs ) {
    Json::CharReaderBuilder rbuilder;

    return Json::parseFromStream(rbuilder, is, &root, &errs);
}


std::string SimpleJSONWrapper::writeString( bool humanFriendly ) {

    if( humanFriendly ) {
        Json::StyledWriter writer;
        return writer.write( root );
    }
    else {
        Json::FastWriter writer;
        return writer.write( root );
    }
}

