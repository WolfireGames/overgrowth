//-----------------------------------------------------------------------------
//           Name: test_message.cpp
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
#include "test_message.h"

#include <Logging/loghandler.h>

#include <iostream>
#include <string>

using std::endl;
using std::string;

namespace OnlineMessages {
TestMessage::TestMessage(string message) : OnlineMessageBase(OnlineMessageCategory::TRANSIENT),
                                           message(message) {
}

binn* TestMessage::Serialize(void* object) {
    TestMessage* test_message = static_cast<TestMessage*>(object);

    binn* l = binn_object();

    binn_object_set_str(l, "message", test_message->message.c_str());

    return l;
}

void TestMessage::Deserialize(void* object, binn* l) {
    TestMessage* test_message = static_cast<TestMessage*>(object);

    char* message;
    binn_object_get_str(l, "message", &message);

    test_message->message = string(message);
}

void TestMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
    TestMessage* test_message = static_cast<TestMessage*>(object);
    LOGW << "TestMessage Received and being Executed: " << test_message->message << endl;
}

void* TestMessage::Construct(void* mem) {
    return new (mem) TestMessage("");
}

void TestMessage::Destroy(void* object) {
    TestMessage* test_message = static_cast<TestMessage*>(object);
    test_message->~TestMessage();
}
}  // namespace OnlineMessages
