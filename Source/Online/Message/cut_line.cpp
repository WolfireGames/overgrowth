//-----------------------------------------------------------------------------
//           Name: cut_line.cpp
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
#include "cut_line.h"

#include <Main/engine.h>
#include <Online/online.h>

namespace OnlineMessages {
CutLine::CutLine() : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                     id(-1),
                     pos(),
                     normal(),
                     dir(),
                     type(0),
                     depth(0),
                     num_hit(0),
                     hit_list() {
}

CutLine::CutLine(ObjectID id, const vec3* points, const vec3& pos, const vec3& normal, const vec3& dir, int type, int depth, int num_hit, const vector<int>& hit_list) : OnlineMessageBase::OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                                                                                                         pos(pos),
                                                                                                                                                                         normal(normal),
                                                                                                                                                                         dir(dir),
                                                                                                                                                                         type(type),
                                                                                                                                                                         depth(depth),
                                                                                                                                                                         num_hit(num_hit),
                                                                                                                                                                         hit_list(hit_list) {
    this->id = Online::Instance()->GetOriginalID(id);

    for (int i = 0; i < 3; i++) {
        this->points[i] = points[i];
    }
}

binn* CutLine::Serialize(void* object) {
    CutLine* cl = static_cast<CutLine*>(object);

    binn* l = binn_object();

    binn_object_set_int32(l, "id", cl->id);
    binn_object_set_vec3(l, "p0", cl->points[0]);
    binn_object_set_vec3(l, "p1", cl->points[1]);
    binn_object_set_vec3(l, "p2", cl->points[2]);
    binn_object_set_vec3(l, "pos", cl->pos);
    binn_object_set_vec3(l, "n", cl->normal);
    binn_object_set_vec3(l, "d", cl->dir);
    binn_object_set_int32(l, "t", cl->type);
    binn_object_set_int32(l, "de", cl->depth);
    binn_object_set_int32(l, "nh", cl->num_hit);
    binn_object_set_vector_int(l, "hl", cl->hit_list);

    return l;
}

void CutLine::Deserialize(void* object, binn* l) {
    CutLine* cl = static_cast<CutLine*>(object);
    binn_object_get_int32(l, "id", &cl->id);
    binn_object_get_vec3(l, "p0", &cl->points[0]);
    binn_object_get_vec3(l, "p1", &cl->points[1]);
    binn_object_get_vec3(l, "p2", &cl->points[2]);
    binn_object_get_vec3(l, "pos", &cl->pos);
    binn_object_get_vec3(l, "n", &cl->normal);
    binn_object_get_vec3(l, "d", &cl->dir);
    binn_object_get_int32(l, "t", &cl->type);
    binn_object_get_int32(l, "de", &cl->depth);
    binn_object_get_int32(l, "nh", &cl->num_hit);
    binn_object_get_vector_int(l, "hl", &cl->hit_list);
}

void CutLine::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    CutLine* cl = static_cast<CutLine*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(cl->id);

    SceneGraph* sg = Engine::Instance()->GetSceneGraph();

    if (sg != nullptr) {
        MovementObject* mo = static_cast<MovementObject*>(sg->GetObjectFromID(object_id));
        if (mo != nullptr) {
            mo->incoming_cut_lines.push_back(ref);
        }
    }
}

void* CutLine::Construct(void* mem) {
    return new (mem) CutLine();
}

void CutLine::Destroy(void* object) {
    CutLine* cl = static_cast<CutLine*>(object);
    cl->~CutLine();
}
}  // namespace OnlineMessages
