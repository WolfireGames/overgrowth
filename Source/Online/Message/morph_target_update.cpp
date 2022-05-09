//-----------------------------------------------------------------------------
//           Name: morph_target_update.cpp
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
#include "morph_target_update.h"

#include <Main/engine.h>
#include <Online/online.h>

namespace OnlineMessages {
MorphTargetUpdate::MorphTargetUpdate() : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                         object_id(-1),
                                         disp_weight(0),
                                         mod_weight(0),
                                         name({'\0'}) {
    reliable_delivery = false;
}

MorphTargetUpdate::MorphTargetUpdate(ObjectID object_id, const MorphTargetStateStorage& morph_state) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                                       disp_weight(morph_state.disp_weight),
                                                                                                       mod_weight(morph_state.mod_weight) {
    this->object_id = Online::Instance()->GetOriginalID(object_id);
    strncpy(name.data(), morph_state.name.c_str(), name.size());
    reliable_delivery = false;
}

binn* MorphTargetUpdate::Serialize(void* object) {
    MorphTargetUpdate* mtu = static_cast<MorphTargetUpdate*>(object);
    binn* l = binn_object();

    binn_object_set_int32(l, "o_id", mtu->object_id);
    binn_object_set_float(l, "dw", mtu->disp_weight);
    binn_object_set_float(l, "mw", mtu->mod_weight);
    binn_object_set_str(l, "n", mtu->name.data());

    return l;
}

void MorphTargetUpdate::Deserialize(void* object, binn* l) {
    MorphTargetUpdate* mtu = static_cast<MorphTargetUpdate*>(object);

    binn_object_get_int32(l, "o_id", &mtu->object_id);
    binn_object_get_float(l, "dw", &mtu->disp_weight);
    binn_object_get_float(l, "mw", &mtu->mod_weight);

    char* b_name_str;
    binn_object_get_str(l, "n", &b_name_str);
    strncpy(mtu->name.data(), b_name_str, mtu->name.size());
}

void MorphTargetUpdate::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    MorphTargetUpdate* mtu = static_cast<MorphTargetUpdate*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(mtu->object_id);
    SceneGraph* sg = Engine::Instance()->GetSceneGraph();

    if (sg != nullptr && Online::Instance()->host_started_level) {
        Object* object = sg->GetObjectFromID(object_id);
        if (object != nullptr && object->GetType() == EntityType::_movement_object) {
            MovementObject* mo = static_cast<MovementObject*>(object);
            MorphTargetStateStorage mtss;
            mtss.disp_weight = mtu->disp_weight;
            mtss.mod_weight = mtu->mod_weight;
            mtss.name = string(mtu->name.data());
            mo->rigged_object()->incoming_network_morphs[mtss.name] = mtss;
        } else {
            LOGW << "Received MorphTargetUpdate, but couldn't find an MovementObject with ID of \"" << object_id << " (" << mtu->object_id << ")\"" << std::endl;
        }
    } else {
        LOGW << "Client received MorphTargetUpdate, but wasn't ready to receive it. The host should not be sending this to us right now!" << std::endl;
    }
}

void* MorphTargetUpdate::Construct(void* mem) {
    return new (mem) MorphTargetUpdate();
}

void MorphTargetUpdate::Destroy(void* object) {
    MorphTargetUpdate* mtu = static_cast<MorphTargetUpdate*>(object);
    mtu->~MorphTargetUpdate();
}
}  // namespace OnlineMessages
