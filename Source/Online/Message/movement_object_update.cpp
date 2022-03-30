//-----------------------------------------------------------------------------
//           Name: movement_object_update.cpp
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
#include "movement_object_update.h"

#include <Utility/binn_util.h>
#include <Main/engine.h>

namespace OnlineMessages {
    MovementObjectUpdate::MovementObjectUpdate() :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT)
    {
        reliable_delivery = false;
    }

    MovementObjectUpdate::MovementObjectUpdate(MovementObject* mo) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT)
    {
        reliable_delivery = false;
        RiggedObject* rigged_object = mo->rigged_object();

        position = mo->position;
        velocity = mo->velocity;
        identifier = Online::Instance()->GetOriginalID(mo->GetID());
        timestamp = mo->walltime_last_update;
        rigged_body_frame.host_walltime = rigged_object->network_bones_host_walltime;
        facing = mo->GetFacing();
        rigged_body_frame.bone_count = rigged_object->network_bones.size();

        for(int i = 0; i < rigged_object->network_bones.size() && i < rigged_body_frame.bones.size(); i++) {
            rigged_body_frame.bones[i] = rigged_object->network_bones[i];
        }
    }

    binn* MovementObjectUpdate::Serialize(void* object) {
        MovementObjectUpdate* mou = static_cast<MovementObjectUpdate*>(object);

        binn* l = binn_object();

        binn_object_set_int32(l, "id", mou->identifier);
        binn_object_set_float(l, "ts", mou->timestamp);
        binn_object_set_vec3(l, "p", mou->position);
        binn_object_set_vec3(l, "v", mou->velocity);
        binn_object_set_vec3(l, "f", mou->facing);
        binn* bo_rbf = mou->rigged_body_frame.Serialize();
        binn_object_set_object(l, "rbf", bo_rbf);
        binn_free(bo_rbf);

        return l;
    }

    void MovementObjectUpdate::Deserialize(void* object, binn* l) {
        MovementObjectUpdate* mou = static_cast<MovementObjectUpdate*>(object);

        binn_object_get_int32(l, "id", &mou->identifier);
        binn_object_get_float(l, "ts", &mou->timestamp);
        binn_object_get_vec3(l, "p", &mou->position);
        binn_object_get_vec3(l, "v", &mou->velocity);
        binn_object_get_vec3(l, "f", &mou->facing);

        void* bo_rbf;
        binn_object_get_object(l, "rbf", &bo_rbf);
        mou->rigged_body_frame.Deserialize((binn*)bo_rbf);
    }

    void MovementObjectUpdate::Execute(const OnlineMessageRef& object_ref, void* object, PeerID peer) {
        MovementObjectUpdate* mou = static_cast<MovementObjectUpdate*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(mou->identifier);
        SceneGraph* sg = Engine::Instance()->GetSceneGraph();

        if(sg != nullptr && Online::Instance()->host_started_level) {
            Object* object = sg->GetObjectFromID(object_id);
            if (object != nullptr && object->GetType() == EntityType::_movement_object) {
                MovementObject* mo = static_cast<MovementObject*>(object);

                // Check if we already have an update for the specified time
                for (auto it = mo->incoming_movement_object_frames.begin(); it != mo->incoming_movement_object_frames.end(); it++) {
                    MovementObjectUpdate* update = static_cast<MovementObjectUpdate*>(it->GetData());
                    if (update->timestamp == mou->timestamp) {
                        LOGW << "Received MovementObjectUpdate with an identical timestamp of an existing update for object: \"" << object_id << " (" << mou->identifier << ")\"" << std::endl;
                        return;
                    }
                }

                mo->incoming_movement_object_frames.push_back(object_ref);
            } else {
                LOGW << "Received MovementObjectUpdate, but couldn't find an MovementObject with ID of \"" << object_id << " (" << mou->identifier << ")\"" << std::endl;
            }
        } else {
            LOGW << "Client received MorphTargetUpdate, but wasn't ready to receive it. The host should not be sending this to us right now!" << std::endl;
        }
    }

    void* MovementObjectUpdate::Construct(void *mem) {
        return new (mem) MovementObjectUpdate();
    }

    void MovementObjectUpdate::Destroy(void* object) {
        MovementObjectUpdate *mou = static_cast<MovementObjectUpdate*>(object);
        mou->~MovementObjectUpdate();
    }
}
