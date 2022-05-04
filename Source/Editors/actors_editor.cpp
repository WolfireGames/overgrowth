//-----------------------------------------------------------------------------
//           Name: actors_editor.cpp
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
#include "actors_editor.h"

#include <Objects/decalobject.h>
#include <Objects/movementobject.h>
#include <Objects/itemobject.h>
#include <Objects/pathpointobject.h>
#include <Objects/hotspot.h>
#include <Objects/group.h>
#include <Objects/prefab.h>
#include <Objects/placeholderobject.h>
#include <Objects/ambientsoundobject.h>
#include <Objects/lightprobeobject.h>
#include <Objects/lightvolume.h>
#include <Objects/dynamiclightobject.h>
#include <Objects/editorcameraobject.h>
#include <Objects/cameraobject.h>
#include <Objects/terrainobject.h>
#include <Objects/envobject.h>
#include <Objects/navmeshhintobject.h>
#include <Objects/navmeshregionobject.h>
#include <Objects/navmeshconnectionobject.h>
#include <Objects/reflectioncaptureobject.h>

#include <Graphics/models.h>
#include <Graphics/camera.h>
#include <Graphics/shaders.h>
#include <Graphics/textures.h>
#include <Graphics/palette.h>
#include <Graphics/pxdebugdraw.h>

#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/memwrite.h>
#include <Internal/profiler.h>
#include <Internal/filesystem.h>
#include <Internal/snprintf.h>
#include <Internal/config.h>

#include <XML/xml_helper.h>
#include <XML/level_loader.h>

#include <Online/online_datastructures.h>
#include <Online/online.h>

#include <UserInput/input.h>
#include <UserInput/keycommands.h>

#include <Math/vec4math.h>
#include <GUI/gui.h>
#include <Game/level.h>
#include <Logging/logdata.h>
#include <Game/EntityDescription.h>
#include <Editors/map_editor.h>

#include <tinyxml.h>

#include <algorithm>
#include <cstdarg>

extern bool shadow_cache_dirty;

void LoadGenericEntity( const Path& path, EntityDescriptionList &desc_list) {
    TiXmlDocument doc(path.GetFullPath());
    if (!doc.LoadFile()) {
        DisplayError("Error",("Bad xml format in " + path.GetFullPathStr()).c_str());
        return;
    }
    EntityType type = CheckGenericType(doc);
    EntityDescription desc;
    desc.AddEntityType(EDF_ENTITY_TYPE, type);
    desc.AddString(EDF_FILE_PATH, path.GetOriginalPath());    
    desc_list.push_back(desc);
}

void LoadGenericEntity(const std::string& rel_path, EntityDescriptionList &desc_list) {
	Path p = FindFilePath(rel_path.c_str(), kDataPaths | kModPaths | kAbsPath);
    if( p.isValid() == false ) {
        DisplayError("Error",("Invalid path " + p.GetFullPathStr()).c_str());
        return;
    }
    LoadGenericEntity(p,desc_list);
}

void LoadDescriptionsFromXMLSection(TiXmlNode* parent, const std::string& section, EntityDescriptionList &desc_list){
    TiXmlNode* pNode = parent->FirstChild(section.c_str());
    if(!pNode){
        return;
    }
    LoadEntityDescriptionListFromXML(desc_list, pNode->ToElement());
}

void LoadSavedPrefab(TiXmlNode* parent, EntityDescriptionList& desc_list) {
    LoadDescriptionsFromXMLSection(parent, "Prefab", desc_list);
}

void LoadSavedEntitiesDesc(TiXmlNode* parent, EntityDescriptionList& desc_list) {
    LoadDescriptionsFromXMLSection(parent, "ActorObjects", desc_list);
    LoadDescriptionsFromXMLSection(parent, "EnvObjects", desc_list);
    LoadDescriptionsFromXMLSection(parent, "Decals", desc_list);
    LoadDescriptionsFromXMLSection(parent, "Hotspots", desc_list);
    LoadDescriptionsFromXMLSection(parent, "Groups", desc_list);
}

