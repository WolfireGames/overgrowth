//-----------------------------------------------------------------------------
//           Name: chat_entry_message.cpp
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
#include "chat_entry_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>
#include <Online/online.h>
#include <Online/online_utility.h>

namespace OnlineMessages {
    ChatEntryMessage::ChatEntryMessage(const std::string & entry) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        entry(entry)
    {

    }

    binn* ChatEntryMessage::Serialize(void* object) {
        ChatEntryMessage* t = static_cast<ChatEntryMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "entry", t->entry);

        return l;
    }

    void ChatEntryMessage::Deserialize(void* object, binn* l) {
        ChatEntryMessage* t = static_cast<ChatEntryMessage*>(object);

        binn_object_get_std_string(l, "entry", &t->entry);
    }

    void ChatEntryMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        ChatEntryMessage* t = static_cast<ChatEntryMessage*>(object);

        // Hosts will prefix name of sender and send it to everyone, clients will simply apply it here.
        if (Online::Instance()->IsActive()) {
            if (Online::Instance()->IsHosting()) {
                if (OnlineUtility::IsCommand(t->entry)) {
                    OnlineUtility::HandleCommand(from, t->entry); // TODO We assume that peer_id == player_id here
                    Online::Instance()->Send<ChatEntryMessage>(Online::Instance()->online_session->player_states[from].playername + ": " + t->entry);
                } else {
                    Online::Instance()->SendRawChatMessage(Online::Instance()->online_session->player_states[from].playername + ": " + t->entry);
                }
            } else {
                Online::Instance()->AddLocalChatMessage(t->entry);
            }
        }
    }

    void* ChatEntryMessage::Construct(void *mem) {
        return new(mem) ChatEntryMessage("");
    }

    void ChatEntryMessage::Destroy(void* object) {
        ChatEntryMessage* t = static_cast<ChatEntryMessage*>(object);
        t->~ChatEntryMessage();
    }
}
