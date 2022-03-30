//-----------------------------------------------------------------------------
//           Name: camera_transform_message.cpp
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
#include "camera_transform_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    CameraTransformMessage::CameraTransformMessage(vec3 position, vec3 flat_facing, vec3 facing, vec3 up, float x_rotation, float y_rotation) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        position(position), flat_facing(flat_facing), facing(facing), up(up), x_rotation(x_rotation), y_rotation(y_rotation)
    {
        reliable_delivery = false;
    }

    binn* CameraTransformMessage::Serialize(void* object) {
        CameraTransformMessage* t = static_cast<CameraTransformMessage*>(object);
        binn* l = binn_object();

        binn_object_set_vec3(l, "position", t->position);
        binn_object_set_vec3(l, "flat_facing", t->flat_facing);
        binn_object_set_vec3(l, "facing", t->facing);
        binn_object_set_vec3(l, "up", t->up);
        binn_object_set_float(l, "x_rotation", t->x_rotation);
        binn_object_set_float(l, "y_rotation", t->y_rotation);

        return l;
    }

    void CameraTransformMessage::Deserialize(void* object, binn* l) {
        CameraTransformMessage* t = static_cast<CameraTransformMessage*>(object);

        binn_object_get_vec3(l, "position", &t->position);
        binn_object_get_vec3(l, "flat_facing", &t->flat_facing);
        binn_object_get_vec3(l, "facing", &t->facing);
        binn_object_get_vec3(l, "up", &t->up);
        binn_object_get_float(l, "x_rotation", &t->x_rotation);
        binn_object_get_float(l, "y_rotation", &t->y_rotation);
    }

    void CameraTransformMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        CameraTransformMessage* t = static_cast<CameraTransformMessage*>(object);

        if (Online::Instance()->online_session->player_states.find(from) != Online::Instance()->online_session->player_states.end()) {
            int camera_id = Online::Instance()->online_session->player_states[from].camera_id; // TODO this assumes peer_id == player_id
            Camera* camera = ActiveCameras::Instance()->GetCamera(camera_id);
            if (camera != nullptr) {
                camera->SetPos(t->position);
                camera->SetFlatFacing(t->flat_facing);
                camera->SetFacing(t->facing);
                camera->SetUp(t->up);
                camera->SetXRotation(t->x_rotation);
                camera->SetYRotation(t->y_rotation);
            } else {
                LOGW << "Got camera transform information for camera " << camera_id << " from client " << std::to_string(from) << " without a valid camera!" << std::endl;
            }
        } else {
            LOGW << "Got camera transform information from a client we can no longer find, they might have disconnected!" << std::endl; // TODO should we even warn about this?
        }
    }

    void* CameraTransformMessage::Construct(void *mem) {
        return new(mem) CameraTransformMessage(vec3(), vec3(), vec3(), vec3(), 0, 0);
    }

    void CameraTransformMessage::Destroy(void* object) {
        CameraTransformMessage* t = static_cast<CameraTransformMessage*>(object);
        t->~CameraTransformMessage();
    }
}
