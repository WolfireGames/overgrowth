//-----------------------------------------------------------------------------
//           Name: set_player_state.h
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
#pragma once

#include "online_message_base.h"

struct PlayerState;

namespace OnlineMessages {
    // TODO It would be nice if we could only send the deltas instead of resending everything every time
    class SetPlayerState : public OnlineMessageBase {
    private:
        PlayerID player_id;

        CommonObjectID object_id;
        std::string playername;
        uint16_t ping;
        int32_t controller_id;
        int32_t camera_id;

    public:
        SetPlayerState(PlayerID player_id, const PlayerState* player_state);

        static binn* Serialize(void* object);
        static void Deserialize(void* object, binn* source);
        static void Execute(const OnlineMessageRef& ref, void* object, PeerID from);
        static void* Construct(void* mem);
        static void Destroy(void* object);
    };
}
