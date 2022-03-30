//-----------------------------------------------------------------------------
//           Name: audio_play_sound_group_message.cpp
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
#include "audio_play_sound_group_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlaySoundGroupMessage::AudioPlaySoundGroupMessage(const std::string& path, vec3 pos) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), pos(pos) {
    }

    binn* AudioPlaySoundGroupMessage::Serialize(void* object) {
        AudioPlaySoundGroupMessage* t = static_cast<AudioPlaySoundGroupMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_vec3(l, "pos", t->pos);

        return l;
    }

    void AudioPlaySoundGroupMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundGroupMessage* t = static_cast<AudioPlaySoundGroupMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_vec3(l, "pos", &t->pos);
    }

    void AudioPlaySoundGroupMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundGroupMessage* t = static_cast<AudioPlaySoundGroupMessage*>(object);

        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        //SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(t->path);
        SoundGroupPlayInfo sgpi(*sgr, t->pos);
        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    }

    void* AudioPlaySoundGroupMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundGroupMessage("", vec3());
    }

    void AudioPlaySoundGroupMessage::Destroy(void* object) {
        AudioPlaySoundGroupMessage* t = static_cast<AudioPlaySoundGroupMessage*>(object);
        t->~AudioPlaySoundGroupMessage();
    }
}
