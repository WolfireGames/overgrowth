//-----------------------------------------------------------------------------
//           Name: ogda_hash.cpp
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
#include "ogda_config.h"
#include <iostream>

#include <Utility/hash.h>
#include <Internal/filesystem.h>
#include <Logging/logdata.h>
#include <Internal/datemodified.h>
#include <Ogda/ogda_config.h>

std::string OgdaGetFileHash(std::string path)
{
    if( config_use_date_modified_as_hash )
    {
        std::stringstream ss;
        ss << GetDateModifiedInt64(path.c_str());
        return ss.str();
    }
    else
    {
        return GetFileHash(path.c_str()).ToString();
    }
}
