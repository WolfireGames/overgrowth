//-----------------------------------------------------------------------------
//           Name: EntityDescription.cpp
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
#include "EntityDescription.h"

#include <Asset/Asset/objectfile.h>
#include <Asset/Asset/decalfile.h>
#include <Asset/Asset/hotspotfile.h>
#include <Asset/Asset/actorfile.h>
#include <Asset/Asset/ambientsounds.h>
#include <Asset/Asset/item.h>

#include <Objects/envobjectattach.h>
#include <Objects/object.h>

#include <Internal/memwrite.h>
#include <Internal/error.h>

#include <Editors/editor_types.h>
#include <Editors/mem_read_entity_description.h>

#include <Main/engine.h>
#include <Compat/filepath.h>
#include <Game/color_tint_component.h>
#include <Math/mat4.h>
#include <Graphics/lightprobecollection.hpp>
#include <Logging/logdata.h>

#include <tinyxml.h>

#include <sstream>
#include <cmath>

void EntityDescription::AddPath(EntityDescriptionFieldType type, const Path& val) {
    AddType(type);
    fields.back().WritePath(val);
}

void EntityDescription::AddVec3( EntityDescriptionFieldType type, const vec3& val ){
    AddType(type);
    memwrite(val.entries, sizeof(float), 3, fields.back().data);
}

void EntityDescription::AddInt( EntityDescriptionFieldType type, const int& val ){
    AddType(type);
	fields.back().WriteInt(val);
}

void EntityDescription::AddMat4( EntityDescriptionFieldType type, const mat4& val ){
    AddType(type);
    memwrite(val.entries, sizeof(float), 16, fields.back().data);
}

void EntityDescription::AddQuaternion( EntityDescriptionFieldType type, const quaternion& val ){
    AddType(type);
    memwrite(val.entries, sizeof(float), 4, fields.back().data);
}

void EntityDescription::AddEntityType( EntityDescriptionFieldType type, const EntityType& val ){
    AddType(type);
    int the_type = val;
    memwrite(&the_type, sizeof(int), 1, fields.back().data);
}

void EntityDescription::AddIntVec( EntityDescriptionFieldType type, const std::vector<int>& val ){
    AddType(type);
	fields.back().WriteIntVec(val);
}

void EntityDescription::AddString( EntityDescriptionFieldType type, const std::string& val ){
    AddType(type);
    fields.back().WriteString(val);
}

const EntityDescriptionField* EntityDescription::GetField( EntityDescriptionFieldType type ) const {
    for(unsigned i=0; i<fields.size(); ++i){
        if(fields[i].type == type){
            return &fields[i];
        }
    }
    return NULL;
}

EntityDescriptionField* EntityDescription::GetField( EntityDescriptionFieldType type ) {
	for(unsigned i=0; i<fields.size(); ++i){
		if(fields[i].type == type){
			return &fields[i];
		}
	}
	return NULL;
}

EntityDescriptionField* EntityDescription::GetEditableField( EntityDescriptionFieldType type ){
    for(unsigned i=0; i<fields.size(); ++i){
        if(fields[i].type == type){
            return &fields[i];
        }
    }
    return NULL;
}

void EntityDescription::AddScriptParams( EntityDescriptionFieldType type, const ScriptParamMap& val ){
    AddType(type);
    WriteScriptParamsToRAM(val, fields.back().data);
}

void EntityDescription::AddPalette( EntityDescriptionFieldType type, const OGPalette& val ){
    AddType(type);
    WritePaletteToRAM(val, fields.back().data);
}

void EntityDescription::ReturnPaths( PathSet& path_set ){  
    std::string str;
    EntityType type = _no_type;
    for(unsigned i=0; i<fields.size(); ++i){
        EntityDescriptionField &field = fields[i];
        switch(field.type){
            case EDF_FILE_PATH:
                field.ReadString(&str);
                if(!str.empty()){
                    if(path_set.find("object "+str) != path_set.end()){
                        return;
                    }
                    path_set.insert("object "+str);
                }
                break;
            case EDF_ENTITY_TYPE:
                field.ReadInt((int*)&type);
                break;
        }
    }

    switch(type){
        case _env_object:
            //ObjectFiles::Instance()->ReturnRef(str)->ReturnPaths(path_set);
            Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(str)->ReturnPaths(path_set);
            break;
        case _decal_object:
            //DecalFiles::Instance()->ReturnRef(str)->ReturnPaths(path_set);
            Engine::Instance()->GetAssetManager()->LoadSync<DecalFile>(str)->ReturnPaths(path_set);
            break;
        case _hotspot_object:
            //HotspotFiles::Instance()->ReturnRef(str)->ReturnPaths(path_set);
            Engine::Instance()->GetAssetManager()->LoadSync<HotspotFile>(str)->ReturnPaths(path_set);
            break;
        case _ambient_sound_object:
            //AmbientSounds::Instance()->ReturnRef(str)->ReturnPaths(path_set);
            Engine::Instance()->GetAssetManager()->LoadSync<AmbientSound>(str)->ReturnPaths(path_set);
            break;
        case _movement_object:
            //ActorFiles::Instance()->ReturnRef(str)->ReturnPaths(path_set);
            Engine::Instance()->GetAssetManager()->LoadSync<ActorFile>(str)->ReturnPaths(path_set);
            break;
        case _item_object:
            //Items::Instance()->ReturnRef(str)->ReturnPaths(path_set);
            Engine::Instance()->GetAssetManager()->LoadSync<Item>(str)->ReturnPaths(path_set);
            break;
        case _group:{
            for(EntityDescriptionList::iterator iter = children.begin(); iter != children.end(); ++iter){
                EntityDescription& child = (*iter);
                child.ReturnPaths(path_set);
            }
            break;}
        case _prefab:{
            for(EntityDescriptionList::iterator iter = children.begin(); iter != children.end(); ++iter){
                EntityDescription& child = (*iter);
                child.ReturnPaths(path_set);
            }
            break;}
        case _camera_type:
        case _path_point_object:
        case _placeholder_object:
        case _light_probe_object:
        case _reflection_capture_object:
        case _light_volume_object:
		case _dynamic_light_object:
        case _navmesh_hint_object:
        case _navmesh_region_object:
        case _navmesh_connection_object:
            break;
        default:
            DisplayError("Error", "Unhandled type passed to EntityDescription::ReturnPaths");
            break;
    }
}

