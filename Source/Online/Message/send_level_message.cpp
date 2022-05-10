//-----------------------------------------------------------------------------
//           Name: send_level_message.cpp
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
#include "send_level_message.h"

#include <Main/engine.h>
#include <Game/level.h>

namespace OnlineMessages {
SendLevelMessage::SendLevelMessage(string msg) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                 msg(msg) {
}

binn* SendLevelMessage::Serialize(void* object) {
    binn* l = binn_object();

    SendLevelMessage* lm = static_cast<SendLevelMessage*>(object);

    binn_object_set_std_string(l, "msg", lm->msg);

    return l;
}

void SendLevelMessage::Deserialize(void* object, binn* l) {
    SendLevelMessage* lm = static_cast<SendLevelMessage*>(object);

    binn_object_get_std_string(l, "msg", &lm->msg);
}

void SendLevelMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    SendLevelMessage* lm = static_cast<SendLevelMessage*>(object);

    SceneGraph* sg = Engine::Instance()->GetSceneGraph();

    if (sg != nullptr) {
        sg->level->Message(lm->msg);
    } else {
        LOGW << "Received level message: \"" << lm->msg << "\" but SceneGraph is nullptr" << std::endl;
    }
}

void* SendLevelMessage::Construct(void* mem) {
    return new (mem) SendLevelMessage("");
}

void SendLevelMessage::Destroy(void* object) {
    SendLevelMessage* lm = static_cast<SendLevelMessage*>(object);
    lm->~SendLevelMessage();
}
}  // namespace OnlineMessages
