//-----------------------------------------------------------------------------
//           Name: set_player_state.cpp
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
#include "set_player_state.h"

#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
SetPlayerState::SetPlayerState(PlayerID player_id, const PlayerState* player_state) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                      player_id(player_id) {
    object_id = Online::Instance()->GetOriginalID(player_state->object_id);
    playername = player_state->playername;
    ping = player_state->ping;
    camera_id = player_state->camera_id;
    controller_id = player_state->controller_id;
}

binn* SetPlayerState::Serialize(void* object) {
    SetPlayerState* t = static_cast<SetPlayerState*>(object);
    binn* l = binn_object();

    binn_object_set_int32(l, "object_id", t->object_id);
    binn_object_set_std_string(l, "playername", t->playername);
    binn_object_set_uint16(l, "ping", t->ping);
    binn_object_set_uint8(l, "player_id", t->player_id);
    binn_object_set_int32(l, "camera_id", t->camera_id);
    binn_object_set_int32(l, "controller_id", t->controller_id);

    return l;
}

void SetPlayerState::Deserialize(void* object, binn* l) {
    SetPlayerState* t = static_cast<SetPlayerState*>(object);

    binn_object_get_int32(l, "object_id", &t->object_id);
    binn_object_get_std_string(l, "playername", &t->playername);
    binn_object_get_uint16(l, "ping", &t->ping);
    binn_object_get_uint8(l, "player_id", &t->player_id);
    binn_object_get_int32(l, "camera_id", &t->camera_id);
    binn_object_get_int32(l, "controller_id", &t->controller_id);
}

void SetPlayerState::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    SetPlayerState* t = static_cast<SetPlayerState*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(t->object_id);

    // Clients have not (and should not) registered a camera id locally applying the host's camera_id will lead to a crash.
    if (Online::Instance()->IsClient()) {
        t->camera_id = 0;
    }

    PlayerState new_player_state;
    new_player_state.object_id = object_id;
    new_player_state.playername = t->playername;
    new_player_state.ping = t->ping;
    new_player_state.camera_id = t->camera_id;
    new_player_state.controller_id = t->controller_id;

    // Set new state
    Online::Instance()->online_session->player_states[t->player_id] = new_player_state;
}

void* SetPlayerState::Construct(void* mem) {
    PlayerState player_state;
    return new (mem) SetPlayerState(0, &player_state);
}

void SetPlayerState::Destroy(void* object) {
    SetPlayerState* t = static_cast<SetPlayerState*>(object);
    t->~SetPlayerState();
}
}  // namespace OnlineMessages
