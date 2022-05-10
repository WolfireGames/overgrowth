//-----------------------------------------------------------------------------
//           Name: angelscript_object_data.cpp
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
#include "angelscript_object_data.h"

#include <Online/online.h>
#include <Main/engine.h>

namespace OnlineMessages {
AngelscriptObjectData::AngelscriptObjectData() : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                 object_id(-1),
                                                 state(0),
                                                 data() {
}

AngelscriptObjectData::AngelscriptObjectData(ObjectID object_id, uint32_t state, vector<uint32_t> data) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                                          state(state),
                                                                                                          data(data) {
    this->object_id = Online::Instance()->GetOriginalID(object_id);
}

binn* AngelscriptObjectData::Serialize(void* object) {
    AngelscriptObjectData* ad = static_cast<AngelscriptObjectData*>(object);
    binn* l = binn_object();

    binn_object_set_int32(l, "id", ad->object_id);
    binn_object_set_uint32(l, "s", ad->state);
    binn_object_set_blob(l, "b", ad->data.data(), ad->data.size() * sizeof(uint32_t));

    return l;
}

void AngelscriptObjectData::Deserialize(void* object, binn* l) {
    AngelscriptObjectData* ad = static_cast<AngelscriptObjectData*>(object);

    binn_object_get_int32(l, "id", &ad->object_id);
    binn_object_get_uint32(l, "s", &ad->state);
    void* data_ptr;
    int data_size;
    binn_object_get_blob(l, "b", &data_ptr, &data_size);
    ad->data.resize(data_size / sizeof(uint32_t));
    memcpy(ad->data.data(), data_ptr, data_size);
}

void AngelscriptObjectData::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    AngelscriptObjectData* ad = static_cast<AngelscriptObjectData*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(ad->object_id);

    SceneGraph* sg = Engine::Instance()->GetSceneGraph();

    if (sg != nullptr) {
        MovementObject* mo = static_cast<MovementObject*>(sg->GetObjectFromID(object_id));

        if (mo != nullptr) {
            mo->addAngelScriptUpdate(ad->state, ad->data);
        }
    }
}

void* AngelscriptObjectData::Construct(void* mem) {
    return new (mem) AngelscriptObjectData();
}

void AngelscriptObjectData::Destroy(void* object) {
    AngelscriptObjectData* ad = static_cast<AngelscriptObjectData*>(object);
    ad->~AngelscriptObjectData();
}
}  // namespace OnlineMessages
