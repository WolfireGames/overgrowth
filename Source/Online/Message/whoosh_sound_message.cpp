//-----------------------------------------------------------------------------
//           Name: whoosh_sound_message.cpp
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
#include "whoosh_sound_message.h"

#include <Main/engine.h>

namespace OnlineMessages {
    WhooshSoundMessage::WhooshSoundMessage(float whoosh_amount, float whoosh_pitch) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT)
    {
        this->whoosh_amount = whoosh_amount;
        this->whoosh_pitch = whoosh_pitch;
    }

    void WhooshSoundMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        WhooshSoundMessage* wsm = static_cast<WhooshSoundMessage*>(object);

        Engine::Instance()->GetSound()->setAirWhoosh(wsm->whoosh_amount, wsm->whoosh_pitch);
    }


    binn* WhooshSoundMessage::Serialize(void *object) {
        WhooshSoundMessage* wsm = static_cast<WhooshSoundMessage*>(object);

        binn* l = binn_object();

        binn_object_set_float(l, "amount", wsm->whoosh_amount);
        binn_object_set_float(l, "pitch", wsm->whoosh_pitch);

        return l;
    }

    void WhooshSoundMessage::Deserialize(void* object, binn* l) {
        WhooshSoundMessage* wsm = static_cast<WhooshSoundMessage*>(object);

        binn_object_get_float(l, "amount", &wsm->whoosh_amount);
        binn_object_get_float(l, "pitch", &wsm->whoosh_pitch);
    }

    void* WhooshSoundMessage::Construct(void* mem) {
        return new (mem) WhooshSoundMessage(0,0);
    }

    void WhooshSoundMessage::Destroy(void *object) {
        WhooshSoundMessage* wsm = static_cast<WhooshSoundMessage*>(object);
        wsm->~WhooshSoundMessage();
    }
}
