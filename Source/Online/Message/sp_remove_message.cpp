//-----------------------------------------------------------------------------
//           Name: sp_remove_message.cpp
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
#include "sp_remove_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
SPRemoveMessage::SPRemoveMessage(ObjectID param_id, const std::string& key_name) : OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
                                                                                   key_name(key_name) {
    this->param_id = Online::Instance()->GetOriginalID(param_id);
}

binn* SPRemoveMessage::Serialize(void* object) {
    SPRemoveMessage* t = static_cast<SPRemoveMessage*>(object);
    binn* l = binn_object();

    binn_object_set_int32(l, "param_id", t->param_id);
    binn_object_set_std_string(l, "key_name", t->key_name);

    return l;
}

void SPRemoveMessage::Deserialize(void* object, binn* l) {
    SPRemoveMessage* t = static_cast<SPRemoveMessage*>(object);

    binn_object_get_int32(l, "param_id", &t->param_id);
    binn_object_get_std_string(l, "key_name", &t->key_name);
}

void SPRemoveMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    SPRemoveMessage* t = static_cast<SPRemoveMessage*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(t->param_id);

    ScriptParams* params = Online::Instance()->GetScriptParamsFromID(object_id);
    if (params != nullptr) {
        params->ASRemove(t->key_name);
        Online::Instance()->UpdateMovementObjectFromID(object_id);
    }
}

void* SPRemoveMessage::Construct(void* mem) {
    return new (mem) SPRemoveMessage(0, "");
}

void SPRemoveMessage::Destroy(void* object) {
    SPRemoveMessage* t = static_cast<SPRemoveMessage*>(object);
    t->~SPRemoveMessage();
}
}  // namespace OnlineMessages
