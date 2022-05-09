//-----------------------------------------------------------------------------
//           Name: actors_editor.h
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

#include <Math/overgrowth_geometry.h>
#include <Game/EntityDescription.h>
#include <Online/online_datastructures.h>

#include <string>

class SceneGraph;
class Object;

bool ActorsEditor_AddEntity(SceneGraph* scenegraph, Object* o, std::map<int, int>* id_remap = NULL, bool group_addition = false, bool level_loading = false);
void ActorsEditor_CopySelectedEntities(SceneGraph* scenegraph, EntityDescriptionList* desc_list);
int ActorsEditor_LoadEntitiesFromFile(const std::string& filename, EntityDescriptionList& desc_list, std::string* file_type, Path* res_path);
int ActorsEditor_LoadEntitiesFromFile(const Path& path, EntityDescriptionList& desc_list, std::string* file_type);
std::vector<Object*> ActorsEditor_AddEntitiesAtMouse(const Path& source, SceneGraph* scenegraph, const EntityDescriptionList& desc_list, LineSegment mouseray, bool in_new_prefab);
std::vector<Object*> ActorsEditor_AddEntitiesAtPosition(const Path& source, SceneGraph* scenegraph, const EntityDescriptionList& desc_list, vec3 position, bool in_new_prefab, CommonObjectID hostid = 0);

void ActorsEditor_AddEntitiesIntoPrefab(Object* obj, SceneGraph* scenegraph, const EntityDescriptionList& desc_list);
void ActorsEditor_UnlocalizeIDs(EntityDescriptionList* desc_list, SceneGraph* scenegraph);

void ActorsEditor_AddEntitiesIntoPrefab(Object* obj, const EntityDescriptionList& desc_list);

EntityType GetTypeFromDesc(const EntityDescription& desc);
Object* CreateObjectFromDesc(const EntityDescription& desc);
void LocalizeIDs(EntityDescriptionList* desc_list, bool keep_external_connections);
