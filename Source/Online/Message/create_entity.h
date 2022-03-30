//-----------------------------------------------------------------------------
//           Name: create_entity.h
//      Developer: Wolfire Games LLC
//         Author: Max Danielsson
//    Description: Create entity message, sent by the host to clients to inform
//                 them of construction of a new entity.
//
//                 Clients will request creation of an entity to the host via a
//                 different message.
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
#include <Math/vec3.h>

#include <string>

using std::string;

namespace OnlineMessages {
    class CreateEntity : public OnlineMessageBase {
    private:
        string path;
        vec3 pos;
        ObjectID entity_object_id;

    public:
        CreateEntity(const string& path, const vec3& pos, ObjectID entity_object_id);

        static binn* Serialize(void* object);
        static void Deserialize(void* object, binn* l);
        static void Execute(const OnlineMessageRef& ref, void* object, PeerID peer);
        static void* Construct(void *mem);
        static void Destroy(void* object);
    };
}
