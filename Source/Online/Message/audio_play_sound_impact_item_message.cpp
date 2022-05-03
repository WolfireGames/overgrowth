//-----------------------------------------------------------------------------
//           Name: audio_play_sound_impact_item_message.cpp
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
#include "audio_play_sound_impact_item_message.h"

#include <Objects/itemobject.h>

#include <Main/engine.h>
#include <Utility/binn_util.h>
#include <Online/online.h>

namespace OnlineMessages {
    AudioPlaySoundImpactItemMessage::AudioPlaySoundImpactItemMessage(ObjectID id, vec3 pos, vec3 normal, float impulse) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        pos(pos),
        normal(normal),
        impulse(impulse) {

        this->id = Online::Instance()->GetOriginalID(id);
    }

    binn* AudioPlaySoundImpactItemMessage::Serialize(void* object) {
        AudioPlaySoundImpactItemMessage* t = static_cast<AudioPlaySoundImpactItemMessage*>(object);
        binn* l = binn_object();

        binn_object_set_int32(l, "id", t->id);
        binn_object_set_vec3(l, "pos", t->pos);
        binn_object_set_vec3(l, "normal", t->normal);
        binn_object_set_float(l, "impulse", t->impulse);

        return l;
    }

    void AudioPlaySoundImpactItemMessage::Deserialize(void* object, binn* l) {
        AudioPlaySoundImpactItemMessage* t = static_cast<AudioPlaySoundImpactItemMessage*>(object);

        binn_object_get_int32(l, "id", &t->id);
        binn_object_get_vec3(l, "pos", &t->pos);
        binn_object_get_vec3(l, "normal", &t->normal);
        binn_object_get_float(l, "impulse", &t->impulse);
    }

    void AudioPlaySoundImpactItemMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AudioPlaySoundImpactItemMessage* t = static_cast<AudioPlaySoundImpactItemMessage*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(t->id);

        SceneGraph* scene_graph = Engine::Instance()->GetSceneGraph();
        if(scene_graph != nullptr) {
            Object* obj = scene_graph->GetObjectFromID(object_id);
            if(obj != nullptr && obj->GetType() == _item_object) {
                ItemObject* item_object = (ItemObject*) obj;
                item_object->PlayImpactSound(t->pos, t->normal, t->impulse);
            } else {
                LOGW << "Unable to find item with ID: " << object_id << " (" << t->id << ")" << std::endl;
            }
        } else {
            LOGW << "Unable to apply impact sound due to missing scene graph" << std::endl;
        }
    }

    void* AudioPlaySoundImpactItemMessage::Construct(void *mem) {
        return new(mem) AudioPlaySoundImpactItemMessage(0, vec3(), vec3(), 0.0f);
    }

    void AudioPlaySoundImpactItemMessage::Destroy(void* object) {
        AudioPlaySoundImpactItemMessage* t = static_cast<AudioPlaySoundImpactItemMessage*>(object);
        t->~AudioPlaySoundImpactItemMessage();
    }
}
