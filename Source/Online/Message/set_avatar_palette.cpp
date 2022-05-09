//-----------------------------------------------------------------------------
//           Name: set_avatar_palette.cpp
//      Developer: Wolfire Games LLC
//    Description: Host telling client that palette of movementobject has changed
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
#include "set_avatar_palette.h"

#include <Main/engine.h>
#include <Online/online.h>

namespace OnlineMessages {
SetAvatarPalette::SetAvatarPalette(ObjectID id, const vec3& color, uint8_t channel) : OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
                                                                                      color(color),
                                                                                      channel(channel) {
    this->id = Online::Instance()->GetOriginalID(id);
}

binn* SetAvatarPalette::Serialize(void* object) {
    SetAvatarPalette* t = static_cast<SetAvatarPalette*>(object);

    binn* l = binn_object();

    binn_object_set_int32(l, "id", t->id);
    binn_object_set_vec3(l, "color", t->color);
    binn_object_set_uint8(l, "channel", t->channel);

    return l;
}

void SetAvatarPalette::Deserialize(void* object, binn* l) {
    SetAvatarPalette* t = static_cast<SetAvatarPalette*>(object);

    binn_object_get_int32(l, "id", &t->id);
    binn_object_get_vec3(l, "color", &t->color);
    binn_object_get_uint8(l, "channel", &t->channel);
}

void SetAvatarPalette::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    SetAvatarPalette* sap = static_cast<SetAvatarPalette*>(object);
    ObjectID object_id = Online::Instance()->GetObjectID(sap->id);
    SceneGraph* sg = Engine::Instance()->GetSceneGraph();

    if (sg != nullptr) {
        Object* o = sg->GetObjectFromID(object_id);
        if (o != nullptr && o->GetType() == _movement_object) {
            MovementObject* mov = static_cast<MovementObject*>(o);
            OGPalette palette(*mov->GetPalette());
            palette[sap->channel].color = sap->color;

            mov->ApplyPalette(palette, true);
        }
    }
}

void* SetAvatarPalette::Construct(void* mem) {
    return new (mem) SetAvatarPalette(0, vec3(), 0);
}

void SetAvatarPalette::Destroy(void* object) {
    SetAvatarPalette* sap = static_cast<SetAvatarPalette*>(object);
    sap->~SetAvatarPalette();
}
}  // namespace OnlineMessages
