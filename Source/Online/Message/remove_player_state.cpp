//-----------------------------------------------------------------------------
//           Name: remove_player_state.cpp
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
#include "remove_player_state.h"

#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
RemovePlayerState::RemovePlayerState(PlayerID player_id) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                           player_id(player_id) {
}

binn* RemovePlayerState::Serialize(void* object) {
    RemovePlayerState* t = static_cast<RemovePlayerState*>(object);
    binn* l = binn_object();

    binn_object_set_uint8(l, "player_id", t->player_id);

    return l;
}

void RemovePlayerState::Deserialize(void* object, binn* l) {
    RemovePlayerState* t = static_cast<RemovePlayerState*>(object);

    binn_object_get_uint8(l, "player_id", &t->player_id);
}

void RemovePlayerState::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    RemovePlayerState* t = static_cast<RemovePlayerState*>(object);

    if (Online::Instance()->online_session->player_states.find(t->player_id) != Online::Instance()->online_session->player_states.end()) {
        Online::Instance()->online_session->player_states.erase(t->player_id);
    }
}

void* RemovePlayerState::Construct(void* mem) {
    return new (mem) RemovePlayerState(0);
}

void RemovePlayerState::Destroy(void* object) {
    RemovePlayerState* t = static_cast<RemovePlayerState*>(object);
    t->~RemovePlayerState();
}
}  // namespace OnlineMessages
