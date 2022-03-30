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
#pragma once

// These values are used in TypeEnable::SetFromConfig and TypeEnable::SaveToConfig
enum EntityType{_any_type = 0,
    _no_type = 1,
    _camera_type = 2,
    _terrain_type = 11,
    _env_object = 20,
    _movement_object = 21,
    _spawn_point = 23,
    _decal_object = 24,
    _hotspot_object = 26,
    _group = 29,
    _rigged_object = 30,
    _item_object = 32,
    _path_point_object = 33,
    _ambient_sound_object = 34,
    _placeholder_object = 35,
    _light_probe_object = 36,
	_dynamic_light_object = 37,
    _navmesh_hint_object = 38,
    _navmesh_region_object = 39,
    _navmesh_connection_object = 40,
    _reflection_capture_object = 41,
    _light_volume_object = 42,
    _prefab = 43
};

const char* CStringFromEntityType(EntityType type);
