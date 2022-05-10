//-----------------------------------------------------------------------------
//           Name: group.cpp
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
#include <Math/vec3.h>
#include <Math/vec3math.h>

#include <Internal/snprintf.h>
#include <Internal/common.h>
#include <Internal/error.h>

#include <Objects/group.h>
#include <Editors/actors_editor.h>

#include <cfloat>
#include <cmath>
#include <algorithm>

extern bool g_debug_runtime_disable_group_pre_draw_camera;
extern bool g_debug_runtime_disable_group_pre_draw_frame;

Group::Group() : child_transforms_need_update(false),
                 child_moved(false) {
    box_.dims = vec3(1.0f);
}

Group::~Group() {
}

bool Group::Initialize() {
    return true;
}

int Group::lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal /*=0*/) {
    if (selected_) {
        return LineCheckEditorCube(start, end, point, normal);
    }
    return -1;
}

void Group::GetChildren(std::vector<Object*>* ret_children) {
    ret_children->resize(children.size());
    for (int i = 0, len = children.size(); i < len; ++i) {
        ret_children->at(i) = children[i].direct_ptr;
    }
}

void Group::GetBottomUpCompleteChildren(std::vector<Object*>* ret_children) {
    for (auto& i : children) {
        Object* obj = i.direct_ptr;
        obj->GetBottomUpCompleteChildren(ret_children);
        ret_children->push_back(obj);
    }
}

void Group::GetTopDownCompleteChildren(std::vector<Object*>* ret_children) {
    for (auto& i : children) {
        Object* obj = i.direct_ptr;
        ret_children->push_back(obj);
    }

    for (auto& i : children) {
        Object* obj = i.direct_ptr;
        obj->GetTopDownCompleteChildren(ret_children);
    }
}

void Group::ChildLost(Object* obj) {
    for (int i = 0, len = children.size(); i < len; ++i) {
        if (children[i].direct_ptr == obj) {
            children[i].direct_ptr = children.back().direct_ptr;
            children.resize(children.size() - 1);
            break;
        }
    }
    child_moved = true;
}

void Group::FinalizeLoadedConnections() {
    Object::FinalizeLoadedConnections();
    for (auto& i : children) {
        i.direct_ptr->ReceiveObjectMessage(OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS);
    }
}

bool Group::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (auto new_desc : desc.children) {
            Object* obj = CreateObjectFromDesc(new_desc);
            if (obj) {
                obj->SetParent(this);
                children.resize(children.size() + 1);
                children.back().direct_ptr = obj;
            } else {
                LOGE << "Failed constructing child to Group" << std::endl;
            }
        }
        const EntityDescriptionField* version_edf = desc.GetField(EDF_VERSION);
        if (!version_edf) {
            InitShape();
        }

        InitRelMats();
    }
    return ret;
}

void Group::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddInt(EDF_VERSION, 1);
    for (const auto& i : children) {
        Object* obj = i.direct_ptr;
        if (obj) {
            if (obj == this) {
                FatalError("Error", "Group %d: GetDesc() including copy of itself", GetID());
            }
            desc.children.resize(desc.children.size() + 1);
            obj->GetDesc(desc.children.back());
        }
    }
}

void Group::InitRelMats() {
    for (auto& i : children) {
        Object* obj = i.direct_ptr;
        if (obj && (obj->GetType() == _group || obj->GetType() == _prefab) && !obj->Selected()) {
            Group* child_group = (Group*)obj;
            child_group->InitRelMats();
        }
    }

    // Get child transforms relative to group transform
    for (auto& i : children) {
        Child& child = i;
        Object* obj = i.direct_ptr;
        if (obj) {
            child.rel_rotation = invert(GetRotation()) * obj->GetRotation();
            child.rel_translation = (invert(GetRotation()) * ((obj->GetTranslation() - translation_)) / scale_);
            child.rel_scale = child.rel_rotation * (obj->GetScale() / scale_);
        }
    }
}

void Group::InitShape() {
    if (children.empty()) {
        return;
    }

    for (auto& i : children) {
        Object* obj = i.direct_ptr;
        if (obj && (obj->GetType() == _group || obj->GetType() == _prefab) && !obj->Selected()) {
            Group* child_group = (Group*)obj;
            child_group->InitShape();
        }
    }

    // Get bounding box points of children
    static const int MAX_POINTS = 800;
    int num_points = 0;
    vec3 points[MAX_POINTS];
    for (auto& i : children) {
        if (num_points <= MAX_POINTS - 8) {
            Object* obj = i.direct_ptr;
            if (obj) {
                // int index = 0;
                for (int j = 0; j < 8; ++j) {
                    points[num_points + j] = obj->GetTransform() * obj->box_.GetPoint(j);
                }
                num_points += 8;
            }
        }
    }

    // Get axis-aligned bounds of child bbox points
    vec3 min(FLT_MAX), max(-FLT_MAX);
    for (int i = 0; i < num_points; ++i) {
        for (int j = 0; j < 3; ++j) {
            min[j] = std::min(min[j], points[i][j]);
            max[j] = std::max(max[j], points[i][j]);
        }
    }

    // Set group shape to axis-aligned bbox
    SetRotation(quaternion());
    SetTranslation((min + max) * 0.5f);
    vec3 scale = max - min;
    // Inflate scale so nested groups are within each other instead of overlapping
    //     (if InitShape() is called in order from child to parent)
    // Also avoids scale[i] == 0.0 which can cause divide-by-zero elsewhere
    static const float SCALE_INFLATE = 0.01f;
    for (int i = 0; i < 3; ++i) {
        scale[i] += SCALE_INFLATE * ((scale[i] < 0.0f) ? -1.0f : 1.0f);
    }
    SetScale(scale);
}

