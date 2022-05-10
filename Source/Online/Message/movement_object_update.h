//-----------------------------------------------------------------------------
//           Name: movement_object_update.h
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

#include "online_message_base.h"

#include <Online/online_datastructures.h>
#include <Objects/riggedobject.h>
#include <Objects/movementobject.h>

namespace OnlineMessages {
class MovementObjectUpdate : public OnlineMessageBase {
   public:
    float timestamp;  // needed, but maybe uint64 bit instead?
    CommonObjectID identifier;

    vec3 position;  // needed for client side culling, either send this or disable culling
    vec3 velocity;
    vec3 facing;  // Needed for correct facing for audio listener
    RiggedObjectFrame rigged_body_frame;

   public:
    MovementObjectUpdate(MovementObject* mo);
    MovementObjectUpdate();

    static binn* Serialize(void* object);
    static void Deserialize(void* object, binn* l);
    static void Execute(const OnlineMessageRef& ref, void* object, PeerID peer);
    static void* Construct(void* mem);
    static void Destroy(void* object);
};
}  // namespace OnlineMessages