bool ActorsEditor_AddEntity(SceneGraph* scenegraph, Object* o, std::map<int,int>* id_remap, bool group_addition, bool level_loading) {
    if(o->GetType() == _env_object && !((EnvObject*)o)->ofr->dynamic){
        shadow_cache_dirty = true;
    }
    std::vector<Object*> children;
    o->GetChildren(&children);
    for(auto & child : children) {
        int old_id = child->GetID();
        if( ActorsEditor_AddEntity(scenegraph, child, id_remap, group_addition, level_loading) == false ) {
            LOGE << "Failed adding child entity, removing from self" <<  std::endl;
            o->ChildLost(child);
            delete child;
            child = NULL;
        } else {
            if(old_id != child->GetID() && id_remap == NULL) {
                LOGW << "id_remap is NULL when adding entity with children" << std::endl;
                continue;
            }

            if(id_remap != NULL)
                id_remap->insert(std::pair<int,int>(old_id, child->GetID()));
        }
    }

    if(o->GetType() == _env_object){
        ((EnvObject*)o)->added_to_physics_scene_ = false;
    }

    if(!level_loading || o->GetID() == -1){
        scenegraph->AssignID(o);
    }

    if( scenegraph->addObject(o) ) { 
        if(!level_loading){
            static const int kBufSize = 256;
            char msg[kBufSize];
            FormatString(msg, kBufSize, "added_object %d", o->GetID());
            scenegraph->level->Message(msg);
        }
        return true;
    } else {
        LOGE << "Failed at adding entity" << std::endl;
        for(auto & child : children) {
            if( child ) {
                child->SetParent(NULL);
            }
        }
        return false;
    }

}

static void GetFlattenedDescList(EntityDescriptionList *desc_list, std::vector<EntityDescription*> *flat_desc_list){
	for(auto & desc_index : *desc_list) 
	{
		EntityDescription *desc = &desc_index;
		flat_desc_list->push_back(desc);
		GetFlattenedDescList(&desc->children, flat_desc_list);
	}
}

void LocalizeIDs(EntityDescriptionList *desc_list, bool keep_external_connections){
	std::vector<EntityDescription*> flat_desc_list;
	GetFlattenedDescList(desc_list, &flat_desc_list);
    std::map<int,int> scene_ids;
    typedef std::map<int,int>::iterator Iter;
	for(auto desc : flat_desc_list) 
	{
			EntityDescriptionField *edf = desc->GetField(EDF_ID);
		if(edf){
            int id;
			memread(&id, sizeof(int), 1, edf->data);
            int new_id = (int) scene_ids.size();

            scene_ids.insert(std::pair<int,int>(id, new_id));

            edf->data.clear();
            edf->WriteInt(new_id);
            //scene_ids.push_back(id);
		} else {
            //scene_ids.push_back(-1);
            LOGW << "Entity has no ID when trying to localize" << std::endl;
		}
	}
	// TODO: do this in O(n) instead of O(n^2)
	for(auto desc : flat_desc_list) 
	{
			EntityDescriptionField *edf = desc->GetField(EDF_CONNECTIONS);
		if(edf) {
			std::vector<int> connections;
			std::vector<int> write_connections;
			edf->ReadIntVec(&connections);
			for(int id : connections) 
			{
                Iter iter = scene_ids.find(id);
                if(iter != scene_ids.end()) {
                    id = iter->second;
                    write_connections.push_back(id);
                }
                else if(keep_external_connections)
                    write_connections.push_back(id);
			}
			edf->data.clear();
			edf->WriteIntVec(write_connections);
		}
		edf = desc->GetField(EDF_ITEM_CONNECTIONS);
		if(edf) {
			std::vector<ItemConnectionData> connections;
			std::vector<ItemConnectionData> write_connections;
			edf->ReadItemConnectionDataVec(&connections);
			for(auto item_connection_data : connections) 
			{
                Iter iter = scene_ids.find(item_connection_data.id);
                if(iter != scene_ids.end()) {
                    item_connection_data.id = iter->second;
                    write_connections.push_back(item_connection_data);
                }
                else if(keep_external_connections)
                    write_connections.push_back(item_connection_data);
			}
			edf->data.clear();
			edf->WriteItemConnectionDataVec(write_connections);
        }
        edf = desc->GetField(EDF_NAV_MESH_CONNECTIONS);
        if(edf) {
			std::vector<NavMeshConnectionData> connections;
			std::vector<NavMeshConnectionData> write_connections;
			edf->ReadNavMeshConnectionDataVec(&connections);
			for(auto nav_mesh_connection_data : connections) 
			{
                Iter iter = scene_ids.find(nav_mesh_connection_data.other_object_id);
                if(iter != scene_ids.end()) {
                    nav_mesh_connection_data.other_object_id = iter->second;
                    write_connections.push_back(nav_mesh_connection_data);
                }
                else if(keep_external_connections)
                    write_connections.push_back(nav_mesh_connection_data);
			}
			edf->data.clear();
			edf->WriteNavMeshConnectionDataVec(write_connections);
        }
        edf = desc->GetField(EDF_CONNECTIONS_FROM);
        if(edf) {
			std::vector<int> connections;
			std::vector<int> write_connections;
			edf->ReadIntVec(&connections);
			for(int id : connections)
			{
                Iter iter = scene_ids.find(id);
                if(iter != scene_ids.end()) {
                    id = iter->second;
                    write_connections.push_back(id);
                }
                else if(keep_external_connections) {
                    write_connections.push_back(id);
                }
			}
			edf->data.clear();
			edf->WriteIntVec(write_connections);
        }
	}
}

