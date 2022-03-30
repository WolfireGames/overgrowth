//-----------------------------------------------------------------------------
//           Name: audio_play_group_gain_message.cpp
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
#include "audio_play_sound_group_gain_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundGroupGainMessage::AudioPlaySoundGroupGainMessage(const std::string& path, vec3 pos, float gain) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), pos(pos), gain(gain)
    {

    }

    binn* AudioPlaySoundGroupGainMessage::Serialize(void* object) {
        AudioPlaySoundGroupGainMessage* t = static_cast<AudioPlaySoundGroupGainMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_vec3(l, "pos", t->pos);
        binn_object_set_float(l, "gain", t->gain);

        return l;
    }

    void AudioPlaySoundGroupGainMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundGroupGainMessage* t = static_cast<AudioPlaySoundGroupGainMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_vec3(l, "pos", &t->pos);
        binn_object_get_float(l, "gain", &t->gain);
    }

    void AudioPlaySoundGroupGainMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundGroupGainMessage* t = static_cast<AudioPlaySoundGroupGainMessage*>(object);

        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(t->path);
        SoundGroupPlayInfo sgpi(*sgr, t->pos);
        sgpi.gain = t->gain;
        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    }

    void* AudioPlaySoundGroupGainMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundGroupGainMessage("", vec3(), 0.0f);
    }

    void AudioPlaySoundGroupGainMessage::Destroy(void* object) {
        AudioPlaySoundGroupGainMessage* t = static_cast<AudioPlaySoundGroupGainMessage*>(object);
        t->~AudioPlaySoundGroupGainMessage();
    }
}
