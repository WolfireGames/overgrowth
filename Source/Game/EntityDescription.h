//-----------------------------------------------------------------------------
//           Name: EntityDescription.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <Asset/Asset/attachmentasset.h>
#include <Asset/assetinfobase.h>

#include <Editors/entity_type.h>
#include <Scripting/scriptparams.h>
#include <Graphics/palette.h>
#include <Game/attachment_type.h>
#include <AI/sample.h>

#include <vector>

class vec3;
class mat4;
class quaternion;

enum EntityDescriptionFieldType {
    EDF_ENTITY_TYPE,
    EDF_NAME,
    EDF_TRANSLATION,
    EDF_SCALE,
    EDF_ROTATION,
    EDF_VERSION,
    EDF_COLOR,
    EDF_OVERBRIGHT,
    EDF_ID,
    EDF_FILE_PATH,
    EDF_BILLBOARD_PATH,
    EDF_SPECIAL_TYPE,
    EDF_CONNECTIONS,
    EDF_ITEM_CONNECTIONS,
    EDF_DIRECTION,
    EDF_DECAL_MISS_LIST,
    EDF_DECAL_ISOLATION_ID,
    EDF_DISPLAY_MODE,
    EDF_DIMENSIONS,
    EDF_USING_IMPOSTER,
    EDF_IS_PLAYER,
    EDF_SCRIPT_PARAMS,
    EDF_PALETTE,
    EDF_SKY_ROTATION,
    EDF_SUN_RADIUS,
    EDF_SUN_COLOR_ANGLE,
    EDF_SUN_DIRECTION,
    EDF_ENV_OBJECT_ATTACH,
    EDF_NEGATIVE_LIGHT_PROBE,
    EDF_GI_COEFFICIENTS,
    EDF_NAV_MESH_CONNECTIONS,
    EDF_NO_NAVMESH,
    //Glimpse - Group no_navmesh.
    EDF_CHILDREN_NO_NAVMESH,    //Used for groups. 
    EDF_PREFAB_LOCKED,
    EDF_PREFAB_PATH,
    EDF_ORIGINAL_SCALE,
    EDF_ROTATION_EULER,
    EDF_HOTSPOT_CONNECTED_TO,    // This is never saved to disk, only used internally
    EDF_HOTSPOT_CONNECTED_FROM,  // This is never saved to disk, only used internally
    EDF_NPC_SCRIPT_PATH,
    EDF_PC_SCRIPT_PATH,
    // EDF_CONNECTIONS means connections to other objects, EDF_CONNECTIONS_FROM
    // means objects connected to this object (i.e. coming from another other)
    EDF_CONNECTIONS_FROM
};

struct ItemConnection {
    int id;
    AttachmentType attachment_type;
    AttachmentRef attachment_ref;
    bool mirrored;
};

struct ItemConnectionData {
    int32_t id;
    int32_t attachment_type;
    std::string attachment_str;
    int32_t mirrored;
};

struct NavMeshConnectionData {
    int32_t other_object_id;
    int32_t offmesh_connection_id;
    SamplePolyAreas poly_area;
};

struct EntityDescriptionField {
    int type;
    std::vector<char> data;
    void ReadBool(bool* val) const;
    void WriteBool(bool val);
    void ReadInt(int* id) const;
    void WriteInt(int id);
    void ReadIntVec(std::vector<int>* vec) const;
    void WriteIntVec(const std::vector<int>& val);
    void ReadItemConnectionDataVec(std::vector<ItemConnectionData>* item_connections) const;
    void WriteItemConnectionDataVec(const std::vector<ItemConnectionData>& val);
    void ReadNavMeshConnectionDataVec(std::vector<NavMeshConnectionData>* navmesh_connections) const;
    void WriteNavMeshConnectionDataVec(const std::vector<NavMeshConnectionData>& val);
    void ReadString(std::string* type_file) const;
    void WriteString(const std::string& str);
    void ReadPath(Path* path) const;
    void WritePath(const Path& val);
};

struct EntityDescription;
typedef std::vector<EntityDescription> EntityDescriptionList;

struct EntityDescription {
    EntityDescriptionList children;
    std::vector<EntityDescriptionField> fields;

    void AddPath(EntityDescriptionFieldType type, const Path& val);
    void AddVec3(EntityDescriptionFieldType type, const vec3& val);
    void AddInt(EntityDescriptionFieldType type, const int& val);
    void AddFloat(EntityDescriptionFieldType type, const float& val);
    void AddMat4(EntityDescriptionFieldType type, const mat4& val);
    void AddQuaternion(EntityDescriptionFieldType type, const quaternion& val);
    void AddEntityType(EntityDescriptionFieldType type, const EntityType& val);
    void AddIntVec(EntityDescriptionFieldType type, const std::vector<int>& val);
    void AddItemConnectionVec(EntityDescriptionFieldType type, const std::vector<ItemConnectionData>& val);
    void AddNavMeshConnnectionVec(EntityDescriptionFieldType type, const std::vector<NavMeshConnectionData>& val);
    void AddScriptParams(EntityDescriptionFieldType type, const ScriptParamMap& val);
    void AddPalette(EntityDescriptionFieldType type, const OGPalette& val);
    void AddString(EntityDescriptionFieldType type, const std::string& val);
    void AddData(EntityDescriptionFieldType type, const std::vector<char>& val);
    void AddBool(EntityDescriptionFieldType type, const bool& val);
    const EntityDescriptionField* GetField(EntityDescriptionFieldType type) const;
    EntityDescriptionField* GetField(EntityDescriptionFieldType type);
    EntityDescriptionField* GetEditableField(EntityDescriptionFieldType type);
    void ReturnPaths(PathSet& path_set);
    void SaveToXML(TiXmlElement* object) const;

   private:
    void AddType(EntityDescriptionFieldType type);
};

inline bool operator==(const EntityDescription& a, const EntityDescription& b) {
    return a.children == b.children && a.fields == b.fields;
}

inline bool operator!=(const EntityDescription& a, const EntityDescription& b) {
    return !(a == b);
}

inline bool operator==(const EntityDescriptionField& a, const EntityDescriptionField& b) {
    return a.type == b.type && a.data == b.data;
}

void GetTSRIinfo(EntityDescription& desc, const TiXmlElement* pElement);
void LoadEntityDescriptionFromXML(EntityDescription& ed, const TiXmlElement* el);
void LoadEntityDescriptionListFromXML(EntityDescriptionList& desc_list, const TiXmlElement* el);
void GetTSREinfo(const TiXmlElement* pElement, vec3& translation, vec3& scale, quaternion& rotation, vec3& rotation_euler);
void GetTSRinfo(const TiXmlElement* pElement, vec3& translation, vec3& scale, quaternion& rotation);
void ClearDescID(EntityDescription* desc);
