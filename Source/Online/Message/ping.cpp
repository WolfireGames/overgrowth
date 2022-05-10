//-----------------------------------------------------------------------------
//           Name: ping.cpp
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
#include "ping.h"

#include <Online/online.h>

namespace OnlineMessages {
Ping::Ping(uint32_t ping_id) : OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
                               ping_id(ping_id) {
}

binn* Ping::Serialize(void* object) {
    Ping* p = static_cast<Ping*>(object);
    binn* l = binn_object();

    binn_object_set_uint32(l, "id", p->ping_id);

    return l;
}

void Ping::Deserialize(void* object, binn* l) {
    Ping* p = static_cast<Ping*>(object);

    binn_object_get_uint32(l, "id", &p->ping_id);
}

void Ping::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    Ping* p = static_cast<Ping*>(object);
    Peer* peer = Online::Instance()->GetPeerFromID(from);
    if (peer != nullptr) {
        Online::Instance()->SendTo<OnlineMessages::Pong>(peer->conn_id, p->ping_id);
    }
}

void* Ping::Construct(void* mem) {
    return new (mem) Ping(std::numeric_limits<uint32_t>::max());
}

void Ping::Destroy(void* object) {
    Ping* p = static_cast<Ping*>(object);
    p->~Ping();
}
}  // namespace OnlineMessages
