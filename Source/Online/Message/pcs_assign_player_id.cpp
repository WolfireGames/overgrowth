//-----------------------------------------------------------------------------
//           Name: pcs_assign_player_id.cpp
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
#include "pcs_assign_player_id.h"

#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
PCSAssignPlayerID::PCSAssignPlayerID(PlayerID player_id) : OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
                                                           player_id(player_id) {
}

binn* PCSAssignPlayerID::Serialize(void* object) {
    PCSAssignPlayerID* t = static_cast<PCSAssignPlayerID*>(object);
    binn* l = binn_object();

    binn_object_set_uint8(l, "player_id", t->player_id);

    return l;
}

void PCSAssignPlayerID::Deserialize(void* object, binn* l) {
    PCSAssignPlayerID* t = static_cast<PCSAssignPlayerID*>(object);

    binn_object_get_uint8(l, "player_id", &t->player_id);
}

void PCSAssignPlayerID::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    PCSAssignPlayerID* t = static_cast<PCSAssignPlayerID*>(object);
    Online::Instance()->online_session->local_player_id = t->player_id;
}

void* PCSAssignPlayerID::Construct(void* mem) {
    return new (mem) PCSAssignPlayerID(0);
}

void PCSAssignPlayerID::Destroy(void* object) {
    PCSAssignPlayerID* t = static_cast<PCSAssignPlayerID*>(object);
    t->~PCSAssignPlayerID();
}
}  // namespace OnlineMessages
