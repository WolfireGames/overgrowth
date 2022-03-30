//-----------------------------------------------------------------------------
//           Name: spawneritem.cpp
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
#include "spawneritem.h"

SpawnerItem::SpawnerItem( std::string _mod_source_title, std::string _display_name, std::string _path, std::string _thumbnail ) : 
    mod_source_title(_mod_source_title),
    display_name(_display_name),
    path(_path),
    thumbnail_path(_thumbnail)
{

}

SpawnerItem::SpawnerItem() { }

bool operator==( const SpawnerItem& a, const SpawnerItem& b ) {
    return a.display_name == b.display_name && a.path == b.path && a.thumbnail_path == b.thumbnail_path;
}