void ActorsEditor_CopySelectedEntities(SceneGraph* scenegraph, EntityDescriptionList *desc_list) {
    desc_list->clear();
    // Add non-grouped objects to copy list
    for (auto obj : scenegraph->objects_) {    
        if (obj->Selected() && obj->permission_flags&Object::CAN_COPY) {
            desc_list->resize(desc_list->size()+1);
            obj->GetDesc(desc_list->back());
        }
	}    
}

EntityType GetTypeFromDesc( const EntityDescription& desc ) {
	EntityType type;
	for (const auto & field : desc.fields){
			if (field.type == EDF_ENTITY_TYPE){
			memread(&type, sizeof(int), 1, field.data);
			break;
		}
	}
	return type;
}

Object * CreateObjectFromDesc( const EntityDescription& desc ) {
    PROFILER_ZONE(g_profiler_ctx, "CreateObjectFromDesc");
	EntityType type = GetTypeFromDesc(desc);

    Object* obj = NULL;
    switch(type){
        case _movement_object:
            obj = new MovementObject();
            break;
        case _camera_type:
            obj = new EditorCameraObject();
            ((CameraObject*)obj)->controlled = true;
            ActiveCameras::Get()->SetCameraObject((CameraObject*)obj);
            ActiveCameras::Get()->SetFlags(Camera::kEditorCamera);
            break;
        case _item_object:
            obj = new ItemObject();
            break;
        case _ambient_sound_object:
            obj = new AmbientSoundObject();
            break;
        case _path_point_object:
            obj = new PathPointObject();
            break;
        case _env_object:
            obj = new EnvObject();
            break;
        case _decal_object:
            obj = new DecalObject();
            break;
        case _hotspot_object:
            obj = new Hotspot();
            break;
        case _group:
            obj = new Group();
            break;
        case _prefab:
            obj = new Prefab();
            break;
        case _placeholder_object:
            obj = new PlaceholderObject();
            break;
        case _light_probe_object:
            obj = new LightProbeObject();
            break;
        case _reflection_capture_object:
            obj = new ReflectionCaptureObject();
            break;
        case _light_volume_object:
            obj = new LightVolumeObject();
            break;
		case _dynamic_light_object:
			obj = new DynamicLightObject();
			break;
        case _navmesh_hint_object:
            obj  = new NavmeshHintObject();
            break;
        case _navmesh_connection_object:
            obj  = new NavmeshConnectionObject();
            break;
        case _navmesh_region_object:
            obj = new NavmeshRegionObject();
            break;
        case _terrain_type:
        case _spawn_point:
            break;
        default: 
            FatalError("Error", "ActorsEditor::CreateObjectFromDesc does not support this object type");
            break;
    }

    if( obj ) {
        if( false == obj->SetFromDesc(desc) ) {
            delete obj;
            obj = NULL;
        }
    }

    return obj;
}

