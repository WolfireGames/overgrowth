//-----------------------------------------------------------------------------
//           Name: audio_play_sound_message.cpp
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
#include "audio_play_sound_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
AudioPlaySoundMessage::AudioPlaySoundMessage(const std::string& path) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                        path(path) {
}

binn* AudioPlaySoundMessage::Serialize(void* object) {
    AudioPlaySoundMessage* t = static_cast<AudioPlaySoundMessage*>(object);
    binn* l = binn_object();

    binn_object_set_std_string(l, "path", t->path);

    return l;
}

void AudioPlaySoundMessage::Deserialize(void* object, binn* l) {
    AudioPlaySoundMessage* t = static_cast<AudioPlaySoundMessage*>(object);

    binn_object_get_std_string(l, "path", &t->path);
}

void AudioPlaySoundMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    AudioPlaySoundMessage* t = static_cast<AudioPlaySoundMessage*>(object);

    SoundPlayInfo spi;
    spi.path = t->path;
    spi.flags = spi.flags | SoundFlags::kRelative;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->Play(handle, spi);
}

void* AudioPlaySoundMessage::Construct(void* mem) {
    return new (mem) AudioPlaySoundMessage("");
}

void AudioPlaySoundMessage::Destroy(void* object) {
    AudioPlaySoundMessage* t = static_cast<AudioPlaySoundMessage*>(object);
    t->~AudioPlaySoundMessage();
}
}  // namespace OnlineMessages
