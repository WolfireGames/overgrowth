//-----------------------------------------------------------------------------
//           Name: audio_play_group_priority_message.cpp
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
#include "audio_play_group_priority_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AudioPlayGroupPriorityMessage::AudioPlayGroupPriorityMessage(const std::string & path, vec3 pos, int priority) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), pos(pos), priority(priority)
    {

    }

    binn* AudioPlayGroupPriorityMessage::Serialize(void* object) {
        AudioPlayGroupPriorityMessage* t = static_cast<AudioPlayGroupPriorityMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_vec3(l, "pos", t->pos);
        binn_object_set_int32(l, "priority", t->priority);

        return l;
    }

    void AudioPlayGroupPriorityMessage::Deserialize(void* object, binn* l) {
        AudioPlayGroupPriorityMessage* t = static_cast<AudioPlayGroupPriorityMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_vec3(l, "pos", &t->pos);
        binn_object_get_int32(l, "priority", &t->priority);
    }

    void AudioPlayGroupPriorityMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlayGroupPriorityMessage* t = static_cast<AudioPlayGroupPriorityMessage*>(object);

        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(t->path);
        SoundGroupPlayInfo sgpi(*sgr, t->pos);
        sgpi.priority = t->priority;
        int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    }

    void* AudioPlayGroupPriorityMessage::Construct(void *mem) {
        return new(mem) AudioPlayGroupPriorityMessage("", vec3(), 0);
    }

    void AudioPlayGroupPriorityMessage::Destroy(void* object) {
        AudioPlayGroupPriorityMessage* t = static_cast<AudioPlayGroupPriorityMessage*>(object);
        t->~AudioPlayGroupPriorityMessage();
    }
}