std::vector<Object*> ActorsEditor_AddEntitiesAtMouse(const Path& source, SceneGraph *scenegraph, const EntityDescriptionList& desc_list, LineSegment mouseray, bool in_new_prefab) {
    if (desc_list.empty())
        return std::vector<Object*>();

    Collision c = scenegraph->lineCheckCollidable(mouseray.start, mouseray.end, NULL);

    vec3 spawn_pos = mouseray.start + normalize(mouseray.end - mouseray.start) * 10.0f;
    if (c.hit && c.hit_what != NULL) {
        spawn_pos = c.hit_where;
    }

    return ActorsEditor_AddEntitiesAtPosition(source, scenegraph, desc_list, spawn_pos, in_new_prefab);
}

std::vector<Object*> ActorsEditor_AddEntitiesAtPosition(const Path& source, SceneGraph *scenegraph, const EntityDescriptionList& desc_list, vec3 spawn_pos, bool in_new_prefab, CommonObjectID hostid) {

	Online* online = Online::Instance();
	if (online->IsClient()) {
		if (hostid == 0) {
			std::string path;

			// This is backwards, but when we paste, source will be "", which means that we rely on desc_list for the path
			// we therefor have to figure out the path, and deserialize it like this.
			bool found = false;
			for (uint32_t i = 0; i < desc_list.size() && !found; i++) {
				for (uint32_t j = 0; j < desc_list[i].fields.size() && !found; j++) {
					if (desc_list[i].fields[j].type == EDF_FILE_PATH) { 
						desc_list[i].fields[j].ReadString(&path); 
						found = true;
					}
				}
			}

			online->Send<OnlineMessages::CreateEntity>(path, spawn_pos, 0);
			return std::vector<Object*>();
		}

	}

    if (desc_list.empty())
        return std::vector<Object*>();
    
    // Get bounding box of the centers of all entities
    bool center_set = false;
    vec3 desc_min, desc_max;
    vec3 vec;
    for(unsigned i=0; i<desc_list.size(); ++i){
        const EntityDescriptionField* edf = desc_list[0].GetField(EDF_TRANSLATION);
        if(edf){
            memread(vec.entries, sizeof(float), 3, edf->data);
            if(!center_set){
                desc_min = vec;
                desc_max = vec;
                center_set = true;
            } else {
                for(unsigned j=0; j<3; ++j){
                    desc_min[j] = min(desc_min[j], vec[j]);
                    desc_max[j] = max(desc_max[j], vec[j]);
                }
            }
        }
    }
    
    // Get midpoint of centers bounding box
    vec3 copied_center = (desc_min + desc_max) * 0.5f;
       
    std::map<int,int> id_remap;

	std::vector<Object*> new_objects;
    for (const auto & i : desc_list) {
		if (GetTypeFromDesc(i) == _decal_object) {
			// Check we are not exceeding max decal limit
			if ((scenegraph->decal_objects_.size() - scenegraph->dynamic_decals.size()) >= scenegraph->kMaxStaticDecals) {
				DisplayError("Warning", "Static decal limit exceeded, cannot add new one!");
				return new_objects;
			}
		}
        Object* new_entity = CreateObjectFromDesc(i);
        if( new_entity ) {
            vec3 offset = (new_entity->GetTranslation() - copied_center);
            vec3 pos = spawn_pos;
            pos += offset;

            if( new_entity->IsGroupDerived() ) {
                Group* group = static_cast<Group*>(new_entity);
                //generate the relative positioning data before moving group object.
                group->InitRelMats();
            } 

            const vec4 &dims = new_entity->box_.dims;
            //The following compensation will instead occur on the full prefab, it's to offset up from the
            //collision point.
            if( in_new_prefab == false ) {
                LOGI << "New entity box: " << dims << std::endl;
                if (new_entity->GetType() == _movement_object) {
                    pos += vec3(0.0f,1.0f,0.0f);
                }
                if (new_entity->GetType() == _item_object) {
                    pos += vec3(0.0f,dims[1]*0.5f+0.1f,0.0f);    
                }
                if (new_entity->GetType() == _env_object) {
                    pos[1] += dims[1]*((EnvObject*)new_entity)->ofr->ground_offset;
                }    
            }
            new_entity->SetTranslation(pos);
            if( new_entity->IsGroupDerived() ) {
                Group* group = static_cast<Group*>(new_entity);
                group->PropagateTransformsDown(true);
            }

            int old_id = new_entity->GetID();
            if( ActorsEditor_AddEntity(scenegraph, new_entity, &id_remap, false) ) {    // note: this needs to be after placement, since some AddEntity functions may assume entity is already in place
                new_objects.push_back(new_entity);
                id_remap[old_id] = new_entity->GetID();



				// Send of a package if host and in an active mp session
				if (online->IsActive()) {
					new_entity->created_on_the_fly = true;
					if (online->IsHosting()) { 
						online->Send<OnlineMessages::CreateEntity>(new_entity->obj_file, spawn_pos, new_entity->GetID());
					} else {
						// this is hacky, checking if hostid is 0, but there is no other option atm.
						// if it's zero, then this function was called with no id supplied, meaning there is no corresponding object
						// on host, which means we are problably looking at a decal or something not gameplay related.
						if (hostid != 0) { 
							online->RegisterHostObjectID(hostid, new_entity->GetID());
						} 
					}
				}
            } else {
                LOGE << "Failed at adding constructed entity" << std::endl;
                delete new_entity;
            }
        } else {
            LOGE << "Failed at constructing requested entity for spawn." << std::endl;
        }
	}

	for (auto & new_object : new_objects) {
        new_object->RemapReferences(id_remap);
    }

	for (auto & new_object : new_objects) {
		new_object->ReceiveObjectMessage(OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS);
	}

    if( in_new_prefab ) {
        Prefab *prefab = new Prefab();
        if( ActorsEditor_AddEntity(scenegraph, prefab) ) {
            prefab->children.resize(new_objects.size());
            prefab->prefab_path = source;
            for(size_t i=0, len=new_objects.size(); i<len; ++i){
                prefab->children[i].direct_ptr = new_objects[i];
            }
            for(size_t i=0, len=new_objects.size(); i<len; ++i){
                prefab->children[i].direct_ptr->SetParent(prefab);
            }
            prefab->InitShape();
            prefab->InitRelMats();
        
            vec3 pos = prefab->GetTranslation();
            pos += vec3(0.0f,prefab->box_.dims[1]*0.5f+0.1f,0.0f);
            prefab->SetTranslation(pos);

            prefab->PropagateTransformsDown(true);
        } else {
            LOGE << "Failed at adding a prefab to contain spawned group" << std::endl;
            delete prefab;
        }
        new_objects.clear();
        new_objects.push_back(prefab);
    }
    scenegraph->UpdatePhysicsTransforms();

	 
    return new_objects;
}

