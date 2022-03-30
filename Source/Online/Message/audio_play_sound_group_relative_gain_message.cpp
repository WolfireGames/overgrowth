//-----------------------------------------------------------------------------
//           Name: audio_play_sound_group_relative_gain_message.cpp
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
#include "audio_play_sound_group_relative_gain_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundGroupRelativeGainMessage::AudioPlaySoundGroupRelativeGainMessage(const std::string& path, float gain) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), gain(gain)
    {

    }

    binn* AudioPlaySoundGroupRelativeGainMessage::Serialize(void* object) {
        AudioPlaySoundGroupRelativeGainMessage* t = static_cast<AudioPlaySoundGroupRelativeGainMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_float(l, "gain", t->gain);

        return l;
    }

    void AudioPlaySoundGroupRelativeGainMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundGroupRelativeGainMessage* t = static_cast<AudioPlaySoundGroupRelativeGainMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_float(l, "gain", &t->gain);
    }

    void AudioPlaySoundGroupRelativeGainMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundGroupRelativeGainMessage* t = static_cast<AudioPlaySoundGroupRelativeGainMessage*>(object);

        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(t->path);
        SoundGroupPlayInfo sgpi(SoundGroupPlayInfo(*sgr, vec3(0.0f)));
        sgpi.flags = sgpi.flags | SoundFlags::kRelative;
        sgpi.gain = t->gain;

        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    }

    void* AudioPlaySoundGroupRelativeGainMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundGroupRelativeGainMessage("", 0.0f);
    }

    void AudioPlaySoundGroupRelativeGainMessage::Destroy(void* object) {
        AudioPlaySoundGroupRelativeGainMessage* t = static_cast<AudioPlaySoundGroupRelativeGainMessage*>(object);
        t->~AudioPlaySoundGroupRelativeGainMessage();
    }
}
