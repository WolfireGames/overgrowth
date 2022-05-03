//-----------------------------------------------------------------------------
//           Name: env_object_update.cpp
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
#include "env_object_update.h"

#include <Main/engine.h>
#include <Online/online.h>

extern Timer game_timer;

namespace OnlineMessages {
    EnvObjectUpdate::EnvObjectUpdate() :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        id(-1), transform(mat4()), timestamp(0.0f)
    {

    }

    EnvObjectUpdate::EnvObjectUpdate(EnvObject* object) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        transform(object->GetTransform()), timestamp(game_timer.GetWallTime())
    {
        this->id = Online::Instance()->GetOriginalID(object->GetID());
    }

    binn* EnvObjectUpdate::Serialize(void* object) {
        EnvObjectUpdate* eou = static_cast<EnvObjectUpdate*>(object);
        binn* l = binn_object();

        binn_object_set_int32(l, "id", eou->id);
        binn_object_set_float(l, "ts", eou->timestamp);
        binn_object_set_mat4(l, "t", eou->transform);

        return l;
    }

    void EnvObjectUpdate::Deserialize(void* object, binn* l) {
        EnvObjectUpdate* eou = static_cast<EnvObjectUpdate*>(object);

        binn_object_get_int32(l, "id", &eou->id);
        binn_object_get_float(l, "ts", &eou->timestamp);
        binn_object_get_mat4(l, "t", &eou->transform);
    }

    void EnvObjectUpdate::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
        EnvObjectUpdate* eou = static_cast<EnvObjectUpdate*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(eou->id);

        SceneGraph* sg = Engine::Instance()->GetSceneGraph();

        if(sg != nullptr && Online::Instance()->host_started_level) {
            Object* object = sg->GetObjectFromID(object_id);
            if (object != nullptr && object->GetType() == EntityType::_env_object) {
                EnvObject* eo = static_cast<EnvObject*>(object);
                eo->incoming_online_env_update.push_back(ref);
            } else {
                LOGW << "Received EnvObjectUpdate, but couldn't find an EnvObject with ID of \"" << object_id << " (" << eou->id << ")\"" << std::endl;
            }
        } else {
            LOGW << "Client received EnvObjectUpdate, but wasn't ready to receive it. The host should not be sending this to us right now!" << std::endl;
        }
    }

    void* EnvObjectUpdate::Construct(void* mem) {
        return new(mem) EnvObjectUpdate();
    }

    void EnvObjectUpdate::Destroy(void* object) {
        EnvObjectUpdate* eou = static_cast<EnvObjectUpdate*>(object);
        eou->~EnvObjectUpdate();
    }
}
