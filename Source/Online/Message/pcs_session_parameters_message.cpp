//-----------------------------------------------------------------------------
//           Name: pcs_session_parameters_message.cpp
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
#include "pcs_session_parameters_message.h"

#include <Online/online.h>
#include <Online/online_utility.h>

#include <Utility/binn_util.h>

namespace OnlineMessages {
    PCSSessionParametersMessage::PCSSessionParametersMessage(std::map<std::string, uint8_t> bind_id_map) :
        OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
        bind_id_map(bind_id_map)
    {

    }

    binn* PCSSessionParametersMessage::Serialize(void* object) {
        PCSSessionParametersMessage* t = static_cast<PCSSessionParametersMessage*>(object);
        binn* l = binn_object();

        binn* list;
        list = binn_list();

        for (auto& binding : t->bind_id_map) {
            binn_list_add_std_string(list, binding.first);
            binn_list_add_uint8(list, binding.second);
        }
        binn_object_set_list(l, "bindings", list);
        return l;
    }

    void PCSSessionParametersMessage::Deserialize(void* object, binn* l) {
        PCSSessionParametersMessage* t = static_cast<PCSSessionParametersMessage*>(object);

        binn* list;
        binn_object_get_list(l, "bindings", (void**) &list);

        t->bind_id_map.clear();

        int count = binn_count(list);
        std::string key;
        uint8_t value;
        for (int i = 1; i <= count; i+=2) {
            binn_list_get_std_string(list, i, &key);
            binn_list_get_uint8(list, i + 1, &value);
            t->bind_id_map.emplace(key, value);
        }
    }

    void PCSSessionParametersMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        PCSSessionParametersMessage* t = static_cast<PCSSessionParametersMessage*>(object);

        // This should never run on the host, kill connections that try to make us
        if (Online::Instance()->IsHosting()) {
            Online::Instance()->CloseConnection(Online::Instance()->GetPeerFromID(from)->conn_id, ConnectionClosedReason::BAD_REQUEST);
            LOGW << "A client just told us to discard the current session. This is something the client should never do. Closing connection" << std::endl;
            return;
        }

        // We should forget about certain things here!
        Online::Instance()->online_session->player_states.clear();
        Online::Instance()->SessionStarted(false);

        Online::Instance()->online_session->binding_id_map_ = t->bind_id_map;

        // Send own parameters to host
        Online::Instance()->Send<OnlineMessages::PCSClientParametersMessage>(OnlineUtility::GetPlayerName(), OnlineUtility::GetActiveModsString());
    }

    void* PCSSessionParametersMessage::Construct(void *mem) {
        std::map<std::string, uint8_t> bind_id_map;
        return new(mem) PCSSessionParametersMessage(bind_id_map);
    }

    void PCSSessionParametersMessage::Destroy(void* object) {
        PCSSessionParametersMessage* t = static_cast<PCSSessionParametersMessage*>(object);
        t->~PCSSessionParametersMessage();
    }
}
