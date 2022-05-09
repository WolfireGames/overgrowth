//-----------------------------------------------------------------------------
//           Name: pcs_client_parameters_message.cpp
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
#include "pcs_client_parameters_message.h"

#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
PCSClientParametersMessage::PCSClientParametersMessage(const std::string& player_name, const std::string& enabled_mods_string) : OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
                                                                                                                                 player_name(player_name),
                                                                                                                                 enabled_mods_string(enabled_mods_string) {
}

binn* PCSClientParametersMessage::Serialize(void* object) {
    PCSClientParametersMessage* t = static_cast<PCSClientParametersMessage*>(object);
    binn* l = binn_object();

    binn_object_set_std_string(l, "player_name", t->player_name);
    binn_object_set_std_string(l, "enabled_mods_string", t->enabled_mods_string);

    return l;
}

void PCSClientParametersMessage::Deserialize(void* object, binn* l) {
    PCSClientParametersMessage* t = static_cast<PCSClientParametersMessage*>(object);

    binn_object_get_std_string(l, "player_name", &t->player_name);
    binn_object_get_std_string(l, "enabled_mods_string", &t->enabled_mods_string);
}

void PCSClientParametersMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    PCSClientParametersMessage* t = static_cast<PCSClientParametersMessage*>(object);

    assert(Online::Instance()->IsHosting());
    Online::Instance()->online_session->client_connection_manager.ApplyPackage(from, t);
}

void* PCSClientParametersMessage::Construct(void* mem) {
    return new (mem) PCSClientParametersMessage("", "");
}

void PCSClientParametersMessage::Destroy(void* object) {
    PCSClientParametersMessage* t = static_cast<PCSClientParametersMessage*>(object);
    t->~PCSClientParametersMessage();
}
}  // namespace OnlineMessages