void ActorsEditor_AddEntitiesIntoPrefab(Object*  obj,SceneGraph *scenegraph, const EntityDescriptionList& desc_list) {
    if (desc_list.empty()) return;

    Prefab *prefab = static_cast<Prefab*>(obj);
    
    // Get bounding box of the centers of all entities
    bool center_set = false;
    vec3 desc_min, desc_max;
    vec3 vec;
    for(unsigned i=0; i<desc_list.size(); ++i){
        const EntityDescriptionField* edf = desc_list[0].GetField(EDF_TRANSLATION);
        if(edf){
            memread(vec.entries, sizeof(float), 3, edf->data);
            if(!center_set){
                desc_min = vec;
                desc_max = vec;
                center_set = true;
            } else {
                for(unsigned j=0; j<3; ++j){
                    desc_min[j] = min(desc_min[j], vec[j]);
                    desc_max[j] = max(desc_max[j], vec[j]);
                }
            }
        }
    }
    
    // Get midpoint of centers bounding box
    vec3 copied_center = (desc_min + desc_max) * 0.5f;

    std::map<int,int> id_remap;

	std::vector<Object*> new_objects;
    for (const auto & desc : desc_list) {
		if (GetTypeFromDesc(desc) == _decal_object) {
			// Check we are not exceeding max decal limit
			if ((scenegraph->decal_objects_.size() - scenegraph->dynamic_decals.size()) >= scenegraph->kMaxStaticDecals) {
				DisplayError("Warning", "Static decal limit exceeded, cannot add new one!");
				return;
			}
		}
        Object* new_entity = CreateObjectFromDesc(desc);

        if( new_entity ) {
            //Position offset from calculated relative origo.
            vec3 pos = (new_entity->GetTranslation() - copied_center);

            if( new_entity->IsGroupDerived() ) {
                Group* group = static_cast<Group*>(new_entity);
                //generate the relative positioning data before moving group object.
                group->InitRelMats();
            } 

            if( new_entity->IsGroupDerived() ) {
                Group* group = static_cast<Group*>(new_entity);
                group->PropagateTransformsDown(true);
            }

            int old_id = new_entity->GetID();
            if( ActorsEditor_AddEntity(scenegraph, new_entity, &id_remap, false) ) {    // note: this needs to be after placement, since some AddEntity functions may assume entity is already in place
                //Set translation again in case intialization moved the object for some reason, this appears to happen for item_object
                new_objects.push_back(new_entity);
                id_remap[old_id] = new_entity->GetID();
                new_entity->SetTranslation(pos);
            } else {
                LOGE << "Failed at adding new entity to scenegraph" << std::endl;
                delete new_entity;
            }
        } else {
            LOGE << "Failed and constructing entity for spawn" << std::endl;
        }
	}

	for (auto & new_object : new_objects) {
        new_object->RemapReferences(id_remap);
    }

	for (auto & new_object : new_objects) {
		new_object->ReceiveObjectMessage(OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS);
	}

    vec3 orig_trans =           prefab->GetTranslation();
    quaternion orig_rotation =  prefab->GetRotation();
    vec3 orig_scale_factor = vec3(1.0f);
    if( prefab->original_scale_ != vec3(0.0f) ) {
         orig_scale_factor = prefab->GetScale()/prefab->original_scale_;
    }

    prefab->SetTranslation(vec3(0.0f));
    prefab->SetRotation(quaternion());  
    prefab->SetScale(vec3(1.0f));

    for(auto & new_object : new_objects){
        Group::Child child;
        child.direct_ptr = new_object;
        child.direct_ptr->SetParent(prefab);
        prefab->children.push_back(child);
    }

    prefab->InitShape();

    prefab->InitRelMats();

    prefab->SetTranslation(orig_trans);
    prefab->SetRotation(orig_rotation);  
    prefab->SetScale(prefab->GetScale()*orig_scale_factor);

    prefab->PropagateTransformsDown(true);

    scenegraph->UpdatePhysicsTransforms();
}

