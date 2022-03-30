//-----------------------------------------------------------------------------
//           Name: sp_union_message.cpp
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
#include "sp_union_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    SPUnionMessage::SPUnionMessage(ObjectID param_id, const std::string& key_name, int32_t value, ScriptParam::ScriptParamType type, ScriptParamEditorType::Type editor_type, const std::string& editor_details) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
        key_name(key_name), type(type), editor_type(editor_type), editor_details(editor_details)
    {
        value_int = value;
        this->param_id = Online::Instance()->GetOriginalID(param_id);
    }

    binn* SPUnionMessage::Serialize(void* object) {
        SPUnionMessage* t = static_cast<SPUnionMessage*>(object);
        binn* l = binn_object();

        binn_object_set_int32(l, "param_id", t->param_id);
        binn_object_set_std_string(l, "key_name", t->key_name);
        binn_object_set_uint8(l, "type", t->type);

        if(t->type == ScriptParam::ScriptParamType::FLOAT) {
            binn_object_set_float(l, "value", t->value_float);
        } else if(t->type == ScriptParam::ScriptParamType::INT) {
            binn_object_set_int32(l, "value", t->value_int);
        } else {
            LOGE << "Unhandled type" << endl;
        }

        binn_object_set_uint8(l, "editor_type", t->editor_type);
        binn_object_set_std_string(l, "editor_details", t->editor_details);

        return l;
    }

    void SPUnionMessage::Deserialize(void* object, binn* l) {
        SPUnionMessage* t = static_cast<SPUnionMessage*>(object);

        binn_object_get_int32(l, "param_id", &t->param_id);
        binn_object_get_std_string(l, "key_name", &t->key_name);

        uint8_t type;
        binn_object_get_uint8(l, "type", &type);
        t->type = (ScriptParam::ScriptParamType)type;

        if(t->type == ScriptParam::ScriptParamType::FLOAT) {
            binn_object_get_float(l, "value", &t->value_float);
        } else if(t->type == ScriptParam::ScriptParamType::INT) {
            binn_object_get_int32(l, "value", &t->value_int);
        } else {
            LOGE << "Unhandled type" << endl;
        }

        uint8_t editor_type;
        binn_object_get_uint8(l, "editor_type", &editor_type);
        t->editor_type = (ScriptParamEditorType::Type)editor_type;

        binn_object_get_std_string(l, "editor_details", &t->editor_details);
    }

    void SPUnionMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        SPUnionMessage* t = static_cast<SPUnionMessage*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(t->param_id);

        ScriptParam param;
        if (t->type == ScriptParam::ScriptParamType::FLOAT) {
            param.SetFloat(t->value_float);
        } else {
            param.SetInt(t->value_int);
        }

        param.editor().SetDetails(t->editor_details);
        param.editor().SetType(t->editor_type);

        ScriptParams* params = Online::Instance()->GetScriptParamsFromID(object_id);
        if(params != nullptr) {
            params->InsertScriptParam(t->key_name, param);
            Online::Instance()->UpdateMovementObjectFromID(object_id);
        } else {
            LOGW << "Unable to apply script param update for param_id: " << object_id << " (" << t->param_id << ")" << endl;
        }
    }

    void* SPUnionMessage::Construct(void *mem) {
        return new(mem) SPUnionMessage(0, "", 0, ScriptParam::ScriptParamType::OTHER, ScriptParamEditorType::Type::UNDEFINED, "");
    }

    void SPUnionMessage::Destroy(void* object) {
        SPUnionMessage* t = static_cast<SPUnionMessage*>(object);
        t->~SPUnionMessage();
    }
}
