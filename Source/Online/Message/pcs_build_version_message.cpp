//-----------------------------------------------------------------------------
//           Name: pcs_build_version_message.cpp
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
#include "pcs_build_version_message.h"

#include <Online/online.h>

namespace OnlineMessages {
    PCSBuildVersionMessage::PCSBuildVersionMessage(const int32_t build_id) :
        OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
        build_id(build_id)
    {

    }

    binn* PCSBuildVersionMessage::Serialize(void* object) {
        PCSBuildVersionMessage* t = static_cast<PCSBuildVersionMessage*>(object);
        binn* l = binn_object();

        binn_object_set_int32(l, "build_id", t->build_id);

        return l;
    }

    void PCSBuildVersionMessage::Deserialize(void* object, binn* l) {
        PCSBuildVersionMessage* t = static_cast<PCSBuildVersionMessage*>(object);

        binn_object_get_int32(l, "build_id", &t->build_id);
    }

    void PCSBuildVersionMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        PCSBuildVersionMessage* t = static_cast<PCSBuildVersionMessage*>(object);

        if (Online::Instance()->IsHosting()) {
            Online::Instance()->online_session->client_connection_manager.ApplyPackage(from, t);
        }
    }

    void* PCSBuildVersionMessage::Construct(void* mem) {
        return new (mem) PCSBuildVersionMessage(-1);
    }

    void PCSBuildVersionMessage::Destroy(void* object) {
        PCSBuildVersionMessage* bvr = static_cast<PCSBuildVersionMessage*>(object);
        bvr->~PCSBuildVersionMessage();
    }
}
