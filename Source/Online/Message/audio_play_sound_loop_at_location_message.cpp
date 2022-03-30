//-----------------------------------------------------------------------------
//           Name: audio_play_sound_loop_at_location_message.cpp
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
#include "audio_play_sound_loop_at_location_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundLoopAtLocationMessage::AudioPlaySoundLoopAtLocationMessage(const std::string& path, float gain, vec3 pos) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), gain(gain), pos(pos)
    {

    }

    binn* AudioPlaySoundLoopAtLocationMessage::Serialize(void* object) {
        AudioPlaySoundLoopAtLocationMessage* t = static_cast<AudioPlaySoundLoopAtLocationMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_vec3(l, "pos", t->pos);
        binn_object_set_float(l, "gain", t->gain);

        return l;
    }

    void AudioPlaySoundLoopAtLocationMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundLoopAtLocationMessage* t = static_cast<AudioPlaySoundLoopAtLocationMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_vec3(l, "pos", &t->pos);
        binn_object_get_float(l, "gain", &t->gain);
    }

    void AudioPlaySoundLoopAtLocationMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundLoopAtLocationMessage* t = static_cast<AudioPlaySoundLoopAtLocationMessage*>(object);

        SoundPlayInfo spi;
        spi.path = t->path;
        spi.looping = true;
        spi.volume = t->gain;
        spi.position = t->pos;
        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->Play(handle, spi);
    }

    void* AudioPlaySoundLoopAtLocationMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundLoopAtLocationMessage("", 0.0f, vec3());
    }

    void AudioPlaySoundLoopAtLocationMessage::Destroy(void* object) {
        AudioPlaySoundLoopAtLocationMessage* t = static_cast<AudioPlaySoundLoopAtLocationMessage*>(object);
        t->~AudioPlaySoundLoopAtLocationMessage();
    }
}
