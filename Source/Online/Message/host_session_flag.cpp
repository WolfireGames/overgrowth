//-----------------------------------------------------------------------------
//           Name: host_session_flag.cpp
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
#include "host_session_flag.h"

#include <Online/online.h>

namespace OnlineMessages {
HostSessionFlag::HostSessionFlag(OnlineFlags flag, bool value) : OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
                                                                 flag(flag),
                                                                 value(value) {
}

binn* HostSessionFlag::Serialize(void* object) {
    HostSessionFlag* hsf = static_cast<HostSessionFlag*>(object);
    binn* l = binn_object();

    binn_object_set_uint32(l, "f", (uint32_t)hsf->flag);
    binn_object_set_bool(l, "v", hsf->value);

    return l;
}

void HostSessionFlag::Deserialize(void* object, binn* l) {
    HostSessionFlag* hsf = static_cast<HostSessionFlag*>(object);

    uint32_t flag;
    binn_object_get_uint32(l, "f", &flag);
    hsf->flag = (OnlineFlags)flag;

    BOOL value;
    binn_object_get_bool(l, "v", &value);
    hsf->value = (bool)value;
}

void HostSessionFlag::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    HostSessionFlag* hsf = static_cast<HostSessionFlag*>(object);
    Online* online = Online::Instance();

    if (online != nullptr && online->online_session != nullptr) {
        online->online_session->host_session_flags[hsf->flag] = hsf->value;
    }
}

void* HostSessionFlag::Construct(void* mem) {
    return new (mem) HostSessionFlag(OnlineFlags::INVALID, false);
}

void HostSessionFlag::Destroy(void* object) {
    HostSessionFlag* hsf = static_cast<HostSessionFlag*>(object);
    hsf->~HostSessionFlag();
}
}  // namespace OnlineMessages
