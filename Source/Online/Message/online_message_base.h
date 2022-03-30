//-----------------------------------------------------------------------------
//           Name: online_message_base.h
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
#pragma once


#include <Online/online_datastructures.h>

#include <Math/vec3.h>

#include <binn.h>

#include <string>


//These are message types, they control the behaviour and allowance of a message
//this only controls messages from the host to the clients, messages sent from the client are always
//treated as equivalent to being TRANSIENT, as the client doesn't maintain session state.
enum class OnlineMessageCategory {
    TRANSIENT, //Messages sent to a single client before a client has loaded a level and is ready to receive all messages
    LEVEL_TRANSIENT, //Messages sent to a single or all clients when a level has fully loaded
    LEVEL_PERSISTENT //Persistent level messages sent to all clients, and future new client who connect.
};


class OnlineMessageBase {
public:
    bool reliable_delivery;
    OnlineMessageCategory cat;
public:
    OnlineMessageBase(OnlineMessageCategory cat);
};