int ActorsEditor_LoadEntitiesFromFile(const std::string& rel_path, EntityDescriptionList &desc_list, std::string* file_type, Path* res_path) {
    if(rel_path.empty()){
        return -1;
    }
    *res_path = FindFilePath(rel_path.c_str(), kModPaths | kDataPaths | kAbsPath);

    return ActorsEditor_LoadEntitiesFromFile(*res_path,desc_list,file_type);
}

int ActorsEditor_LoadEntitiesFromFile(const Path& path, EntityDescriptionList &desc_list, std::string* file_type)
{
	if(path.isValid() == false){
        DisplayError("Error",("Could not find file " + path.GetFullPathStr()).c_str());
        return -1;
    }

    TiXmlDocument doc(path.GetFullPath());
    if (!doc.LoadFile()) {
        DisplayError("Error",("Bad xml format in " + path.GetFullPathStr()).c_str());
        return -1;
    }

    TiXmlHandle hDoc(&doc);
    TiXmlDeclaration* decl = ((hDoc.FirstChild()).ToNode())->ToDeclaration();
    if (strcmp(decl->Version(),"2.0") == 0) {
        std::string type;
        if (XmlHelper::getNodeValue(doc,"Type",type)) {
            *file_type = std::string(type);
            if (type == "generic") {
                LoadGenericEntity(path, desc_list);
            } else if (type == "saved") {
                LoadSavedEntitiesDesc(&doc, desc_list);
            } else if (type == "prefab") {
                LoadSavedPrefab(&doc, desc_list);
            } else {
                DisplayError("Error",("Unhandled specification type in " + path.GetFullPathStr()).c_str(), _ok);
            }
        } else {
            DisplayError("Error",("Bad XML in " + path.GetFullPathStr()).c_str());
        }
    } else if (strcmp(decl->Version(),"1.0") == 0) {
        LoadGenericEntity(path,desc_list);
    } else {
        DisplayError("Error","Unhandled OG XML version");
    }
    return 0;

}