void Group::PreDrawFrame(float curr_game_time) {
    if (g_debug_runtime_disable_group_pre_draw_frame) {
        return;
    }

    if (child_moved) {
        InitShape();
        InitRelMats();
        child_moved = false;
    }

    if (child_transforms_need_update) {
        PropagateTransformsDown(false);
    }

    for (auto& child : children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->PreDrawFrame(curr_game_time);
        }
    }
}

void Group::SetTranslationRotationFast(const vec3& trans, const quaternion& rotation) {
    Object::SetTranslationRotationFast(trans, rotation);
    PropagateTransformsDownFast(false);
}

void Group::PropagateTransformsDownFast(bool deep) {
    for (auto& child : children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->SetTranslationRotationFast(translation_ + (GetRotation() * (scale_ * child.rel_translation)), GetRotation() * child.rel_rotation);
            if (deep && obj->IsGroupDerived()) {
                Group* group = static_cast<Group*>(obj);
                group->PropagateTransformsDownFast(deep);
            }
        }
    }
}

void Group::PropagateTransformsDown(bool deep) {
    for (auto& child : children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->SetScale((invert(child.rel_rotation) * child.rel_scale) * scale_);
            obj->SetRotation(GetRotation() * child.rel_rotation);
            obj->SetTranslation(translation_ + (GetRotation() * (scale_ * child.rel_translation)));

            if (deep && obj->IsGroupDerived()) {
                Group* group = static_cast<Group*>(obj);
                group->PropagateTransformsDown(deep);
            }
        }
    }
    child_transforms_need_update = false;
    child_moved = false;
}

void Group::PreDrawCamera(float curr_game_time) {
    if (g_debug_runtime_disable_group_pre_draw_camera) {
        return;
    }

    for (auto& child : children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->PreDrawCamera(curr_game_time);
        }
    }
}

void Group::Moved(Object::MoveType type) {
    Object::Moved(type);
    child_transforms_need_update = true;
}

void Group::ChildMoved(Object::MoveType type) {
    child_moved = true;
}

void Group::UpdateParentHierarchy() {
    for (auto& i : children) {
        i.direct_ptr->UpdateParentHierarchy();
    }
}

void Group::GetDisplayName(char* buf, int buf_size) {
    if (GetName().empty()) {
        FormatString(buf, buf_size, "%d, Group with %d children", GetID(), children.size());
    } else {
        FormatString(buf, buf_size, "%s, Group with %d children", GetName().c_str(), children.size());
    }
}

void Group::SetEnabled(bool val) {
    enabled_ = val;
    for (auto& child : children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->SetEnabled(val);
        }
    }
}

void Group::HandleTransformationOccured() {
    for (auto& child : children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->HandleTransformationOccurred();
        }
    }
}

void Group::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    switch (type) {
        case OBJECT_MSG::SET_COLOR: {
            vec3 temp = *va_arg(args, vec3*);

            std::vector<Child>::iterator cit = children.begin();
            for (; cit != children.end(); cit++) {
                cit->direct_ptr->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &temp);
            }

            break;
        }
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void Group::RemapReferences(std::map<int, int> id_map) {
    for (auto& i : children) {
        i.direct_ptr->RemapReferences(id_map);
    }
}

ObjectSanityState Group::GetSanity() {
    uint32_t sanity_flags = 0;

    if (children.size() == 0) {
        sanity_flags |= kObjectSanity_G_Empty;
    }

    for (auto& i : children) {
        if (i.direct_ptr) {
        } else {
            sanity_flags |= kObjectSanity_G_NullChild;
        }
    }

    std::vector<Object*> all_children;
    GetTopDownCompleteChildren(&all_children);

    for (uint32_t i = 0; i < all_children.size(); i++) {
        Object* obj = all_children[i];
        if (obj) {
            if (obj->GetSanity().Ok() == false) {
                sanity_flags |= kObjectSanity_G_ChildHasSanityIssue;
            }

            std::vector<int> connections;
            obj->GetConnectionIDs(&connections);

            for (int connection : connections) {
                bool internally_connected = false;
                for (uint32_t t = 0; t < all_children.size(); t++) {
                    if (all_children[i] && all_children[t]->GetID() == connection) {
                        internally_connected = true;
                    }
                }

                if (internally_connected == false) {
                    // TODO: Specify what is externally connected
                    sanity_flags |= kObjectSanity_G_ChildHasExternalConnection;
                }
            }
        } else {
            sanity_flags |= kObjectSanity_G_NullChild;
        }
    }

    return ObjectSanityState(GetType(), GetID(), sanity_flags);
}
