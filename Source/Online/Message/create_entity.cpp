//-----------------------------------------------------------------------------
//           Name: create_entity.cpp
//      Developer: Wolfire Games LLC
//         Author: Max Danielsson
//    Description: Create entity message, sent by the host to clients to inform
//                 them of construction of a new entity.
//
//                 Clients will request creation of an entity to the host via a
//                 different message.
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
#include "create_entity.h"

#include <Online/online.h>
#include <Main/engine.h>
#include <Editors/map_editor.h>

namespace OnlineMessages {
CreateEntity::CreateEntity(const string& path, const vec3& pos, ObjectID entity_object_id) : OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
                                                                                             path(path),
                                                                                             pos(pos),
                                                                                             entity_object_id(entity_object_id) {
}

binn* CreateEntity::Serialize(void* object) {
    CreateEntity* ce = static_cast<CreateEntity*>(object);
    binn* l = binn_object();

    binn_object_set_std_string(l, "path", ce->path);
    binn_object_set_vec3(l, "pos", ce->pos);
    binn_object_set_int32(l, "id", ce->entity_object_id);

    return l;
}

void CreateEntity::Deserialize(void* object, binn* l) {
    CreateEntity* ce = static_cast<CreateEntity*>(object);

    binn_object_get_std_string(l, "path", &ce->path);
    binn_object_get_vec3(l, "pos", &ce->pos);
    binn_object_get_int32(l, "id", &ce->entity_object_id);
}

void CreateEntity::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    CreateEntity* ce = static_cast<CreateEntity*>(object);
    SceneGraph* sg = Engine::Instance()->GetSceneGraph();
    Online* online = Online::Instance();

    if (sg != nullptr) {
        sg->map_editor->CreateObjectFromHost(ce->path, ce->pos, ce->entity_object_id);
    }
}

void* CreateEntity::Construct(void* mem) {
    return new (mem) CreateEntity("", vec3(), -1);
}

void CreateEntity::Destroy(void* object) {
    CreateEntity* ce = static_cast<CreateEntity*>(object);
    ce->~CreateEntity();
}
}  // namespace OnlineMessages
