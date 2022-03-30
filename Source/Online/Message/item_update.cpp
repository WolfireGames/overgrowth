//-----------------------------------------------------------------------------
//           Name: item_update.cpp
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
#include "item_update.h"

#include <Utility/binn_util.h>
#include <Main/engine.h>
#include <Online/online.h>

extern Timer game_timer;

namespace OnlineMessages {
    ItemUpdate::ItemUpdate() :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        id(-1), timestamp(0), transform(mat4())
    {

    }

    ItemUpdate::ItemUpdate(ItemObject* object) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
        timestamp(game_timer.GetWallTime()), transform(object->GetTransform())
    {
        this->id = Online::Instance()->GetOriginalID(object->GetID());
    }

    binn* ItemUpdate::Serialize(void* object) {
        ItemUpdate* iu = static_cast<ItemUpdate*>(object);

        binn* l = binn_object();

        binn_object_set_int32(l, "id", iu->id);
        binn_object_set_float(l, "ts", iu->timestamp);
        binn_object_set_mat4(l, "t", iu->transform);

        return l;
    }

    void ItemUpdate::Deserialize(void* object, binn* l) {
        ItemUpdate* iu = static_cast<ItemUpdate*>(object);

        binn_object_get_int32(l, "id", &iu->id);
        binn_object_get_float(l, "ts", &iu->timestamp);
        binn_object_get_mat4(l, "t", &iu->transform);
    }

    void ItemUpdate::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
        ItemUpdate* iu = static_cast<ItemUpdate*>(object);
        ObjectID object_id = Online::Instance()->GetObjectID(iu->id);
        SceneGraph* sg = Engine::Instance()->GetSceneGraph();

        if(sg != nullptr) {
            Object* obj = sg->GetObjectFromID(object_id);
            if(obj) {
                ItemObject* item = static_cast<ItemObject*>(obj);

                ItemObjectFrame iof;

                iof.host_walltime = iu->timestamp;
                iof.transform = iu->transform;

                item->incoming_online_item_frames.push_back(iof);
            }
        }
    }

    void* ItemUpdate::Construct(void* mem) {
        return new(mem) ItemUpdate();
    }

    void ItemUpdate::Destroy(void* object) {
        ItemUpdate* iu = static_cast<ItemUpdate*>(object);
        iu->~ItemUpdate();
    }
}
