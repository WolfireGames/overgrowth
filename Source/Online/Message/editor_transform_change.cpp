//-----------------------------------------------------------------------------
//           Name: editor_transform_change.cpp
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
#include "editor_transform_change.h"

#include <Main/engine.h>
#include <Online/online.h>

namespace OnlineMessages {
    EditorTransformChange::EditorTransformChange(ObjectID id, const vec3& trans, const vec3& scale, const quaternion& rot) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        trans(trans), scale(scale), rot(rot)
    {
        this->id = Online::Instance()->GetOriginalID(id);
    }

    binn* EditorTransformChange::Serialize(void* object) {
        EditorTransformChange* etc = static_cast<EditorTransformChange*>(object);

        binn* l = binn_object();

        binn_object_set_int32(l, "id", etc->id);
        binn_object_set_vec3(l, "t", etc->trans);
        binn_object_set_vec3(l, "s", etc->scale);
        binn_object_set_quat(l, "r", etc->rot);

        return l;
    }

    void EditorTransformChange::Deserialize(void* object, binn* l) {
        EditorTransformChange* etc = static_cast<EditorTransformChange*>(object);

        binn_object_get_int32(l, "id", &etc->id);
        binn_object_get_vec3(l, "t", &etc->trans);
        binn_object_get_vec3(l, "s", &etc->scale);
        binn_object_get_quat(l, "r", &etc->rot);
    }

    void EditorTransformChange::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
        EditorTransformChange* etc = static_cast<EditorTransformChange*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(etc->id);

        SceneGraph* sg = Engine::Instance()->GetSceneGraph();

        if (sg != nullptr) {
            Object * obj = sg->GetObjectFromID(object_id);
            if (obj != nullptr) {
                obj->SetTranslation(etc->trans);
                obj->SetRotation(etc->rot);
                obj->SetScale(etc->scale);

                if (obj->GetType() == EntityType::_env_object) {
                    EnvObject* envobj = (EnvObject *)obj;

                    envobj->UpdateBoundingSphere();
                    envobj->UpdatePhysicsTransform();
                }
            }
        }
    }

    void* EditorTransformChange::Construct(void* mem) {
        return new(mem) EditorTransformChange(0,vec3(),vec3(),quaternion());
    }

    void EditorTransformChange::Destroy(void* object) {
        EditorTransformChange* etc = static_cast<EditorTransformChange*>(object);
        etc->~EditorTransformChange();
    }
}
