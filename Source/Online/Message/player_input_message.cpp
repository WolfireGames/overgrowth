//-----------------------------------------------------------------------------
//           Name: player_input_message.cpp
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
#include "player_input_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    PlayerInputMessage::PlayerInputMessage(float depth, uint16_t depth_count, uint16_t count, uint8_t id) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        depth(depth), depth_count(depth_count), count(count), id(id)
    {

    }

    binn* PlayerInputMessage::Serialize(void* object) {
        PlayerInputMessage* t = static_cast<PlayerInputMessage*>(object);
        binn* l = binn_object();

        binn_object_set_float(l, "depth", t->depth);
        binn_object_set_uint16(l, "depth_count", t->depth_count);
        binn_object_set_uint16(l, "count", t->count);
        binn_object_set_uint8(l, "id", t->id);

        return l;
    }

    void PlayerInputMessage::Deserialize(void* object, binn* l) {
        PlayerInputMessage* t = static_cast<PlayerInputMessage*>(object);

        binn_object_get_float(l, "depth", &t->depth);
        binn_object_get_uint16(l, "depth_count", &t->depth_count);
        binn_object_get_uint16(l, "count", &t->count);
        binn_object_get_uint8(l, "id", &t->id);
    }

    void PlayerInputMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        PlayerInputMessage* t = static_cast<PlayerInputMessage*>(object);

        // Don't process "quit"
        if(t->id == Online::Instance()->GetBindID("quit")) {
            return;
        }

        // TODO We'll need to replace PeerID with a player ID in order to support multiple inputs
        Online::Instance()->online_session->player_inputs[from].push_back(ref);
    }

    void* PlayerInputMessage::Construct(void *mem) {
        return new(mem) PlayerInputMessage(0, 0, 0, 0);
    }

    void PlayerInputMessage::Destroy(void* object) {
        PlayerInputMessage* t = static_cast<PlayerInputMessage*>(object);
        t->~PlayerInputMessage();
    }
}
