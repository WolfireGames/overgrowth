//-----------------------------------------------------------------------------
//           Name: entity_type.cpp
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
#include "entity_type.h"

const char* CStringFromEntityType(EntityType type) {
    switch(type){
        case _any_type: return "_any_type"; break;
        case _no_type: return "_no_type"; break;
        case _camera_type: return "_camera_type"; break;
        case _terrain_type: return "_terrain_type"; break;
        case _env_object: return "_env_object"; break;
        case _movement_object: return "_movement_object"; break;
        case _spawn_point: return "_spawn_point"; break;
        case _decal_object: return "_decal_object"; break;
        case _hotspot_object: return "_hotspot_object"; break;
        case _group: return "_editable_entity_group"; break;
        case _prefab: return "_prefab"; break;
        case _rigged_object: return "_rigged_object"; break;
        case _item_object: return "_item_object"; break;
        case _path_point_object: return "_path_point_object"; break;
        case _ambient_sound_object: return "_ambient_sound_object"; break;
        case _placeholder_object: return "_placeholder_object"; break;
        case _light_probe_object: return "_light_probe_object"; break;
		case _dynamic_light_object: return "_dynamic_light_object"; break;
        case _navmesh_hint_object: return "_navmesh_hint_object"; break;
        case _navmesh_region_object: return "_navmesh_region_object"; break;
        case _navmesh_connection_object: return "_navmesh_connection_object"; break;
        case _reflection_capture_object: return "_reflection_capture_object"; break;
        case _light_volume_object: return "_light_volume_object"; break;
        default: return "unknown type"; break;
    }
}
