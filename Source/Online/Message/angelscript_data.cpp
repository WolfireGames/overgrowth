//-----------------------------------------------------------------------------
//           Name: angelscript_data.cpp
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
#include "angelscript_data.h"
#include <Online/online.h>

namespace OnlineMessages {
AngelscriptData::AngelscriptData() : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                     state(0),
                                     data(),
                                     is_persistent_sync(false) {
}

AngelscriptData::AngelscriptData(uint32_t state, vector<char> data, bool is_persistent_sync) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                               state(state),
                                                                                               data(data),
                                                                                               is_persistent_sync(is_persistent_sync) {
}

binn* AngelscriptData::Serialize(void* object) {
    AngelscriptData* ad = static_cast<AngelscriptData*>(object);
    binn* l = binn_object();

    binn_object_set_uint32(l, "s", ad->state);
    binn_object_set_blob(l, "b", ad->data.data(), ad->data.size());
    binn_object_set_bool(l, "sync", ad->is_persistent_sync);

    return l;
}

void AngelscriptData::Deserialize(void* object, binn* l) {
    AngelscriptData* ad = static_cast<AngelscriptData*>(object);
    binn_object_get_uint32(l, "s", &ad->state);
    void* data_ptr;
    int data_size;
    binn_object_get_blob(l, "b", &data_ptr, &data_size);
    ad->data.resize(data_size);
    memcpy(ad->data.data(), data_ptr, data_size);
    BOOL is_persistent_sync_binn;
    binn_object_get_bool(l, "sync", &is_persistent_sync_binn);
    ad->is_persistent_sync = is_persistent_sync_binn;
}

void AngelscriptData::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    AngelscriptData* ad = static_cast<AngelscriptData*>(object);

    AngelScriptUpdate asu;

    asu.data = ad->data;
    asu.state = ad->state;

    if (ad->is_persistent_sync) {
        Online::Instance()->online_session->peer_queued_sync_updates.push(asu);
    } else {
        Online::Instance()->online_session->peer_queued_level_updates.push(asu);
    }
}

void* AngelscriptData::Construct(void* mem) {
    return new (mem) AngelscriptData();
}

void AngelscriptData::Destroy(void* object) {
    AngelscriptData* ad = static_cast<AngelscriptData*>(object);
    ad->~AngelscriptData();
}
}  // namespace OnlineMessages