void ActorsEditor_UnlocalizeIDs(EntityDescriptionList *desc_list, SceneGraph *scenegraph){
	std::vector<EntityDescription*> flat_desc_list;
	GetFlattenedDescList(desc_list, &flat_desc_list);
    std::map<int, int> scene_ids;
    typedef std::map<int, int>::iterator Iter;
	for(auto desc : flat_desc_list) 
	{
			EntityDescriptionField *edf = desc->GetField(EDF_ID);
        if(edf){
            int id;
            memread(&id, sizeof(int), 1, edf->data);
            int new_id = scenegraph->GetAndReserveID();

		    edf->data.clear();
		    edf->WriteInt(new_id);
		    scene_ids.insert(std::pair<int, int>(id, new_id));
        }
	}
	for(auto desc : flat_desc_list) 
	{
			EntityDescriptionField *edf = desc->GetField(EDF_CONNECTIONS);
		if(edf) {
			std::vector<int> connections;
			std::vector<int> write_connections;
			edf->ReadIntVec(&connections);
			for(int id : connections) 
			{
                Iter iter = scene_ids.find(id);
                if(iter != scene_ids.end())
                    id = iter->second;
				write_connections.push_back(id);
			}
			edf->data.clear();
			edf->WriteIntVec(write_connections);
		}
		edf = desc->GetField(EDF_ITEM_CONNECTIONS);
		if(edf) {
			std::vector<ItemConnectionData> connections;
			std::vector<ItemConnectionData> write_connections;
			edf->ReadItemConnectionDataVec(&connections);
			for(auto write_data : connections) 
			{
                Iter iter = scene_ids.find(write_data.id);
                if(iter != scene_ids.end())
                    write_data.id = iter->second;
                write_connections.push_back(write_data);
			}
			edf->data.clear();
			edf->WriteItemConnectionDataVec(write_connections);
		}
		edf = desc->GetField(EDF_NAV_MESH_CONNECTIONS);
		if(edf) {
			std::vector<NavMeshConnectionData> connections;
			std::vector<NavMeshConnectionData> write_connections;
			edf->ReadNavMeshConnectionDataVec(&connections);
			for(auto write_data : connections) 
			{
                Iter iter = scene_ids.find(write_data.other_object_id);
                if(iter != scene_ids.end())
                    write_data.other_object_id = iter->second;
                write_connections.push_back(write_data);
			}
			edf->data.clear();
			edf->WriteNavMeshConnectionDataVec(write_connections);
		}
		edf = desc->GetField(EDF_CONNECTIONS_FROM);
		if(edf) {
			std::vector<int> connections;
			std::vector<int> write_connections;
			edf->ReadIntVec(&connections);
			for(int id : connections) 
			{
                Iter iter = scene_ids.find(id);
                if(iter != scene_ids.end())
                    id = iter->second;
				write_connections.push_back(id);
			}
			edf->data.clear();
			edf->WriteIntVec(write_connections);
		}
	}
}

TypeEnable::TypeEnable(const char* config_postfix):
    unknown_types_enabled_(true),
    config_postfix(config_postfix)
{
    SetFromConfig();
}

