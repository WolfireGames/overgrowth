//-----------------------------------------------------------------------------
//           Name: material_sound_event.cpp
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
#include "material_sound_event.h"

#include <Main/engine.h>
#include <Online/online.h>

namespace OnlineMessages {
MaterialSoundEvent::MaterialSoundEvent(ObjectID object_id, const string& ev, const vec3& pos, float gain) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                                            event_name(ev),
                                                                                                            pos(pos),
                                                                                                            gain(gain) {
    this->object_id = Online::Instance()->GetOriginalID(object_id);
}

binn* MaterialSoundEvent::Serialize(void* object) {
    MaterialSoundEvent* mse = static_cast<MaterialSoundEvent*>(object);

    binn* l = binn_object();

    binn_object_set_int32(l, "id", mse->object_id);
    binn_object_set_std_string(l, "event", mse->event_name);
    binn_object_set_vec3(l, "pos", mse->pos);
    binn_object_set_float(l, "gain", mse->gain);

    return l;
}

void MaterialSoundEvent::Deserialize(void* object, binn* l) {
    MaterialSoundEvent* mse = static_cast<MaterialSoundEvent*>(object);

    binn_object_get_int32(l, "id", &mse->object_id);
    binn_object_get_std_string(l, "event", &mse->event_name);
    binn_object_get_vec3(l, "pos", &mse->pos);
    binn_object_get_float(l, "gain", &mse->gain);
}

void MaterialSoundEvent::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    MaterialSoundEvent* mse = static_cast<MaterialSoundEvent*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(mse->object_id);

    SceneGraph* sg = Engine::Instance()->GetSceneGraph();

    if (sg != nullptr) {
    }
}

void* MaterialSoundEvent::Construct(void* mem) {
    return new (mem) MaterialSoundEvent(-1, "", vec3(), 0.0f);
}

void MaterialSoundEvent::Destroy(void* object) {
    MaterialSoundEvent* mse = static_cast<MaterialSoundEvent*>(object);
    mse->~MaterialSoundEvent();
}
}  // namespace OnlineMessages
