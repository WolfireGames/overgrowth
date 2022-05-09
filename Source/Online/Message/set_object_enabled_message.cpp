//-----------------------------------------------------------------------------
//           Name: set_object_enabled_message.cpp
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
#include "set_object_enabled_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
SetObjectEnabledMessage::SetObjectEnabledMessage(ObjectID object_id, bool is_enabled) : OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
                                                                                        is_enabled(is_enabled) {
    this->object_id = Online::Instance()->GetOriginalID(object_id);
}

binn* SetObjectEnabledMessage::Serialize(void* object) {
    SetObjectEnabledMessage* t = static_cast<SetObjectEnabledMessage*>(object);
    binn* l = binn_object();

    binn_object_set_int32(l, "object_id", t->object_id);
    binn_object_set_bool(l, "is_enabled", t->is_enabled);

    return l;
}

void SetObjectEnabledMessage::Deserialize(void* object, binn* l) {
    SetObjectEnabledMessage* t = static_cast<SetObjectEnabledMessage*>(object);
    BOOL temp;
    binn_object_get_bool(l, "is_enabled", &temp);
    binn_object_get_int32(l, "object_id", &t->object_id);

    t->is_enabled = temp;
}

void SetObjectEnabledMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    SetObjectEnabledMessage* t = static_cast<SetObjectEnabledMessage*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(t->object_id);

    Object* obj = Engine::Instance()->GetSceneGraph()->GetObjectFromID(object_id);
    if (obj) {
        obj->SetEnabled(t->is_enabled);
    } else {
        LOGW << "We were sent an update for \"" << object_id << " (" << t->object_id << ")\", but were not able to apply it due to not finding said object" << endl;
    }
}

void* SetObjectEnabledMessage::Construct(void* mem) {
    return new (mem) SetObjectEnabledMessage(0, true);
}

void SetObjectEnabledMessage::Destroy(void* object) {
    SetObjectEnabledMessage* t = static_cast<SetObjectEnabledMessage*>(object);
    t->~SetObjectEnabledMessage();
}
}  // namespace OnlineMessages
