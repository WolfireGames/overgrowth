//-----------------------------------------------------------------------------
//           Name: placeholderobject.cpp
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
#include "placeholderobject.h"

#include <Editors/actors_editor.h>
#include <Editors/map_editor.h>

#include <Graphics/textures.h>
#include <Graphics/Billboard.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/shaders.h>

#include <Internal/common.h>
#include <Internal/memwrite.h>
#include <Internal/profiler.h>

#include <Objects/envobject.h>
#include <Objects/prefab.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Math/vec3math.h>
#include <Game/EntityDescription.h>

extern bool shadow_cache_dirty;
extern bool g_debug_runtime_disable_placeholder_object_draw;

PlaceholderObject::PlaceholderObject() : special_type_(kSpawn),
                                         connect_to_type_filter_flags_(1ULL << EntityType::_movement_object),
                                         visible_(true),
                                         connect_id_(-1),
                                         title_debug_string_id(-1),
                                         editor_label_offset_prev(0.0f),
                                         editor_label_scale_prev(0.0f),
                                         unsaved_changes(false) {
    box_.dims = vec3(1.0f);
}

void PlaceholderObject::DisposePreviewObjects() {
    for (auto& preview_object : preview_objects_) {
        delete preview_object;
    }
    preview_objects_.clear();
}

void PlaceholderObject::SetPreview(const std::string& path) {
    if (preview_path_ == path) {
        return;
    }
    preview_path_ = path;
    DisposePreviewObjects();
    EntityDescriptionList desc_list;
    std::string file_type;
    Path source;
    ActorsEditor_LoadEntitiesFromFile(path, desc_list, &file_type, &source);
    preview_objects_.reserve(desc_list.size());
    for (auto& i : desc_list) {
        Object* obj = CreateObjectFromDesc(i);
        if (obj) {
            if (obj->GetType() == _env_object) {
                EnvObject* eo = (EnvObject*)obj;
                eo->placeholder_ = true;
                eo->added_to_physics_scene_ = true;
                // TODO: find another way to specify stipple
                // eo->shader_id_ = Shaders::Instance()->returnProgram(eo->ofr->shader_name + " #HALFTONE_STIPPLE");
            }
            if (scenegraph_) {
                obj->scenegraph_ = scenegraph_;
                obj->Initialize();
            }
            preview_objects_.push_back(obj);
        } else {
            LOGE << "Failed at loading content for Placeholder Preview object" << std::endl;
        }
    }
}

PlaceholderObject::~PlaceholderObject() {
    ClearEditorLabel();
    DisposePreviewObjects();
}

void PlaceholderObject::Draw() {
    if (g_debug_runtime_disable_placeholder_object_draw) {
        return;
    }

    if (!scenegraph_->map_editor->IsTypeEnabled(_placeholder_object) ||
        scenegraph_->map_editor->state_ == MapEditor::kInGame ||
        !visible_ ||
        Graphics::Instance()->media_mode() ||
        ActiveCameras::Instance()->Get()->GetFlags() == Camera::kPreviewCamera) {
        if (title_debug_string_id != -1) {
            DebugDraw::Instance()->SetVisible(title_debug_string_id, false);
        }
    } else {
        if (title_debug_string_id != -1) {
            DebugDraw::Instance()->SetVisible(title_debug_string_id, true);
        }

        bool old_shadow_cache_dirty = shadow_cache_dirty;
        for (auto& preview_object : preview_objects_) {
            preview_object->SetTransformationMatrix(GetTransform());
            preview_object->ReceiveObjectMessage(OBJECT_MSG::DRAW);
        }
        shadow_cache_dirty = old_shadow_cache_dirty;  // We don't want to update static shadow maps just because we moved this preview object

        if (ActiveCameras::Get()->GetFlags() == Camera::kEditorCamera) {
            if (billboard_texture_ref_.valid()) {
                DebugDraw::Instance()->AddBillboard(billboard_texture_ref_->GetTextureRef(), GetTranslation(), GetScale()[0], vec4(1.0f), kStraight, _delete_on_draw);
            }
            DrawEditorLabel();
        } else {
            ClearEditorLabel();
        }

        if (connect_id_ != -1) {
            Object* obj = scenegraph_->GetObjectFromID(connect_id_);
            if (obj) {
                DebugDraw::Instance()->AddLine(GetTranslation(), obj->GetTranslation(), vec4(1.0f, 1.0f, 1.0f, 0.5f), _delete_on_draw);
            }
        }
    }
}

void PlaceholderObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, preview_path_);
    if (billboard_texture_ref_.valid()) {
        desc.AddString(EDF_BILLBOARD_PATH, billboard_path_);
    }
    desc.AddInt(EDF_SPECIAL_TYPE, special_type_);
    std::vector<int> connections;
    connections.push_back(connect_id_);
    desc.AddIntVec(EDF_CONNECTIONS, connections);
}

bool PlaceholderObject::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);

    if (ret) {
        // Kludge to fill in the idName: (if the idName field is blank -- fill it in based on the object id
        if (sp.HasParam("LocName") && sp.IsParamString("LocName") && sp.GetStringVal("LocName") == "") {
            char locationChars[256];
            sprintf(locationChars, "location%d", GetID());
            std::string locationStr(locationChars);
            sp.ASRemove("LocName");
            sp.ASAddString("LocName", locationStr);
        }

        if (sp.HasParam("Name") && sp.IsParamString("Name") && sp.GetStringVal("Name") == "arena_spawn") {
            SetBillboard("Data/UI/spawner/thumbs/Utility/placeholder_arena_spawn.png");
        }

        if (sp.HasParam("Name") && sp.IsParamString("Name") && sp.GetStringVal("Name") == "arena_battle") {
            SetBillboard("Data/UI/spawner/thumbs/Utility/placeholder_arena_battle.png");
        }

        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_FILE_PATH: {
                    std::string path;
                    field.ReadString(&path);
                    SetPreview(path);
                    break;
                }
                case EDF_BILLBOARD_PATH: {
                    std::string path;
                    field.ReadString(&path);
                    SetBillboard(path);
                    break;
                }
                case EDF_SPECIAL_TYPE:
                    memread(&special_type_, sizeof(int), 1, field.data);
                    break;
                case EDF_CONNECTIONS: {
                    std::vector<int> connections;
                    field.ReadIntVec(&connections);
                    if (!connections.empty()) {
                        connect_id_ = connections[0];
                    }
                    break;
                }
            }
        }
    }

    return ret;
}

void PlaceholderObject::SetVisible(bool visible) {
    visible_ = visible;
}

PlaceholderObject::PlaceholderObjectType PlaceholderObject::GetSpecialType() {
    return special_type_;
}

void PlaceholderObject::SetSpecialType(PlaceholderObject::PlaceholderObjectType val) {
    special_type_ = val;
}

bool PlaceholderObject::Initialize() {
    for (auto obj : preview_objects_) {
        obj->scenegraph_ = scenegraph_;
        obj->Initialize();
    }
    obj_file = "placeholder_object";
    return true;
}

void PlaceholderObject::SetBillboard(const std::string& path) {
    Textures::Instance()->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    billboard_texture_ref_ = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(path, PX_SRGB, 0x0);
    billboard_path_ = path;
}

uint64_t PlaceholderObject::GetConnectToTypeFilterFlags() {
    return connect_to_type_filter_flags_;
}

void PlaceholderObject::SetConnectToTypeFilterFlags(uint64_t entity_type_flags) {
    connect_to_type_filter_flags_ = entity_type_flags;
}

bool PlaceholderObject::ConnectTo(Object& other, bool checking_other /*= false*/) {
    if (other.GetType() == _hotspot_object) {
        return Object::ConnectTo(other, checking_other);
    }
    if (!connectable()) {
        return false;
    }
    if (connect_to_type_filter_flags_ == (1ULL << EntityType::_any_type) ||
        (connect_to_type_filter_flags_ != 1ULL << EntityType::_no_type && (1ULL << other.GetType() & connect_to_type_filter_flags_) != 0)) {
        // If the target object is part of a prefab, we want to connect the placeholder to the prefab, to maintain the connection.
        Prefab* parent = other.GetPrefabParent();
        if (parent) {
            connect_id_ = parent->GetID();
        } else {
            connect_id_ = other.GetID();
        }
        return true;
    } else {
        return false;
    }
}

