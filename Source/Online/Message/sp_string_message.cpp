//-----------------------------------------------------------------------------
//           Name: sp_string_message.cpp
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
#include "sp_string_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
SPStringMessage::SPStringMessage(ObjectID param_id, const std::string& key_name, const std::string& value, ScriptParamEditorType::Type editor_type, const std::string& editor_details) : OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
                                                                                                                                                                                         key_name(key_name),
                                                                                                                                                                                         value(value),
                                                                                                                                                                                         editor_type(editor_type),
                                                                                                                                                                                         editor_details(editor_details) {
    this->param_id = Online::Instance()->GetOriginalID(param_id);
}

binn* SPStringMessage::Serialize(void* object) {
    SPStringMessage* t = static_cast<SPStringMessage*>(object);
    binn* l = binn_object();

    binn_object_set_int32(l, "param_id", t->param_id);
    binn_object_set_std_string(l, "key_name", t->key_name);
    binn_object_set_std_string(l, "value", t->value);
    binn_object_set_uint8(l, "editor_type", t->editor_type);
    binn_object_set_std_string(l, "editor_details", t->editor_details);

    return l;
}

void SPStringMessage::Deserialize(void* object, binn* l) {
    SPStringMessage* t = static_cast<SPStringMessage*>(object);

    binn_object_get_int32(l, "param_id", &t->param_id);
    binn_object_get_std_string(l, "key_name", &t->key_name);
    binn_object_get_std_string(l, "value", &t->value);

    uint8_t editor_type;
    binn_object_get_uint8(l, "editor_type", &editor_type);
    t->editor_type = (ScriptParamEditorType::Type)editor_type;

    binn_object_get_std_string(l, "editor_details", &t->editor_details);
}

void SPStringMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    SPStringMessage* t = static_cast<SPStringMessage*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(t->param_id);

    ScriptParam param;
    param.SetString(t->value);
    param.editor().SetType(t->editor_type);
    param.editor().SetDetails(t->editor_details);

    ScriptParams* params = Online::Instance()->GetScriptParamsFromID(object_id);
    if (params != nullptr) {
        params->InsertScriptParam(t->key_name, param);
        Online::Instance()->UpdateMovementObjectFromID(object_id);
    } else {
        LOGW << "Unable to apply script param update for param_id: " << object_id << " (" << t->param_id << ")" << endl;
    }
}

void* SPStringMessage::Construct(void* mem) {
    return new (mem) SPStringMessage(0, "", "", ScriptParamEditorType::Type::UNDEFINED, "");
}

void SPStringMessage::Destroy(void* object) {
    SPStringMessage* t = static_cast<SPStringMessage*>(object);
    t->~SPStringMessage();
}
}  // namespace OnlineMessages
