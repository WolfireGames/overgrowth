//-----------------------------------------------------------------------------
//           Name: audio_play_sound_group_relative_message.cpp
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
#include "audio_play_sound_group_relative_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundGroupRelativeMessage::AudioPlaySoundGroupRelativeMessage(const std::string & path) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path)
    {

    }

    binn* AudioPlaySoundGroupRelativeMessage::Serialize(void* object) {
        AudioPlaySoundGroupRelativeMessage* t = static_cast<AudioPlaySoundGroupRelativeMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);

        return l;
    }

    void AudioPlaySoundGroupRelativeMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundGroupRelativeMessage* t = static_cast<AudioPlaySoundGroupRelativeMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
    }

    void AudioPlaySoundGroupRelativeMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundGroupRelativeMessage* t = static_cast<AudioPlaySoundGroupRelativeMessage*>(object);

        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(t->path);
        SoundGroupPlayInfo sgpi(SoundGroupPlayInfo(*sgr, vec3(0.0f)));
        sgpi.flags = sgpi.flags | SoundFlags::kRelative;

        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);

        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    }

    void* AudioPlaySoundGroupRelativeMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundGroupRelativeMessage("");
    }

    void AudioPlaySoundGroupRelativeMessage::Destroy(void* object) {
        AudioPlaySoundGroupRelativeMessage* t = static_cast<AudioPlaySoundGroupRelativeMessage*>(object);
        t->~AudioPlaySoundGroupRelativeMessage();
    }
}
