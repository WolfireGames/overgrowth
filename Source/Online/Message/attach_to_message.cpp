//-----------------------------------------------------------------------------
//           Name: attach_to_message.cpp
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
#include "attach_to_message.h"

#include <Main/engine.h>
#include <Online/online.h>
#include <Utility/binn_util.h>

namespace OnlineMessages {
    AttachToMessage::AttachToMessage(ObjectID parent_id, ObjectID child_id, uint32_t bone_id, bool attach, bool mirrored) :
        OnlineMessageBase(OnlineMessageCategory::LEVEL_PERSISTENT),
        bone_id(bone_id), attach(attach), mirrored(mirrored) {

        this->parent_id = Online::Instance()->GetOriginalID(parent_id);
        this->child_id = Online::Instance()->GetOriginalID(child_id);
    }

    binn* AttachToMessage::Serialize(void* object) {
        AttachToMessage* t = static_cast<AttachToMessage*>(object);
        binn* l = binn_object();

        binn_object_set_int32(l, "parent_id", t->parent_id);
        binn_object_set_int32(l, "child_id", t->child_id);
        binn_object_set_uint32(l, "bone_id", t->bone_id);
        binn_object_set_bool(l, "attach", t->attach);
        binn_object_set_bool(l, "mirrored", t->mirrored);

        return l;
    }

    void AttachToMessage::Deserialize(void* object, binn* l) {
        AttachToMessage* t = static_cast<AttachToMessage*>(object);

        binn_object_get_int32(l, "parent_id", &t->parent_id);
        binn_object_get_int32(l, "child_id", &t->child_id);
        binn_object_get_uint32(l, "bone_id", &t->bone_id);

        BOOL attach, mirrored;
        binn_object_get_bool(l, "attach", &attach);
        binn_object_get_bool(l, "mirrored", &mirrored);
        t->attach = attach;
        t->mirrored = mirrored;
    }

    void AttachToMessage::Execute(const OnlineMessageRef& ref, void* object, PeerID from) {
        AttachToMessage* t = static_cast<AttachToMessage*>(object);
        ObjectID parent_object_id = Online::Instance()->GetObjectID(t->parent_id);
        ObjectID child_object_id = Online::Instance()->GetObjectID(t->child_id);

        SceneGraph* graph = Engine::Instance()->GetSceneGraph();

        // We need to make sure that we actually know what objects
        // the sender is referencing
        Object * parent = graph->GetObjectFromID(parent_object_id);
        Object * child = graph->GetObjectFromID(child_object_id);

        if (parent != nullptr && child != nullptr) {
            if (t->attach) {
                child->SetParent(parent);
                if (child->GetType() == EntityType::_item_object) {
                    ItemObject* item_object = (ItemObject*)child;
                    MovementObject* movement_object = (MovementObject*)parent;
                    AttachmentSlotList attachment_slots;
                    movement_object->rigged_object()->AvailableItemSlots(item_object->item_ref(), &attachment_slots);
                    for (AttachmentSlotList::iterator iterator = attachment_slots.begin(); iterator != attachment_slots.end(); ++iterator) {
                        AttachmentSlot& slot = (*iterator);
                        if (slot.type == (AttachmentType)t->bone_id && slot.mirrored == t->mirrored) {
                            movement_object->AttachItemToSlotEditor(item_object->GetID(), slot.type, slot.mirrored, slot.attachment_ref, true);
                            break;
                        }
                    }

                } else if (parent->GetType() == EntityType::_movement_object) {
                    MovementObject * mov = (MovementObject *)parent;
                    RiggedObject * rigged = mov->rigged_object();
                    // this is copied and pasted from the "original code" -rewrite so that it does not default construct inside the vector
                    rigged->children.resize(rigged->children.size() + 1);

                    // dummy data
                    AttachedEnvObject &attached_env_object = rigged->children.back();
                    attached_env_object.bone_connection_dirty = true;
                    attached_env_object.direct_ptr = child;
                    attached_env_object.bone_connects[0].bone_id = t->bone_id;
                    attached_env_object.bone_connects[0].num_connections = 1;
                }
            } else {
                if (child->GetType() == EntityType::_item_object) {
                    parent->Disconnect(*child, true, false);
                } else {
                    parent->ChildLost(child);
                }
            }
        }
    }

    void* AttachToMessage::Construct(void *mem) {
        return new(mem) AttachToMessage(0, 0, 0, false, false);
    }

    void AttachToMessage::Destroy(void* object) {
        AttachToMessage* t = static_cast<AttachToMessage*>(object);
        t->~AttachToMessage();
    }
}
