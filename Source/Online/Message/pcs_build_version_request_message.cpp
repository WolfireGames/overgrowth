//-----------------------------------------------------------------------------
//           Name: pcs_build_version_request_message.cpp
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
#include "pcs_build_version_request_message.h"

#include <Online/Message/pcs_build_version_message.h>
#include <Online/online.h>

#include <Version/version.h>

namespace OnlineMessages {
    PCSBuildVersionRequestMessage::PCSBuildVersionRequestMessage() :
        OnlineMessageBase(OnlineMessageCategory::TRANSIENT)
    {

    }

    binn* PCSBuildVersionRequestMessage::Serialize(void* object) {
        return binn_object();
    }

    void PCSBuildVersionRequestMessage::Deserialize(void* object, binn* l) {

    }

    void PCSBuildVersionRequestMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        Online::Instance()->SendTo<OnlineMessages::PCSBuildVersionMessage>(Online::Instance()->GetPeerFromID(from)->conn_id, GetBuildID());
    }

    void* PCSBuildVersionRequestMessage::Construct(void* mem) {
        return new (mem) PCSBuildVersionRequestMessage();
    }

    void PCSBuildVersionRequestMessage::Destroy(void* object) {
        PCSBuildVersionRequestMessage* bvr = static_cast<PCSBuildVersionRequestMessage*>(object);
        bvr->~PCSBuildVersionRequestMessage();
    }
}
