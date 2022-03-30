//-----------------------------------------------------------------------------
//           Name: placeholderobject.h
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

#include <Graphics/textureref.h>
#include <Objects/object.h>
#include <Asset/Asset/texture.h>

#include <string>
#include <vector>

class PlaceholderObject: public Object {
public:
    enum PlaceholderObjectType {
        kSpawn, kCamPreview, kPlayerConnect
    };
    PlaceholderObject();
    virtual ~PlaceholderObject();
    virtual EntityType GetType() const { return _placeholder_object; }
    PlaceholderObjectType GetSpecialType();
    void SetSpecialType(PlaceholderObjectType val);
    virtual bool Initialize();
    void SetPreview(const std::string& path);
    void SetBillboard(const std::string& path);
    virtual bool AcceptConnectionsFrom(ConnectionType type, Object& object);
    virtual void Draw();
    virtual void GetDesc(EntityDescription &desc) const;
    virtual bool SetFromDesc( const EntityDescription& desc );
    void SetVisible(bool visible);
    bool connectable();
    uint64_t GetConnectToTypeFilterFlags();
    void SetConnectToTypeFilterFlags(uint64_t entity_type_flags);
    virtual bool ConnectTo( Object& other, bool checking_other = false );
    virtual bool Disconnect( Object& other, bool checking_other = false );
    virtual void GetConnectionIDs(std::vector<int>* cons);

    int GetConnectID();
    virtual void NotifyDeleted( Object* o );
    virtual void Moved(Object::MoveType type);
    std::string editor_display_name;
    virtual void GetDisplayName(char* buf, int buf_size);

    virtual ObjectSanityState GetSanity();

    bool unsaved_changes;
private:
    TextureAssetRef billboard_texture_ref_;
    PlaceholderObjectType special_type_;
    uint64_t connect_to_type_filter_flags_;
    bool visible_;
    int connect_id_;
    std::string preview_path_;
    std::string billboard_path_;
    std::vector<Object*> preview_objects_;
    void DisposePreviewObjects();
    std::string prev_editor_label;
    int title_debug_string_id;
    bool label_position_update;
    vec3 editor_label_offset_prev;
    float editor_label_scale_prev;

    void DrawEditorLabel();
    void ClearEditorLabel();

    virtual void RemapReferences(std::map<int,int> id_map);
};
