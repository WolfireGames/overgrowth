//-----------------------------------------------------------------------------
//           Name: audio_play_sound_loop_message.h
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

namespace OnlineMessages {
    class AudioPlaySoundLoopMessage : public OnlineMessageBase {
    private:
        std::string path;
        float gain;

    public:
        AudioPlaySoundLoopMessage(std::string path, float gain);

        static binn* Serialize(void* object);
        static void Deserialize(void* object, binn* source);
        static void Execute(const OnlineMessageRef& ref, void* object, PeerID from);
        static void* Construct(void* mem);
        static void Destroy(void* object);
    };
}