void TypeEnable::SetTypeEnabled( EntityType type, bool enabled ) {
    type_enabled_[type] = enabled;
    WriteTypeString(type);
}

void TypeEnable::SetAll( bool enabled ) {
    unknown_types_enabled_ = enabled;
    type_enabled_.clear();
    WriteToConfig();
}

bool TypeEnable::IsTypeEnabled( EntityType type ) const {
    TypeEnabledMap::const_iterator it = type_enabled_.find(type);
    if(it == type_enabled_.end()){
        return unknown_types_enabled_;
    } else {
        return it->second;
    }
}

void TypeEnable::SetFromConfig() {
    SetTypeEnabled(_camera_type, ReadTypeString(_camera_type));
    SetTypeEnabled(_terrain_type, ReadTypeString(_terrain_type));
    SetTypeEnabled(_env_object, ReadTypeString(_env_object));
    SetTypeEnabled(_movement_object, ReadTypeString(_movement_object));
    SetTypeEnabled(_spawn_point, ReadTypeString(_spawn_point));
    SetTypeEnabled(_decal_object, ReadTypeString(_decal_object));
    SetTypeEnabled(_hotspot_object, ReadTypeString(_hotspot_object));
    SetTypeEnabled(_group, ReadTypeString(_group));
    SetTypeEnabled(_rigged_object, ReadTypeString(_rigged_object));
    SetTypeEnabled(_item_object, ReadTypeString(_item_object));
    SetTypeEnabled(_path_point_object, ReadTypeString(_path_point_object));
    SetTypeEnabled(_ambient_sound_object, ReadTypeString(_ambient_sound_object));
    SetTypeEnabled(_placeholder_object, ReadTypeString(_placeholder_object));
    SetTypeEnabled(_light_probe_object, ReadTypeString(_light_probe_object));
	SetTypeEnabled(_dynamic_light_object, ReadTypeString(_dynamic_light_object));
    SetTypeEnabled(_navmesh_hint_object, ReadTypeString(_navmesh_hint_object));
    SetTypeEnabled(_navmesh_region_object, ReadTypeString(_navmesh_region_object));
    SetTypeEnabled(_navmesh_connection_object, ReadTypeString(_navmesh_connection_object));
    SetTypeEnabled(_reflection_capture_object, ReadTypeString(_reflection_capture_object));
    SetTypeEnabled(_light_volume_object, ReadTypeString(_light_volume_object));
    SetTypeEnabled(_prefab, ReadTypeString(_prefab));
}

void TypeEnable::WriteToConfig() {
    WriteTypeString(_camera_type);
    WriteTypeString(_terrain_type);
    WriteTypeString(_env_object);
    WriteTypeString(_movement_object);
    WriteTypeString(_spawn_point);
    WriteTypeString(_decal_object);
    WriteTypeString(_hotspot_object);
    WriteTypeString(_group);
    WriteTypeString(_rigged_object);
    WriteTypeString(_item_object);
    WriteTypeString(_path_point_object);
    WriteTypeString(_ambient_sound_object);
    WriteTypeString(_placeholder_object);
    WriteTypeString(_light_probe_object);
	WriteTypeString(_dynamic_light_object);
    WriteTypeString(_navmesh_hint_object);
    WriteTypeString(_navmesh_region_object);
    WriteTypeString(_navmesh_connection_object);
    WriteTypeString(_reflection_capture_object);
    WriteTypeString(_light_volume_object);
    WriteTypeString(_prefab);
}

bool TypeEnable::ReadTypeString(EntityType type) {
    char buffer[128];
    FormatString(buffer, 128, "type_enabled_%s_%s", config_postfix, CStringFromEntityType(type));
    if(config.HasKey(buffer)) {
        return config[buffer].toBool();
    } else {
        return unknown_types_enabled_;
    }
}

void TypeEnable::WriteTypeString(EntityType type) const {
    char buffer[128];
    FormatString(buffer, 128, "type_enabled_%s_%s", config_postfix, CStringFromEntityType(type));
    config.GetRef(buffer) = IsTypeEnabled(type);
}
