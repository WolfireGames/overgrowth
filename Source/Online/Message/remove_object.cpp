//-----------------------------------------------------------------------------
//           Name: remove_object.cpp
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
#include "remove_object.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Editors/map_editor.h>

namespace OnlineMessages {
    RemoveObject::RemoveObject(ObjectID id, EntityType type) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
        type(type)
    {
        this->id = Online::Instance()->GetOriginalID(id);
    }

    binn* RemoveObject::Serialize(void* object) {
        RemoveObject* ro = static_cast<RemoveObject*>(object);

        binn* l = binn_object();

        binn_object_set_int32(l, "id", ro->id);
        binn_object_set_int32(l, "type", ro->type);

        return l;
    }

    void RemoveObject::Deserialize(void* object, binn* l) {
        RemoveObject* ro = static_cast<RemoveObject*>(object);

        binn_object_get_int32(l, "id", &ro->id);
        int32_t type;
        binn_object_get_int32(l, "type", &type);
        ro->type = (EntityType)type;
    }

    void RemoveObject::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
        RemoveObject* ro = static_cast<RemoveObject*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(ro->id);
        SceneGraph* sg = Engine::Instance()->GetSceneGraph();

        if(sg != nullptr) {
            Object* o = sg->GetObjectFromID(object_id);
            if (o != nullptr) {
                sg->map_editor->RemoveObject(o, sg, true);
                Online::Instance()->DeRegisterHostClientIDTranslation(object_id);
            } else {
                LOGW << "Was asked to delete object: " << object_id << " (" << ro->id << "), but was unable to find it";
            }
        }
    }

    void* RemoveObject::Construct(void* mem) {
        return new(mem) RemoveObject((ObjectID)-1,(EntityType)0);
    }

    void RemoveObject::Destroy(void* object) {
        RemoveObject* ro = static_cast<RemoveObject*>(object);
        ro->~RemoveObject();
    }
}
