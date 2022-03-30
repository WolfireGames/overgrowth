//-----------------------------------------------------------------------------
//           Name: audio_play_sound_location_message.cpp
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
#include "audio_play_sound_location_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundLocationMessage::AudioPlaySoundLocationMessage(const std::string & path, vec3 pos) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), pos(pos)
    {

    }

    binn* AudioPlaySoundLocationMessage::Serialize(void* object) {
        AudioPlaySoundLocationMessage* t = static_cast<AudioPlaySoundLocationMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_vec3(l, "pos", t->pos);

        return l;
    }

    void AudioPlaySoundLocationMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundLocationMessage* t = static_cast<AudioPlaySoundLocationMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_vec3(l, "pos", &t->pos);
    }

    void AudioPlaySoundLocationMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundLocationMessage* t = static_cast<AudioPlaySoundLocationMessage*>(object);

        SoundPlayInfo spi;
        spi.path = t->path;
        spi.position = t->pos;
        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->Play(handle, spi);
    }

    void* AudioPlaySoundLocationMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundLocationMessage("", vec3());
    }

    void AudioPlaySoundLocationMessage::Destroy(void* object) {
        AudioPlaySoundLocationMessage* t = static_cast<AudioPlaySoundLocationMessage*>(object);
        t->~AudioPlaySoundLocationMessage();
    }
}
