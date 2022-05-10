//-----------------------------------------------------------------------------
//           Name: pong.cpp
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
#include "pong.h"

#include <Online/online.h>
#include <Internal/timer.h>

extern Timer game_timer;

namespace OnlineMessages {
Pong::Pong(uint32_t ping_id) : OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
                               ping_id(ping_id) {
}

binn* Pong::Serialize(void* object) {
    Pong* p = static_cast<Pong*>(object);
    binn* l = binn_object();

    binn_object_set_uint32(l, "id", p->ping_id);

    return l;
}

void Pong::Deserialize(void* object, binn* l) {
    Pong* p = static_cast<Pong*>(object);

    binn_object_get_uint32(l, "id", &p->ping_id);
}

void Pong::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    Pong* p = static_cast<Pong*>(object);
    Online* online = Online::Instance();

    Peer* peer = online->GetPeerFromID(from);
    if (peer != nullptr && online->online_session->last_ping_id == p->ping_id) {
        peer->current_ping_delta = game_timer.wall_time - online->online_session->last_ping_time;
    }
}

void* Pong::Construct(void* mem) {
    return new (mem) Pong(std::numeric_limits<uint32_t>::max());
}

void Pong::Destroy(void* object) {
    Pong* p = static_cast<Pong*>(object);
    p->~Pong();
}
}  // namespace OnlineMessages
