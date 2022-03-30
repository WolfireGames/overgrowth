//-----------------------------------------------------------------------------
//           Name: detailmapinfo.cpp
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "detailmapinfo.h"

#include <Internal/comma_separated_list.h>
#include <Internal/filesystem.h>
#include <Internal/returnpathutil.h>
#include <Internal/levelxml.h>

#include <Asset/Asset/material.h>
#include <Logging/logdata.h>
#include <Game/detailobjectlayer.h>
#include <Main/engine.h>

#include <tinyxml.h>

void DetailMapInfo::Print()
{
    LOGI << "Colorpath: " <<  colorpath << std::endl;
    LOGI << "Normalpath: " << normalpath << std::endl;
    LOGI << "Materialpath: " <<  materialpath << std::endl;
}

void DetailMapInfo::ReturnPaths( PathSet &path_set )
{
    path_set.insert("texture "+colorpath);
    path_set.insert("texture "+normalpath);
    //Materials::Instance()->ReturnRef(materialpath)->ReturnPaths(path_set);
    Engine::Instance()->GetAssetManager()->LoadSync<Material>(materialpath)->ReturnPaths(path_set);
}
