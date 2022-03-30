//-----------------------------------------------------------------------------
//           Name: cut_line.h
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

#include <Objects/movementobject.h>

#include <vector>

using std::vector;

namespace OnlineMessages {
    class CutLine : public OnlineMessageBase {
    private:
        CommonObjectID id;

    public:
        vec3 points[3];
        vec3 pos;
        vec3 normal;
        vec3 dir;
        int type;
        int depth;
        int num_hit;
        vector<int> hit_list;

    public:
        CutLine();
        CutLine(ObjectID id,
            const vec3* points,
            const vec3& pos,
            const vec3& normal,
            const vec3& dir,
            int type,
            int depth,
            int num_hit,
            const vector<int>& hit_list);

        static binn* Serialize(void* object);
        static void Deserialize(void* object, binn* l);
        static void Execute(const OnlineMessageRef& ref, void* object, PeerID peer);
        static void* Construct(void *mem);
        static void Destroy(void* object);
    };
}