void EntityDescription::AddItemConnectionVec( EntityDescriptionFieldType type, const std::vector<ItemConnectionData>& val ) {
    AddType(type);
	fields.back().WriteItemConnectionDataVec(val);
}

void EntityDescription::AddNavMeshConnnectionVec(EntityDescriptionFieldType type, const std::vector<NavMeshConnectionData>& val) {
    AddType(type);
    fields.back().WriteNavMeshConnectionDataVec(val);
}

void EntityDescription::AddType( EntityDescriptionFieldType type ) {
    fields.resize(fields.size()+1);
    fields.back().type = type;
}

void EntityDescription::SaveToXML( TiXmlElement* parent ) const {
    TiXmlElement* object = new TiXmlElement("Unknown Type");
    parent->LinkEndChild(object);
    for(unsigned i=0; i<fields.size(); ++i){
        const EntityDescriptionField &field = fields[i];
        switch(field.type){
            case EDF_ID: {
                int id;
				field.ReadInt(&id);
                object->SetAttribute("id", id);
				break;}
			case EDF_VERSION: {
				int version;
				field.ReadInt(&version);
				object->SetAttribute("version", version);
				break;}
            case EDF_ENTITY_TYPE: {
				int type;
				field.ReadInt(&type);
                std::string type_str;
                switch(type){
                    case _hotspot_object:
                        type_str = "Hotspot";
                        break;
                    case _env_object:
                        type_str = "EnvObject";
                        break;
                    case _decal_object:
                        type_str = "Decal";
                        break;
                    case _movement_object:
                        type_str = "ActorObject";
                        break;
                    case _item_object:
                        type_str = "ItemObject";
                        break;
                    case _group:
                        type_str = "Group";
                        break;
                    case _ambient_sound_object:
                        type_str = "AmbientSoundObject";
                        break;
                    case _path_point_object:
                        type_str = "PathPointObject";
                        break;
                    case _placeholder_object:
                        type_str = "PlaceholderObject";
                        break;
                    case _light_probe_object:
                        type_str = "LightProbeObject";
                        break;
                    case _reflection_capture_object:
                        type_str = "ReflectionCaptureObject";
                        break;
					case _dynamic_light_object:
						type_str = "DynamicLightObject";
						break;
                    case _navmesh_hint_object:
                        type_str = "NavmeshHintObject";
                        break;
                    case _navmesh_connection_object:
                        type_str = "NavmeshConnectionObject";
                        break;
                    case _navmesh_region_object:
                        type_str = "NavmeshRegionObject";
                        break;
                    case _camera_type:
                        type_str = "CameraObject";
                        break;
                    case _light_volume_object:
                        type_str = "LightVolumeObject";
                        break;
                    case _prefab:
                        type_str = "Prefab";
                        break;
                    default:
                        FatalError("Error", "Unknown EntityType in EntityDescription");
                        break;
                }
                object->SetValue(type_str.c_str());
                break;}
            case EDF_NAME: {
                std::string name;
                field.ReadString(&name);
                if(name.empty() == false) {
                    object->SetAttribute("name",name.c_str());
                }
                break;}
            case EDF_FILE_PATH: {
                std::string type_file;
                field.ReadString(&type_file);
                object->SetAttribute("type_file", type_file.c_str());
                break;}
            case EDF_TRANSLATION: {
                vec3 translation;
                memread(translation.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("t0", translation[0]);
                object->SetDoubleAttribute("t1", translation[1]);
                object->SetDoubleAttribute("t2", translation[2]);
                break;}
            case EDF_SCALE: {
                vec3 scale;
                memread(scale.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("s0", scale[0]);
                object->SetDoubleAttribute("s1", scale[1]);
                object->SetDoubleAttribute("s2", scale[2]);
                break;}
            case EDF_ROTATION: {
                quaternion q;
                memread(q.entries, sizeof(float), 4, field.data);
                object->SetDoubleAttribute("q0", q.entries[0]);
                object->SetDoubleAttribute("q1", q.entries[1]);
                object->SetDoubleAttribute("q2", q.entries[2]);
                object->SetDoubleAttribute("q3", q.entries[3]);
                break;}
            case EDF_ROTATION_EULER: {
                vec3 rot;
                memread(rot.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("euler_x", rot.entries[0]);
                object->SetDoubleAttribute("euler_y", rot.entries[1]);
                object->SetDoubleAttribute("euler_z", rot.entries[2]);
                break;}
            case EDF_SCRIPT_PARAMS: {
                ScriptParamMap spm;
                ReadScriptParametersFromRAM(spm, field.data);
                TiXmlElement* params = new TiXmlElement("parameters");
                object->LinkEndChild(params);
                WriteScriptParamsToXML(spm, params);
                break;}
            case EDF_COLOR: {
                vec3 color;
                memread(color.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("color_r", color[0]);
                object->SetDoubleAttribute("color_g", color[1]);
                object->SetDoubleAttribute("color_b", color[2]);  
                break;}      
            case EDF_OVERBRIGHT: {
                float overbright;
                memread(&overbright, sizeof(float), 1, field.data);
                object->SetDoubleAttribute("overbright", overbright);
                break;}     
            case EDF_PALETTE: {
                OGPalette palette;
                ReadPaletteFromRAM(palette, field.data);
                TiXmlElement* palette_el = new TiXmlElement("Palette");
                object->LinkEndChild(palette_el);
                WritePaletteToXML(palette, palette_el);
                break;}
            case EDF_CONNECTIONS: {
                // Read connections from entity description
                std::vector<int> connections;
				field.ReadIntVec(&connections);
                // Write connections to XML
                TiXmlElement* connections_el = new TiXmlElement("Connections");
                object->LinkEndChild(connections_el);
                for(size_t i=0, len=connections.size(); i<len; ++i){
                    TiXmlElement* connection = new TiXmlElement("Connection");
                    connection->SetAttribute("id", connections[i]);
                    connections_el->LinkEndChild(connection);
                }
                break;}
            case EDF_CONNECTIONS_FROM: {
                // Read connections from entity description
                std::vector<int> connections;
				field.ReadIntVec(&connections);
                // Write connections to XML
                TiXmlElement* connections_el = new TiXmlElement("ConnectionsFrom");
                object->LinkEndChild(connections_el);
                for(size_t i=0, len=connections.size(); i<len; ++i){
                    TiXmlElement* connection = new TiXmlElement("Connection");
                    connection->SetAttribute("id", connections[i]);
                    connections_el->LinkEndChild(connection);
                }
                break;}
            case EDF_ITEM_CONNECTIONS: {
                std::vector<ItemConnectionData> item_connections;
				field.ReadItemConnectionDataVec(&item_connections);
                // Write item connections to XML
                TiXmlElement* item_connections_el = new TiXmlElement("ItemConnections");
                object->LinkEndChild(item_connections_el);
                for(size_t i=0, len=item_connections.size(); i<len; ++i) {
                    ItemConnectionData& item_connection = item_connections[i];
                    TiXmlElement* item_connection_el = new TiXmlElement("ItemConnection");
                    item_connection_el->SetAttribute("id", item_connection.id);
                    item_connection_el->SetAttribute("type", item_connection.attachment_type);
                    item_connection_el->SetAttribute("mirrored", item_connection.mirrored);
                    item_connection_el->SetAttribute("attachment", item_connection.attachment_str.c_str());
                    item_connections_el->LinkEndChild(item_connection_el);
                }
                break;}
            case EDF_IS_PLAYER: {
                int is_player;
                memread(&is_player, sizeof(int), 1, field.data);
                object->SetAttribute("is_player", is_player != 0);
                break;}
            case EDF_USING_IMPOSTER: {
                int using_imposter;
                memread(&using_imposter, sizeof(int), 1, field.data);
                object->SetAttribute("uses_imposter", using_imposter != 0);
                break;}
            case EDF_DIMENSIONS: {
                vec3 dims;
                memread(dims.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("d0", dims[0]);
                object->SetDoubleAttribute("d1", dims[1]);
                object->SetDoubleAttribute("d2", dims[2]);
                break;}
            case EDF_DIRECTION:{
                vec3 direction;
                memread(direction.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("p0", direction[0]);
                object->SetDoubleAttribute("p1", direction[1]);
                object->SetDoubleAttribute("p2", direction[2]);
                break;}
            case EDF_DECAL_ISOLATION_ID:{
                int isolation_id;
                field.ReadInt(&isolation_id);
                object->SetAttribute("i", isolation_id!=-1);
                if(isolation_id != -1) {
                    object->SetAttribute("io", isolation_id);
                }
                break;}
            case EDF_DISPLAY_MODE:{
                int display_mode;
                memread(&display_mode, sizeof(int), 1, field.data);
                object->SetAttribute("p_mode", display_mode);
                break;}
            case EDF_NEGATIVE_LIGHT_PROBE:{
                int val;
                memread(&val, sizeof(int), 1, field.data);
                object->SetAttribute("negative_light_probe", val);
                break;}
            case EDF_GI_COEFFICIENTS:{
                // There are a fixed amount of coefficients
                // Use attributes instead of child elements

                float data[kLightProbeNumCoeffs];
                memcpy(data, &field.data[0], sizeof(data));
                char elemName[] = "gicoeff00";
                for (unsigned int i = 0; i < kLightProbeNumCoeffs; i++) {
                    elemName[7] = '0' + (i / 10);
                    elemName[8] = '0' + (i % 10);
                    object->SetDoubleAttribute(elemName, data[i]);
                }
                break;}
            case EDF_SPECIAL_TYPE:{
                int special_type;
                memread(&special_type, sizeof(int), 1, field.data);
                object->SetAttribute("special_type", special_type);
                break;}
            case EDF_DECAL_MISS_LIST:{
                // Read miss list from entity description
                int miss_list_size;
                int index=0;
                memread(&miss_list_size, sizeof(int), 1, field.data, index);
                std::vector<int> miss_list;
                miss_list.resize(miss_list_size);
                if(miss_list_size){
                    memread(&miss_list[0], sizeof(int), miss_list_size, field.data, index);
                }
                // Write miss list to XML
                for(int i=0; i<miss_list_size; ++i){
                    std::ostringstream oss;
                    oss << "o" << i;
                    object->SetDoubleAttribute(oss.str().c_str(), miss_list[i]); 
                }
                break;}
            case EDF_ENV_OBJECT_ATTACH:{
                std::vector<AttachedEnvObject> aeo_vec;
                Deserialize(field.data, aeo_vec);
                TiXmlElement* env_object_attachments = new TiXmlElement("EnvObjectAttachments");
                object->LinkEndChild(env_object_attachments);
                for(unsigned i=0; i<aeo_vec.size(); ++i){
                    TiXmlElement* connection = new TiXmlElement("EnvObjectAttachment");
                    env_object_attachments->LinkEndChild(connection);
                    const AttachedEnvObject& aeo = aeo_vec[i];
                    for(unsigned j=0; j<kMaxBoneConnects; ++j){
                        TiXmlElement* bone_connect_el = new TiXmlElement("BoneConnect");
                        connection->LinkEndChild(bone_connect_el);
                        const BoneConnect &bone_connect = aeo.bone_connects[j];
                        bone_connect_el->SetAttribute("bone_id", bone_connect.bone_id);
                        bone_connect_el->SetAttribute("num_connections", bone_connect.num_connections);
                        int index;
                        char name[4];
                        for (int c = 0; c < 4; c++) {
                            for (int r = 0; r < 4; r++) {
                                name[0] = 'm';
                                index = r+c*4;
                                if(index<10){
                                    name[1] = '0'+index;
                                    name[2] = '\0';
                                } else {
                                    name[1] = '1';
                                    name[2] = '0'+index%10;
                                    name[3] = '\0';
                                }
                                bone_connect_el->SetDoubleAttribute(name, (double)bone_connect.rel_mat(r,c));
                            }
                        }
                    }
                }

                break;}
            case EDF_NAV_MESH_CONNECTIONS: {
                std::vector<NavMeshConnectionData> navmesh_connections;
                field.ReadNavMeshConnectionDataVec(&navmesh_connections);

                TiXmlElement* navmesh_connections_el = new TiXmlElement("NavMeshConnections");
                object->LinkEndChild(navmesh_connections_el);
                for(size_t i=0, len=navmesh_connections.size(); i<len; ++i) {
                    NavMeshConnectionData& navmesh_connection = navmesh_connections[i]; 
                    TiXmlElement* navmesh_connection_el = new TiXmlElement("NavMeshConnection");
                    navmesh_connection_el->SetAttribute("other_object_id", navmesh_connection.other_object_id);
                    navmesh_connection_el->SetAttribute("offmesh_connection_id", navmesh_connection.offmesh_connection_id);
                    navmesh_connection_el->SetAttribute("poly_area", navmesh_connection.poly_area);
                    navmesh_connections_el->LinkEndChild(navmesh_connection_el);
                }
                break;}
            case EDF_NO_NAVMESH: {
                bool val;
                field.ReadBool(&val);
                if(val) {
                    object->SetAttribute("no_navmesh","true");
                }
                break;}
            case EDF_PREFAB_LOCKED: {
                bool val;
                field.ReadBool(&val);
                object->SetAttribute("prefab_locked", val ? "true" : "false");
                break;}
            case EDF_PREFAB_PATH: {
                std::string val;
                field.ReadString(&val);
                object->SetAttribute("prefab_path", val.c_str());
                break;}
            case EDF_ORIGINAL_SCALE: {
                vec3 scale;
                memread(scale.entries, sizeof(float), 3, field.data);
                object->SetDoubleAttribute("os0", scale[0]);
                object->SetDoubleAttribute("os1", scale[1]);
                object->SetDoubleAttribute("os2", scale[2]);
                break;}
            case EDF_NPC_SCRIPT_PATH: {
                std::string path;
                field.ReadString(&path);
                object->SetAttribute("npc_script_path", path.c_str());
                break;}
            case EDF_PC_SCRIPT_PATH: {
                std::string path;
                field.ReadString(&path);
                object->SetAttribute("pc_script_path", path.c_str());
                break;}
        }
    }
    
    for(EntityDescriptionList::const_iterator iter = children.begin();
        iter != children.end(); ++iter)
    {
        const EntityDescription& child = (*iter);
        child.SaveToXML(object);
    }
}

void EntityDescription::AddFloat( EntityDescriptionFieldType type, const float& val ) {
    AddType(type);
    memwrite(&val, sizeof(float), 1, fields.back().data);
}

void EntityDescription::AddData(EntityDescriptionFieldType type, const std::vector<char>& val) {
    AddType(type);
    memwrite(&val[0], val.size(), 1, fields.back().data);    
}

void EntityDescription::AddBool(EntityDescriptionFieldType type, const bool& val) {
    AddType(type);
    char in = val ? 1 : 0;
    memwrite(&in, sizeof(char), 1, fields.back().data);    
}

void GetTSREinfo(const TiXmlElement* pElement, vec3 &translation, vec3 &scale, quaternion &rotation, vec3 &rotation_euler) {
    GetTSRinfo(pElement, translation, scale, rotation);

    rotation_euler = vec3();
    double dval;
    if(pElement->QueryDoubleAttribute("euler_x",&dval)==TIXML_SUCCESS) rotation_euler[0] = (float)dval;
    if(pElement->QueryDoubleAttribute("euler_y",&dval)==TIXML_SUCCESS) rotation_euler[1] = (float)dval;
    if(pElement->QueryDoubleAttribute("euler_z",&dval)==TIXML_SUCCESS) rotation_euler[2] = (float)dval;
}

void GetTSRinfo(const TiXmlElement* pElement, vec3 &translation, vec3 &scale, quaternion &rotation) {
    double dval;

    char name[4];
    for (int i = 0; i < 3; i++) {
        name[0] = 't';
        name[1] = '0'+i;
        name[2] = '\0';
        if (pElement->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS) {
            translation[i] = (float)dval;
        } else {
            translation[i] = 0;
        }
    }

    for (int i = 0; i < 3; i++) {
        name[0] = 's';
        name[1] = '0'+i;
        name[2] = '\0';
        if (pElement->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS) scale[i] = (float)dval;
        else scale[i] = 0;
    }

    int index;
    int quat_elements = 0;

    for (int i = 0; i < 4; i++) {
        name[0] = 'q';
        name[1] = '0' + i;
        name[2] = '\0';
        if (pElement->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS) {
            rotation.entries[i] = (float)dval;
            ++quat_elements;
        }
    }

    if (quat_elements == 3) {
        // reconstruct the last element
        float temp = 1.0f - (rotation.entries[0] * rotation.entries[0] + rotation.entries[1] * rotation.entries[1] + rotation.entries[2] * rotation.entries[2]);
        if (temp < 0.0f) {
            rotation.entries[3] = 0.0f;
            LOGE << "Invalid quaternion in level xml file" << std::endl;
        } else {
            rotation.entries[3] = sqrtf(temp);
        }
		rotation = QNormalize(rotation);
    } else if(quat_elements < 3) {
        // no quaternion, old format
        // reconstruct from matrix
        mat4 temp;
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                name[0] = 'r';
                index = r+c*4;
                if(index<10){
                    name[1] = '0'+index;
                    name[2] = '\0';
                } else {
                    name[1] = '1';
                    name[2] = '0'+index%10;
                    name[3] = '\0';
                }
                if (pElement->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS) temp(r,c) = (float)dval;
                else temp(r,c) = 0;
            }
        }
        rotation = QuaternionFromMat4(temp);
    }
}

void GetTSRIinfo( EntityDescription &desc, const TiXmlElement* pElement ){
    int ival;
    if (pElement->QueryIntAttribute("id",&ival)!=TIXML_SUCCESS) {
        ival = -1;
    }

    desc.AddInt(EDF_ID, ival);

    vec3 translation;
    vec3 scale;
    quaternion rotation;
    vec3 rotation_euler;

    GetTSREinfo(pElement, translation, scale, rotation, rotation_euler);

    desc.AddVec3(EDF_TRANSLATION, translation);
    desc.AddVec3(EDF_SCALE, scale);
    desc.AddQuaternion(EDF_ROTATION, rotation);
    desc.AddVec3(EDF_ROTATION_EULER, rotation_euler);

    ScriptParamMap spm;
    const TiXmlElement* params = pElement->FirstChildElement("parameters");
    if(params){
        ReadScriptParametersFromXML(spm, params);
    }
    desc.AddScriptParams(EDF_SCRIPT_PARAMS, spm);
}

void LoadEnvDescriptionFromXML( EntityDescription& desc, const TiXmlElement* el){
    desc.fields.reserve(6);
    desc.AddEntityType(EDF_ENTITY_TYPE, _env_object);
    const char* name = el->Attribute("name");
    if( name ) { 
        desc.AddString(EDF_NAME, name);
    }

    std::string type_file = SanitizePath(el->Attribute("type_file"));
    desc.AddString(EDF_FILE_PATH, type_file);

    const char* no_navmesh = el->Attribute("no_navmesh");
    if(no_navmesh) {
        desc.AddBool(EDF_NO_NAVMESH, saysTrue(no_navmesh) == 1);
    } else {
        desc.AddBool(EDF_NO_NAVMESH, false);
    }

    ColorTintComponent::LoadDescriptionFromXML(desc, el);

    GetTSRIinfo(desc, el);
}

void LoadDecalDescriptionFromXML( EntityDescription& desc, const TiXmlElement* el){
    desc.AddEntityType(EDF_ENTITY_TYPE, _decal_object);
    const char* ename = el->Attribute("name");
    if( ename ) { 
        desc.AddString(EDF_NAME, ename);
    }

    std::string type_file = SanitizePath(el->Attribute("type_file"));
    desc.AddString(EDF_FILE_PATH, type_file);

    vec3 projected_dir;
    double dval;
    char name[4];
    for (int i = 0; i < 3; i++) {
        name[0] = 'p';
        name[1] = '0'+i;
        name[2] = '\0';
        if (el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS) projected_dir[i] = (float)dval;
        else projected_dir[i] = 0;
    }
    desc.AddVec3(EDF_DIRECTION, projected_dir);

    ColorTintComponent::LoadDescriptionFromXML(desc, el);

    int display_mode = 0;
    int pmode;
    if (el->QueryIntAttribute("p_mode",&pmode)==TIXML_SUCCESS) {
        display_mode = pmode;
    }
    desc.AddInt(EDF_DISPLAY_MODE, display_mode);

    GetTSRIinfo(desc, el);

    // load object miss list
    std::vector<int> miss_list;
    int miss_list_val;
    std::string curr_num;
    while(1) {
        std::stringstream num;
        curr_num = num.str();
        if (el->QueryIntAttribute(("o" + curr_num).c_str(), &miss_list_val) == TIXML_SUCCESS) {
            miss_list.push_back(miss_list_val);
        }
        else break;
    }

    desc.AddIntVec(EDF_DECAL_MISS_LIST, miss_list);

    int isolation_id = -1;
    int isolation_state = 0;
    if (el->QueryIntAttribute("i", &isolation_state) == TIXML_SUCCESS) {
        if (isolation_state) {
            int read_isolation_id;
            if (el->QueryIntAttribute("io", &read_isolation_id) == TIXML_SUCCESS) {
                isolation_id = read_isolation_id;
            }
        }
    }
    desc.AddInt(EDF_DECAL_ISOLATION_ID, isolation_id);
}


void LoadGroupDescriptionFromXML( EntityDescription& desc, const TiXmlElement* el ) {
    desc.AddEntityType(EDF_ENTITY_TYPE, _group);
    const char* ename = el->Attribute("name");
    if( ename ) { 
        desc.AddString(EDF_NAME, ename);
    }
    GetTSRIinfo(desc, el);

    int ival;
    int using_imposter = 0;
    if (el->QueryIntAttribute("uses_imposter",&ival)==TIXML_SUCCESS) {
        using_imposter = ival!=0;
    }
    desc.AddInt(EDF_USING_IMPOSTER, using_imposter);

	if (el->QueryIntAttribute("version",&ival)==TIXML_SUCCESS) {
		desc.AddInt(EDF_VERSION, ival);
	}

    vec3 dims;
    char name[3];
    double dval;
    for (int i = 0; i < 3; i++) {
        name[0] = 'd';
        name[1] = '0'+i;
        name[2] = '\0';
        if (el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS){
            dims[i] = (float)dval;
        } else {
            dims[i] = 1;
        }
    }
    desc.AddVec3(EDF_DIMENSIONS, dims);

    vec3 color;
    for (int i = 0; i < 3; i++) {
        name[0] = 'c';
        name[1] = '0'+i;
        name[2] = '\0';
        if (el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS){
            color[i] = (float)dval;
        } else {
            color[i] = 0;
        }
    }
    desc.AddVec3(EDF_COLOR, color);
    
    LoadEntityDescriptionListFromXML(desc.children, el);
}

void LoadPrefabDescriptionFromXML( EntityDescription& desc, const TiXmlElement* el ) {
    desc.AddEntityType(EDF_ENTITY_TYPE, _prefab);
    desc.AddBool(EDF_PREFAB_LOCKED,SAYS_TRUE == saysTrue(el->Attribute("prefab_locked")));
    desc.AddString(EDF_PREFAB_PATH, nullAsEmpty(el->Attribute("prefab_path")));

    const char* ename = el->Attribute("name");
    if( ename ) { 
        desc.AddString(EDF_NAME, ename);
    }
    GetTSRIinfo(desc, el);

    double os0,os1,os2;
    bool success = (el->QueryDoubleAttribute("os0", &os0) == TIXML_SUCCESS)
                && (el->QueryDoubleAttribute("os1", &os1) == TIXML_SUCCESS)
                && (el->QueryDoubleAttribute("os2", &os2) == TIXML_SUCCESS);
    if( success ) {
        vec3 original_scale((float)os0, (float)os1, (float)os2);
        desc.AddVec3(EDF_ORIGINAL_SCALE,original_scale);
    }

    int ival;
    int using_imposter = 0;
    if (el->QueryIntAttribute("uses_imposter",&ival)==TIXML_SUCCESS) {
        using_imposter = ival!=0;
    }
    desc.AddInt(EDF_USING_IMPOSTER, using_imposter);

	if (el->QueryIntAttribute("version",&ival)==TIXML_SUCCESS) {
		desc.AddInt(EDF_VERSION, ival);
	}

    vec3 dims;
    char name[3];
    double dval;
    for (int i = 0; i < 3; i++) {
        name[0] = 'd';
        name[1] = '0'+i;
        name[2] = '\0';
        if (el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS){
            dims[i] = (float)dval;
        } else {
            dims[i] = 1;
        }
    }
    desc.AddVec3(EDF_DIMENSIONS, dims);

    vec3 color;
    for (int i = 0; i < 3; i++) {
        name[0] = 'c';
        name[1] = '0'+i;
        name[2] = '\0';
        if (el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS){
            color[i] = (float)dval;
        } else {
            color[i] = 0;
        }
    }
    desc.AddVec3(EDF_COLOR, color);

    LoadEntityDescriptionListFromXML(desc.children, el);
}

void LoadActorFromXML( EntityDescription& desc, const TiXmlElement* el, EntityType type) {
    const char* type_file_raw = el->Attribute("type_file");
    std::string type_file;
    if(type_file_raw && strlen(type_file_raw) > 0){
        type_file = SanitizePath(type_file_raw);
    }

    desc.AddEntityType(EDF_ENTITY_TYPE, type);
    const char* name = el->Attribute("name");
    if( name ) { 
        desc.AddString(EDF_NAME, name);
    }
    desc.AddString(EDF_FILE_PATH, type_file);

    if(type == _movement_object){
        std::string script_path;
        if(el->QueryStringAttribute("npc_script_path", &script_path) == TIXML_SUCCESS) {
            desc.AddString(EDF_NPC_SCRIPT_PATH, script_path);
        }
        if(el->QueryStringAttribute("pc_script_path", &script_path) == TIXML_SUCCESS) {
            desc.AddString(EDF_PC_SCRIPT_PATH, script_path);
        }

        int the_id = -1;
        const TiXmlElement* connections = el->FirstChildElement("Connections");
        if(connections){
            const TiXmlElement* connection = connections->FirstChildElement("Connection");
            int id;
            while(connection){
                if(connection->QueryIntAttribute("id", &id)==TIXML_SUCCESS){
                    the_id = id;
                }
                connection = connection->NextSiblingElement();
            }
        }
        std::vector<int> connection_vec;
        connection_vec.push_back(the_id);
        desc.AddIntVec(EDF_CONNECTIONS, connection_vec);
        int is_player = 0;
        el->QueryIntAttribute("is_player", &is_player);
        desc.AddInt(EDF_IS_PLAYER, is_player);

        std::vector<AttachedEnvObject> aeo_vec;
        const TiXmlElement* env_object_attachments = el->FirstChildElement("EnvObjectAttachments");
        if(env_object_attachments){
            const TiXmlElement* connection = env_object_attachments->FirstChildElement("EnvObjectAttachment");
            while(connection){
                AttachedEnvObject aeo;
                if(connection->QueryIntAttribute("obj_id", &aeo.legacy_obj_id) == TIXML_NO_ATTRIBUTE){
                    aeo.legacy_obj_id = -1;
                }
                const TiXmlElement* bone_connect_el = connection->FirstChildElement("BoneConnect");
                unsigned num = 0;
                while(bone_connect_el){
                    BoneConnect bone_connect;
                    bone_connect_el->QueryIntAttribute("bone_id", &bone_connect.bone_id);
                    bone_connect_el->QueryIntAttribute("num_connections", &bone_connect.num_connections);
                    int index;
                    double dval;
                    char name[4];
                    for (int c = 0; c < 4; c++) {
                        for (int r = 0; r < 4; r++) {
                            name[0] = 'm';
                            index = r+c*4;
                            if(index<10){
                                name[1] = '0'+index;
                                name[2] = '\0';
                            } else {
                                name[1] = '1';
                                name[2] = '0'+index%10;
                                name[3] = '\0';
                            }
                            if (bone_connect_el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS) bone_connect.rel_mat(r,c) = (float)dval;
                            else bone_connect.rel_mat(r,c) = 0;
                        }
                    }
                    if(num<kMaxBoneConnects){
                        aeo.bone_connects[num] = bone_connect;
                    }
                    ++num;
                    bone_connect_el = bone_connect_el->NextSiblingElement();
                }
                aeo_vec.push_back(aeo);
                connection = connection->NextSiblingElement();
            }
        }
        if(!aeo_vec.empty()){
            std::vector<char> aeo_data;
            Serialize(aeo_vec, aeo_data);
            desc.AddData(EDF_ENV_OBJECT_ATTACH, aeo_data);
        }

        const TiXmlElement* item_connections = el->FirstChildElement("ItemConnections");
        if(item_connections){
            std::vector<ItemConnectionData> item_connection_vec;
            const TiXmlElement* item_connection = item_connections->FirstChildElement("ItemConnection");
            while(item_connection){
                ItemConnectionData new_item_connection;
                item_connection->QueryIntAttribute("id", &new_item_connection.id);
                item_connection->QueryIntAttribute("type", &new_item_connection.attachment_type);
                item_connection->QueryIntAttribute("mirrored", &new_item_connection.mirrored);
                new_item_connection.attachment_str = item_connection->Attribute("attachment");
                item_connection_vec.push_back(new_item_connection);
                item_connection = item_connection->NextSiblingElement();
            }
            desc.AddItemConnectionVec(EDF_ITEM_CONNECTIONS, item_connection_vec);
        }

        OGPalette palette;
        const TiXmlElement* palette_el = el->FirstChildElement("Palette");
        if(palette_el){
            ReadPaletteFromXML(palette, palette_el);
        }
        desc.AddPalette(EDF_PALETTE, palette);
        LoadEntityDescriptionListFromXML(desc.children, el);
    
    } else if( type == _navmesh_connection_object ) {
        const TiXmlElement* navmesh_connections = el->FirstChildElement("NavMeshConnections");
        if(navmesh_connections){
            std::vector<NavMeshConnectionData> navmesh_connection_vec;
            const TiXmlElement* navmesh_connection = navmesh_connections->FirstChildElement("NavMeshConnection");
            while(navmesh_connection){
                NavMeshConnectionData new_navmesh_connection;
                navmesh_connection->QueryIntAttribute("other_object_id", &new_navmesh_connection.other_object_id);
                navmesh_connection->QueryIntAttribute("offmesh_connection_id", &new_navmesh_connection.offmesh_connection_id);
                int poly_area;
                navmesh_connection->QueryIntAttribute("poly_area", &poly_area);
                new_navmesh_connection.poly_area = (SamplePolyAreas)poly_area;
                navmesh_connection_vec.push_back( new_navmesh_connection );
                navmesh_connection = navmesh_connection->NextSiblingElement();
            }
            desc.AddNavMeshConnnectionVec(EDF_NAV_MESH_CONNECTIONS, navmesh_connection_vec);
        }
    } else if(type == _path_point_object  ){
        const TiXmlElement* connections = el->FirstChildElement("Connections");
        if(connections){
            const TiXmlElement* connection = connections->FirstChildElement("Connection");
            std::vector<int> ids;
            int id;
            while(connection){
                if(connection->QueryIntAttribute("id", &id)==TIXML_SUCCESS){
                    ids.push_back(id);
                }
                connection = connection->NextSiblingElement();
            }
            desc.AddIntVec(EDF_CONNECTIONS, ids);
        }
    } else if(type == _item_object){
        ColorTintComponent::LoadDescriptionFromXML(desc, el);
    } else if(type == _reflection_capture_object){
        const size_t data_size = kLightProbeNumCoeffs * sizeof(float);
        std::vector<char> data(data_size);
        char elemName[] = "gicoeff00";
        for (unsigned int i = 0; i < kLightProbeNumCoeffs; i++) {
            elemName[7] = '0' + (i / 10);
            elemName[8] = '0' + (i % 10);
            float fval = 0.0f;
            el->QueryFloatAttribute(elemName, &fval);
            memcpy(&data[i * sizeof(float)], &fval, sizeof(float));
        }
        desc.AddData(EDF_GI_COEFFICIENTS, data);
    } else if(type == _light_probe_object){
        int val = 0;
        el->QueryIntAttribute("negative_light_probe", &val);
        desc.AddInt(EDF_NEGATIVE_LIGHT_PROBE, val);
        if (!val) {
            const size_t data_size = kLightProbeNumCoeffs * sizeof(float);
            std::vector<char> data(data_size);
            char elemName[] = "gicoeff00";
            for (unsigned int i = 0; i < kLightProbeNumCoeffs; i++) {
                elemName[7] = '0' + (i / 10);
                elemName[8] = '0' + (i % 10);
                float fval = 0.0f;
                el->QueryFloatAttribute(elemName, &fval);
                memcpy(&data[i * sizeof(float)], &fval, sizeof(float));
            }

            desc.AddData(EDF_GI_COEFFICIENTS, data);
        }
    } else if(type == _dynamic_light_object) {
        vec3 color;
        char name[] = "color_x";
        const char rgb[] = "rgb";
        double dval = 0.0f;
        for (int i = 0; i < 3; i++) {
            name[6] = rgb[i];
            if (el->QueryDoubleAttribute(name,&dval)==TIXML_SUCCESS){
                color[i] = (float)dval;
            } else {
                color[i] = 0;
            }
        }
        desc.AddVec3(EDF_COLOR, color);
        double overbright = 1.0;
        el->QueryDoubleAttribute("overbright", &overbright);
        desc.AddFloat(EDF_OVERBRIGHT, (float) overbright);
    } else if(type == _placeholder_object){
        int the_id = -1;
        const TiXmlElement* connections = el->FirstChildElement("Connections");
        if(connections){
            const TiXmlElement* connection = connections->FirstChildElement("Connection");
            int id;
            while(connection){
                if(connection->QueryIntAttribute("id", &id)==TIXML_SUCCESS){
                    the_id = id;
                }
                connection = connection->NextSiblingElement();
            }
        }
        std::vector<int> connection_vec;
        connection_vec.push_back(the_id);
        desc.AddIntVec(EDF_CONNECTIONS, connection_vec);
        int special_type = 0;
        el->QueryIntAttribute("special_type",&special_type);
        desc.AddInt(EDF_SPECIAL_TYPE, special_type);
    }

    GetTSRIinfo(desc, el);
}

void LoadHotspotFromXML( EntityDescription& desc, const TiXmlElement* el ) {
    desc.AddEntityType(EDF_ENTITY_TYPE, _hotspot_object);
    const char* name = el->Attribute("name");
    if( name ) { 
        desc.AddString(EDF_NAME, name);
    }
    std::string type_file = SanitizePath(el->Attribute("type_file"));
    desc.AddString(EDF_FILE_PATH, type_file);
    GetTSRIinfo(desc, el);

    std::vector<int> connection_ids;
    const TiXmlElement* connections = el->FirstChildElement("Connections");
    if(connections) {
        const TiXmlElement* connection = connections->FirstChildElement("Connection");

        int id;
        while(connection) {
            if(connection->QueryIntAttribute("id", &id) == TIXML_SUCCESS) {
                connection_ids.push_back(id);
            }
            connection = connection->NextSiblingElement();
        }
    }
    desc.AddIntVec(EDF_CONNECTIONS, connection_ids);

    connection_ids.resize(0);
    connections = el->FirstChildElement("ConnectionsFrom");
    if(connections) {
        const TiXmlElement* connection = connections->FirstChildElement("Connection");

        int id;
        while(connection) {
            if(connection->QueryIntAttribute("id", &id) == TIXML_SUCCESS) {
                connection_ids.push_back(id);
            }
            connection = connection->NextSiblingElement();
        }
    }
    desc.AddIntVec(EDF_CONNECTIONS_FROM, connection_ids);
}

void LoadEntityDescriptionFromXML( EntityDescription& desc, const TiXmlElement* el ) {
    const char* type = el->Value();
    switch(type[0]){
        case 'A': 
            switch(type[1]){
                case 'c':
                    if(strcmp(type,"ActorObject") == 0){ LoadActorFromXML(desc, el, _movement_object); }
                    break;
                case 'm':
                    if(strcmp(type,"AmbientSoundObject") == 0){ LoadActorFromXML(desc, el, _ambient_sound_object); }
                    break;
            }
            break;
        case 'C': 
            if(strcmp(type,"CameraObject") == 0){ LoadActorFromXML(desc, el, _camera_type);  }
            break;
        case 'D':
			switch (type[1]){
				case 'e':
					if (strcmp(type, "Decal") == 0){ LoadDecalDescriptionFromXML(desc, el); }
					break;
				case 'y':
					if (strcmp(type, "DynamicLightObject") == 0){ LoadActorFromXML(desc, el, _dynamic_light_object); }
					break;
			}
			break;
        case 'E':
            if(strcmp(type,"EnvObject") == 0){ LoadEnvDescriptionFromXML(desc, el); }
            break;
        case 'G':
            if(strcmp(type,"Group") == 0){ LoadGroupDescriptionFromXML(desc, el); }
            break;
        case 'H':
            if(strcmp(type,"Hotspot") == 0){ LoadHotspotFromXML(desc, el); }
            break;
        case 'I':
            if(strcmp(type,"ItemObject") == 0){ LoadActorFromXML(desc, el, _item_object); }
            break;
        case 'L':
            switch (type[5]){
            case 'P':
                if(strcmp(type,"LightProbeObject") == 0){ LoadActorFromXML(desc, el, _light_probe_object); }
                break;
            case 'V':
                if(strcmp(type,"LightVolumeObject") == 0){ LoadActorFromXML(desc, el, _light_volume_object); }
                break;
            }
            break;
        case 'P': 
            switch(type[1]){
                case 'a':
                    if(strcmp(type,"PathPointObject") == 0){ LoadActorFromXML(desc, el, _path_point_object); }
                    break;
                case 'l':
                    if(strcmp(type,"PlaceholderObject") == 0){ LoadActorFromXML(desc, el, _placeholder_object); }
                    break;
                case 'r':
                    if(strcmp(type,"Prefab") == 0){ LoadPrefabDescriptionFromXML(desc, el); }
                    break;
            }
            break;
        case 'N':
            if(strcmp(type,"NavmeshHintObject") == 0 ){ LoadActorFromXML(desc, el, _navmesh_hint_object); }
            if(strcmp(type,"NavmeshConnectionObject") == 0 ){ LoadActorFromXML(desc, el, _navmesh_connection_object); }
            if(strcmp(type,"NavmeshRegionObject") == 0 ){ LoadActorFromXML(desc, el, _navmesh_region_object); }
            break;
        case 'R':
            if(strcmp(type,"ReflectionCaptureObject") == 0){ LoadActorFromXML(desc, el, _reflection_capture_object); }
            break;
    }
}

void LoadEntityDescriptionListFromXML( EntityDescriptionList& desc_list, const TiXmlElement* el ) {
    if(!el){
        FatalError("Error", "Cannot load entity description list from null pointer");
    }
    const TiXmlElement* child = el->FirstChildElement();
    while(child){
        const char* val = child->Value();
        if(strcmp(val, "parameters") != 0) {
            EntityDescription desc;
            LoadEntityDescriptionFromXML(desc, child);
            if(!desc.fields.empty()){
                desc_list.push_back(desc);
            }
        } 
        child = child->NextSiblingElement();   
    }
}

void ClearDescID(EntityDescription *desc) {
	EntityDescriptionField* edf = desc->GetEditableField(EDF_ID);
	if(edf){
		edf->data.clear();
		int null_id = -1;
		memwrite(&null_id, sizeof(int), 1, edf->data);
	}
	EntityDescriptionList &children = desc->children;
	for(EntityDescriptionList::iterator it = children.begin();
		it != children.end(); ++it)
	{
		ClearDescID(&(*it));
	}
}

void EntityDescriptionField::ReadInt(int* id) const {
	memread(id, sizeof(int), 1, data);
}

void EntityDescriptionField::WriteInt(int id) {
	memwrite(&id, sizeof(int), 1, data);
}

void EntityDescriptionField::ReadBool(bool* value) const {
    char id;
	memread(&id, sizeof(char), 1, data);
    *value = (id != 0);
}

void EntityDescriptionField::WriteBool(bool value) {
    char id = value ? 1 : 0;
	memwrite(&id, sizeof(char), 1, data);
}

void EntityDescriptionField::ReadIntVec(std::vector<int>* vec) const {
	int index=0;
	int size;
	memread(&size, sizeof(int), 1, data, index);
	vec->resize(size);
	if(size){
		memread(&vec->at(0), sizeof(int), size, data, index);
	}
}

void EntityDescriptionField::WriteIntVec(const std::vector<int>& val) {
	int val_size = (int)val.size();
	memwrite(&val_size, sizeof(int), 1, data);
	if(val_size > 0){
		memwrite(&val[0], sizeof(int), val_size, data);
	}
}

void EntityDescriptionField::ReadItemConnectionDataVec(std::vector<ItemConnectionData>* item_connections) const {
	int index=0;
	int num_connections;
	memread(&num_connections, sizeof(int), 1, data, index);
	if(num_connections){
		item_connections->resize(num_connections);
		for(int i=0; i<num_connections; ++i){
			MemReadItemConnectionData(item_connections->at(i), data, index);
		}
	}
}

void EntityDescriptionField::WriteItemConnectionDataVec(const std::vector<ItemConnectionData> &val) {
	int val_size = (int)val.size();
	memwrite(&val_size, sizeof(int), 1, data);
	for(int i=0; i<val_size; ++i){
		const ItemConnectionData &icd = val[i];
		MemWriteItemConnectionData(icd, data);
	}
}

void EntityDescriptionField::ReadNavMeshConnectionDataVec(std::vector<NavMeshConnectionData>* navmesh_connections) const
{
    int index=0;
    int32_t num_connections;
    memread(&num_connections, sizeof(int32_t), 1, data, index);
    if(num_connections){
        navmesh_connections->resize(num_connections);
        for(int i=0; i<num_connections; ++i){
            MemReadNavMeshConnectionData(navmesh_connections->at(i), data, index);
        }
    }
}

void EntityDescriptionField::WriteNavMeshConnectionDataVec(const std::vector<NavMeshConnectionData> &val)
{
    int32_t val_size = (int32_t)val.size();
    memwrite(&val_size, sizeof(int32_t), 1, data);
    for( int i=0; i<val_size; ++i)
    {
        const NavMeshConnectionData &nmcd = val[i];
        MemWriteNavMeshConnectionData(nmcd,data);
    }  
}

void EntityDescriptionField::ReadString(std::string* str) const {
    str->assign(data.begin(), data.end());
}

void EntityDescriptionField::WriteString(const std::string& str) {
    data.assign(str.begin(), str.end());
}

void EntityDescriptionField::ReadPath( Path* path ) const {
    std::string str;
    ReadString(&str);
    if( str.empty() ) {
        *path = Path();
    } else {
        *path = FindFilePath(str);
    }
}

void EntityDescriptionField::WritePath( const Path& val ) {
    WriteString(std::string(val.original));
}