bool PlaceholderObject::AcceptConnectionsFrom(Object::ConnectionType type, Object& object) {
    return connectable() && type == kCTMovementObjects;  // Not sure this is correct because I have no clue how placeholders work yet
}

bool PlaceholderObject::Disconnect(Object& other, bool checking_other) {
    if (other.GetType() == _hotspot_object) {
        return Object::Disconnect(other, checking_other);
    }
    if (!connectable()) {
        return false;
    }
    if (connect_id_ == other.GetID()) {
        connect_id_ = -1;
        return true;
    } else {
        return false;
    }
}

void PlaceholderObject::GetConnectionIDs(std::vector<int>* cons) {
    if (connect_id_ != -1) {
        cons->push_back(connect_id_);
    }
}

bool PlaceholderObject::connectable() {
    return (special_type_ == kPlayerConnect);
}

int PlaceholderObject::GetConnectID() {
    return connect_id_;
}

void PlaceholderObject::NotifyDeleted(Object* o) {
    Object::NotifyDeleted(o);
    if (connect_id_ == o->GetID()) {
        connect_id_ = -1;
    }
}

void PlaceholderObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    label_position_update = true;
}

void PlaceholderObject::GetDisplayName(char* buf, int buf_size) {
    if (editor_display_name.length() > 0) {
        if (GetName().empty()) {
            FormatString(buf, buf_size, "%d, %s", GetID(), editor_display_name.c_str());
        } else {
            FormatString(buf, buf_size, "%s, %s", GetName().c_str(), editor_display_name.c_str());
        }
    } else {
        if (GetName().empty()) {
            FormatString(buf, buf_size, "%d: Empty Object", GetID());
        } else {
            FormatString(buf, buf_size, "%s: Empty Object", GetName().c_str());
        }
    }
}

ObjectSanityState PlaceholderObject::GetSanity() {
    uint32_t sanity_flags = 0;
    if (special_type_ == kPlayerConnect && connect_id_ == -1) {
        if (connect_id_ == -1) {
            sanity_flags |= kObjectSanity_PO_UnsetConnectID;
        }
    }
    return ObjectSanityState(GetType(), GetID(), sanity_flags);
}

void PlaceholderObject::DrawEditorLabel() {
    PROFILER_ZONE(g_profiler_ctx, "DrawEditorLabel");
    if (prev_editor_label != editor_label || editor_label_scale != editor_label_scale_prev) {
        ClearEditorLabel();
    }

    if (title_debug_string_id == -1 && editor_label.empty() == false) {
        title_debug_string_id = DebugDraw::Instance()->AddText(vec3(0.0f), editor_label, editor_label_scale, _persistent, _DD_NO_FLAG);
        prev_editor_label = editor_label;
        editor_label_scale_prev = editor_label_scale;
        label_position_update = true;
    }

    if ((label_position_update || editor_label_offset != editor_label_offset_prev) && title_debug_string_id != -1) {
        DebugDraw::Instance()->SetPosition(title_debug_string_id, GetTranslation() + editor_label_offset * GetScale()[1]);
        editor_label_offset_prev = editor_label_offset;
        label_position_update = false;
    }
}

void PlaceholderObject::ClearEditorLabel() {
    if (title_debug_string_id != -1) {
        DebugDraw::Instance()->Remove(title_debug_string_id);
        title_debug_string_id = -1;
    }
}

void PlaceholderObject::RemapReferences(std::map<int, int> id_map) {
    if (id_map.find(connect_id_) != id_map.end()) {
        connect_id_ = id_map[connect_id_];
    } else {
        connect_id_ = -1;
    }
}
