//-----------------------------------------------------------------------------
//           Name: sp_rename_message.cpp
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
#include "sp_rename_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    SPRenameMessage::SPRenameMessage(ObjectID param_id, const std::string& current_key_name, const std::string& new_key_name) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
        current_key_name(current_key_name), new_key_name(new_key_name)
    {
        this->param_id = Online::Instance()->GetOriginalID(param_id);
    }

    binn* SPRenameMessage::Serialize(void* object) {
        SPRenameMessage* t = static_cast<SPRenameMessage*>(object);
        binn* l = binn_object();

        binn_object_set_int32(l, "param_id", t->param_id);
        binn_object_set_std_string(l, "current_key_name", t->current_key_name);
        binn_object_set_std_string(l, "new_key_name", t->new_key_name);

        return l;
    }

    void SPRenameMessage::Deserialize(void* object, binn* l) {
        SPRenameMessage* t = static_cast<SPRenameMessage*>(object);

        binn_object_get_int32(l, "param_id", &t->param_id);
        binn_object_get_std_string(l, "current_key_name", &t->current_key_name);
        binn_object_get_std_string(l, "new_key_name", &t->new_key_name);
    }

    void SPRenameMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        SPRenameMessage* t = static_cast<SPRenameMessage*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(t->param_id);

        ScriptParams* params = Online::Instance()->GetScriptParamsFromID(object_id);
        if(params != nullptr) {
            if (params->RenameParameterKey(t->current_key_name, t->new_key_name)) {
                Online::Instance()->UpdateMovementObjectFromID(object_id);
            } else {
                LOGE << "Tried renaming \"" << t->current_key_name << "\" to \"" << t->new_key_name << "\" but failed finding key to rename" << std::endl;
            }
        } else {
            LOGW << "Tried renaming \"" << t->current_key_name << "\" to \"" << t->new_key_name << "\" but couldn't find params with id: " << object_id << " (" << t->param_id << ")" << std::endl;
        }
    }

    void* SPRenameMessage::Construct(void *mem) {
        return new(mem) SPRenameMessage(0, "", "");
    }

    void SPRenameMessage::Destroy(void* object) {
        SPRenameMessage* t = static_cast<SPRenameMessage*>(object);
        t->~SPRenameMessage();
    }
}
