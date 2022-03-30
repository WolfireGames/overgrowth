//-----------------------------------------------------------------------------
//           Name: audio_play_sound_loop_message.cpp
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
#include "audio_play_sound_loop_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundLoopMessage::AudioPlaySoundLoopMessage(std::string path, float gain) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), gain(gain)
    {

    }

    binn* AudioPlaySoundLoopMessage::Serialize(void* object) {
        AudioPlaySoundLoopMessage* t = static_cast<AudioPlaySoundLoopMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_float(l, "gain", t->gain);

        return l;
    }

    void AudioPlaySoundLoopMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundLoopMessage* t = static_cast<AudioPlaySoundLoopMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_float(l, "gain", &t->gain);
    }

    void AudioPlaySoundLoopMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundLoopMessage* t = static_cast<AudioPlaySoundLoopMessage*>(object);

        SoundPlayInfo spi;
        spi.path = t->path;
        spi.flags = spi.flags | SoundFlags::kRelative;
        spi.looping = true;
        spi.volume = t->gain;
        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->Play(handle, spi);
    }

    void* AudioPlaySoundLoopMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundLoopMessage("", 0.0f);
    }

    void AudioPlaySoundLoopMessage::Destroy(void* object) {
        AudioPlaySoundLoopMessage* t = static_cast<AudioPlaySoundLoopMessage*>(object);
        t->~AudioPlaySoundLoopMessage();
    }
}
