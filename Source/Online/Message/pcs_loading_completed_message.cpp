//-----------------------------------------------------------------------------
//           Name: pcs_loading_completed_message.cpp
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
#include "pcs_loading_completed_message.h"

#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    PCSLoadingCompletedMessage::PCSLoadingCompletedMessage() :
        OnlineMessageBase(OnlineMessageCategory::TRANSIENT)
    {
        level_name = Online::Instance()->level_name;
    }

    binn* PCSLoadingCompletedMessage::Serialize(void* object) {
        PCSLoadingCompletedMessage* t = static_cast<PCSLoadingCompletedMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "level_name", t->level_name);

        return l;
    }

    void PCSLoadingCompletedMessage::Deserialize(void* object, binn* l) {
        PCSLoadingCompletedMessage* t = static_cast<PCSLoadingCompletedMessage*>(object);

        binn_object_get_std_string(l, "level_name", &t->level_name);
    }

    void PCSLoadingCompletedMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        PCSLoadingCompletedMessage* t = static_cast<PCSLoadingCompletedMessage*>(object);

        if (Online::Instance()->IsHosting()) {
            if (Online::Instance()->level_name == t->level_name) {
                // A client just told us (host) they are done loading, update actor in state machine
                Online::Instance()->online_session->client_connection_manager.ApplyPackage(from, t);
            } else {
                LOGW << "Client completed loading \"" << t->level_name << "\", but we are in \"" << Online::Instance()->level_name << "\"" << std::endl;
            }
        } else {
            // Host started game session. We are about to get carpet bombed with packages!
            Online::Instance()->SessionStarted(true);
        }
    }

    void* PCSLoadingCompletedMessage::Construct(void *mem) {
        return new(mem) PCSLoadingCompletedMessage();
    }

    void PCSLoadingCompletedMessage::Destroy(void* object) {
        PCSLoadingCompletedMessage* t = static_cast<PCSLoadingCompletedMessage*>(object);
        t->~PCSLoadingCompletedMessage();
    }
}
