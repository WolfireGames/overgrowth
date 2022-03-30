//-----------------------------------------------------------------------------
//           Name: objhullseeker.cpp
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
#include "objhullseeker.h"
#include "Internal/filesystem.h"
#include "Logging/logdata.h"

std::vector<Item> ObjHullSeeker::Search(const Item& item )
{
    std::vector<Item> items;
    std::string full_path;

    full_path = item.GetAbsPath();
    full_path = full_path.substr(0,full_path.length()-4) + "HULL.obj";
    if( CheckFileAccess(full_path.c_str()) )
    {
        items.push_back(Item( item.input_folder, item.GetPath().substr(0,item.path.length()-4) + "HULL.obj", "model_hull", item.source ));
    }

    full_path = item.GetAbsPath();
    full_path = full_path.substr(0,full_path.length()-4) + "hull.obj";
    if( CheckFileAccess(full_path.c_str()) )
    {
        items.push_back(Item( item.input_folder, item.GetPath().substr(0,item.path.length()-4) + "hull.obj", "model_hull", item.source ));
    }

    return items;
}

