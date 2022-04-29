//-----------------------------------------------------------------------------
//           Name: audio_play_sound_group_voice_message.cpp
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
#include "audio_play_sound_group_voice_message.h"

#include <Main/engine.h>
#include <Utility/binn_util.h>
#include <Online/online.h>

namespace OnlineMessages {
    AudioPlaySoundGroupVoiceMessage::AudioPlaySoundGroupVoiceMessage(const std::string& path, ObjectID id, float delay) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        path(path), delay(delay)
    {
        this->id = Online::Instance()->GetOriginalID(id);
    }

    binn* AudioPlaySoundGroupVoiceMessage::Serialize(void* object) {
        AudioPlaySoundGroupVoiceMessage* t = static_cast<AudioPlaySoundGroupVoiceMessage*>(object);
        binn* l = binn_object();

        binn_object_set_std_string(l, "path", t->path);
        binn_object_set_int32(l, "id", t->id);
        binn_object_set_float(l, "delay", t->delay);

        return l;
    }

    void AudioPlaySoundGroupVoiceMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundGroupVoiceMessage* t = static_cast<AudioPlaySoundGroupVoiceMessage*>(object);

        binn_object_get_std_string(l, "path", &t->path);
        binn_object_get_int32(l, "id", &t->id);
        binn_object_get_float(l, "delay", &t->delay);
    }

    void AudioPlaySoundGroupVoiceMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundGroupVoiceMessage* t = static_cast<AudioPlaySoundGroupVoiceMessage*>(object);
        ObjectID object_id = Online::Instance()->GetOriginalID(t->id);

        SceneGraph* scene_graph = Engine::Instance()->GetSceneGraph();
        if(scene_graph != nullptr) {
            Object* obj = scene_graph->GetObjectFromID(object_id);
            if(obj != nullptr && obj->GetType() == _movement_object) {
                MovementObject* movement_object = (MovementObject*) obj;
                movement_object->PlaySoundGroupVoice(t->path, t->delay);
            } else {
                LOGW << "Unable to find movement object with ID: " << object_id << " (" << t->id << ")" << std::endl;
            }
        } else {
            LOGW << "Unable to apply group voice sound due to missing scene graph" << std::endl;
        }
    }

    void* AudioPlaySoundGroupVoiceMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundGroupVoiceMessage("", 0, 0.0f);
    }

    void AudioPlaySoundGroupVoiceMessage::Destroy(void* object) {
        AudioPlaySoundGroupVoiceMessage* t = static_cast<AudioPlaySoundGroupVoiceMessage*>(object);
        t->~AudioPlaySoundGroupVoiceMessage();
    }
}
