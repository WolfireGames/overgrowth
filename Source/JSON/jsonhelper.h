//-----------------------------------------------------------------------------
//           Name: jsonhelper.h
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
#pragma once

#include <JSON/json.h>

#include <cstdio>
#include <string>

/**
 * Wrap up the common JSON loading/creation - mostly for Angelscript interface
 **/
class SimpleJSONWrapper {
    
    Json::Value root;
    
public:
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    SimpleJSONWrapper() :
        root( Json::objectValue )
    {}
    
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the root of the object
     *
     * @returns a reference to the root object
     *
     */
    Json::Value& getRoot() {return root; }
    
    /*******************************************************************************************/
    /**
     * @brief  Parses a string containing JSON
     *
     * @returns true if parse ok, false otherwise
     *
     */
    bool parseString( std::string& sourceString );

    /*******************************************************************************************/
    /**
     * @brief  Parses a file, finds the file using the built in FinFilePath routine.
     *
     * @returns true if parse ok, false otherwise
     *
     */
    bool parseFile( std::string& sourceFile );
    
    /*******************************************************************************************/
    /**
     * @brief  Parses a istream file containing json
     *
     * @returns true if parse ok, false otherwise
     *
     */
    bool parseIstream( std::istream& is, std::string& errs );
    
    /*******************************************************************************************/
    /**
     * @brief  Outputs this JSON object to a string
     *
     * @param humanFriendly If true, do some formatting else just output a more machine friendly version
     *
     */
    std::string writeString( bool humanFriendly = false );
    
    
    
};
