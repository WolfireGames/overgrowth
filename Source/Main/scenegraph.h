//-----------------------------------------------------------------------------
//           Name: scenegraph.h
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

#include <Graphics/drawbatch.h>
#include <Graphics/lightprobecollection.hpp>
#include <Graphics/dynamiclightcollection.hpp>
#include <Graphics/flares.h>
#include <Graphics/navmeshrenderer.h>

#include <Editors/entity_type.h>
#include <Editors/object_sanity_state.h>

#include <Math/vec4.h>
#include <Objects/object_msg.h>
#include <Asset/Asset/material.h>
#include <Internal/collisiondetection.h>
#include <Scripting/scriptparams.h>

#include <list>
#include <map>

class Sound;
class Object;
class TerrainObject;
class DecalObject;
class Hotspot;
class Group;
struct SphereCollision;
class BulletWorld;
class NavMesh;
class ParticleSystem;
class MapEditor;
class Level;
class Sky;
class Textures;
class EnvObject;
class LightVolumeObject;
class MovementObject;

struct SceneLight {
    vec3 pos;
    vec3 color;
    float intensity;
};

struct ShadowCacheObjectLightBounds {
    float min_bounds[2];
    float max_bounds[2];
    bool is_calculated;
    bool is_ignored;
};

class SceneGraph {
    public:
        enum SceneDrawType { kStaticOnly, kStaticAndDynamic };
        SceneLight primary_light;
        MapEditor* map_editor;
        Level* level;
        Sky* sky;
        Flares flares;
        ParticleSystem *particle_system;
        TerrainObject* terrain_object_;
        LightProbeCollection light_probe_collection;
		DynamicLightCollection dynamic_light_collection;

        std::vector<mat4> ref_cap_matrix;
        std::vector<mat4> ref_cap_matrix_inverse;
        TextureRef cubemaps;
        std::vector<TextureRef> sub_cubemaps;
        bool cubemaps_need_refresh;
        bool reflection_data_loaded;
        float fog_amount;
        float haze_mult; // derived from fog_amount

        unsigned infreq_update_index;

        Path level_path_;
        bool level_has_been_previously_saved_;
        std::string level_name_;
        std::string level_visible_name_;
        std::string level_visible_description_;

        // Objects that must be updated every timestep
        static const int kMaxUpdateObjects = 1024;
        Object* update_objects_[kMaxUpdateObjects];
        int num_update_objects;

        typedef std::vector<Object*> object_list;
        object_list objects_;
        object_list collide_objects_;
        object_list visible_objects_; // Objects that have a Draw() function
        std::vector<EnvObject*> visible_static_meshes_;
        std::vector<ShadowCacheObjectLightBounds> visible_static_meshes_shadow_cache_bounds_;
        std::vector<uint16_t> visible_static_mesh_indices_;  // Will be used to keep both arrays in sorted order. Should never be more than 65536 visible static meshes
        std::vector<TerrainObject*> terrain_objects_;
        std::vector<ShadowCacheObjectLightBounds> terrain_objects_shadow_cache_bounds_;
        object_list decal_objects_;
        object_list movement_objects_;
        object_list item_objects_;
        object_list hotspots_;
        object_list navmesh_hints_;
        object_list navmesh_connections_;
        object_list path_points_;
        std::vector<LightVolumeObject*> light_volume_objects_;
        std::vector<int> object_ids_to_delete;

        bool hotspots_modified_;

        static const size_t destruction_sanity_size = 256;
        size_t destruction_sanity_insert_position;
        Object* destruction_sanity[destruction_sanity_size];

        static const size_t destruction_memory_size = 128;
        static const size_t destruction_memory_string_size = 128;
        size_t destruction_memory_insert_position;
        int destruction_memory_ids[destruction_memory_size];
        char destruction_memory_strings[destruction_memory_size*destruction_memory_string_size];

        static const unsigned int kMaxWarnings = 64;
        ObjectSanityState sanity_list[kMaxWarnings];
        uint32_t partial_object_loop_counter;

		// Maximum total number of static and dynamic decals
		// This could be raised as high as 65535, after that refactoring is required to replace 16-bit uints
		static const unsigned int kMaxDecals = 20000;
		// Maximum number of decals added at run time
		// When count exceeds this older ones are removed
		static const unsigned int kMaxDynamicDecals = 1000;
		// Maximum number of decals that can be added via editor
		static const unsigned int kMaxStaticDecals = kMaxDecals - kMaxDynamicDecals;
		typedef std::deque<DecalObject*> decal_deque;
		decal_deque dynamic_decals;

        BulletWorld *bullet_world_;
        BulletWorld *abstract_bullet_world_;
        BulletWorld *plant_bullet_world_;
        
        SceneGraph();
        ~SceneGraph();
        
        void UpdatePhysicsTransforms();
        bool addObject(Object* new_object);

		bool AddDynamicDecal(DecalObject *decal);

        Collision lineCheck(const vec3 &start, const vec3 &end);
        Collision lineCheckCollidable(const vec3 &start, const vec3 &end, Object* notHit = NULL);
        void LineCheckAll(const vec3 &start, const vec3 &end, std::vector<Collision> *collisions);

        void Update(float timestep, float curr_game_time);
        void Draw(SceneGraph::SceneDrawType scene_draw_type);
        
        Object* GetLastSelected();
        void ReturnSelected(std::vector<Object*>* selected);
        void UnselectAll();

        Object* GetObjectFromID(int object_id);
        bool DoesObjectWithIdExist(int object_id);
        std::vector<Object*> GetObjectsOfType(enum EntityType type);
        void UnlinkObject(Object *o);
        void LinkObject(Object* new_object);
        void CreateNavMesh();
        void SaveNavMesh();
        bool LoadNavMesh();
        void AddSceneToNavmesh();
        NavMesh* GetNavMesh();
        const MaterialEvent *GetMaterialEvent( const std::string &the_event, const vec3 &event_pos );
        const MaterialEvent *GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, const std::string &mod );
        const MaterialEvent *GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, Object* excluded_object );
        const MaterialEvent *GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, const std::string &mod, Object* excluded_object );
        Object* GetClosestObject(const vec3 &pos, Object* excluded_object = NULL, vec3 *hit_pos = NULL, int *hit_tri = NULL);
        const MaterialDecal *GetMaterialDecal( const std::string &type, const vec3 &pos );
        const MaterialParticle *GetMaterialParticle( const std::string &type, const vec3 &pos );
        float GetMaterialHardness( const vec3 &pos, Object* excluded_object = NULL );
        float GetMaterialFriction( const vec3 &pos, Object* excluded_object = NULL );
        float GetMaterialSharpPenetration( const vec3 &pos, Object* excluded_object = NULL );
        vec3 GetColorAtPoint( const vec3 &pos );
        void AssignID(Object* obj);
        void QueueLevelReset();
        void GetSweptSphereCollisionCharacters( const vec3 &pos, const vec3 &pos2, float radius, SphereCollision &as_col );
		int CheckRayCollisionCharacters( const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal, int *bone );
		void GetPlayerCharacterIDs(int* num_avatars, int avatar_ids[], int max_avatars);
		void GetNPCCharacterIDs(int* num_avatars, int avatar_ids[], int max_avatars);
        void GetCharacterIDs(int* num_avatars, int avatar_ids[], int max_avatars);
        void SendMessageToAllObjects( OBJECT_MSG::Type type );
        void SendScriptMessageToAllObjects( std::string& msg );
        std::vector<MovementObject*> GetControlledMovementObjects();
        std::vector<MovementObject*> GetControllableMovementObjects();

        bool VerifySanity();

        enum DepthType {
            kDepthShadow,
            kDepthAllShadowCascades,
            kDepthPrePass
        };
        void DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, DepthType depth_type, SceneDrawType scene_draw_type);
        int GetAndReserveID();
        int LinkUpdateObject( Object* obj );
        void UnlinkUpdateObject( Object* obj, int entry );
        void Dispose();

        void SetNavMeshVisible( bool v );
        void SetCollisionNavMeshVisible( bool v );

        bool IsNavMeshVisible();
        bool IsCollisionNavMeshVisible();

        void PrepareLightsAndDecals(vec2 active_screen_start, vec2 active_screen_end, vec2 screen_dims);
        void BindDecals(int the_shader);
        void BindLights(int the_shader);
        
        static void ApplyScriptParams(SceneGraph* scenegraph, const ScriptParamMap& spm);

        void PrintCurrentObjects();
        int CountObjectsWithName(const char* name);
        bool IsObjectSane(Object* obj);

        const char* GetDestroyedObjectInfo(int object_id);

        void DumpState();

        enum PreloadType {
            kUnknown = (1<<0),
            kDrawDepthOnly = (1<<1),
            kDrawAllShadowCascades = (1<<2),
            kDrawDepthNoAA = (1<<3),
            kFullDraw = (1<<4),
            kWireframe = (1<<5),
            kDecal = (1<<6),
            kPreloadTypeAll = (1<<7) - 1,
            kOptionalNone = (1<<7),
            kOptionalGeometry = (1<<8),
            kOptionalTessellation = (1<<9)
        };
        void GetParticleShaderNames(std::map<std::string, int>& preload_shaders);
        void PreloadForDrawType(std::map<std::string, int>& preload_shaders, PreloadType type);
        void PreloadShaders();
private:
        bool visible_objects_need_sort;
        bool queued_level_reset_;
        typedef std::vector<Object*> IDMap;
        IDMap object_from_id_map_;
        NavMesh *nav_mesh_;

        NavMeshRenderer nav_mesh_renderer_;

        // this buffer contains decal and light data
        std::vector<float> decal_tbo;
        TextureRef decal_data_texture;

        // this buffer contains
        // 1. grid lookup (should be in a separate 3D texture)
        // 2. decal cluster contents (decal indices)
        // 2. light cluster contents (light indices)
        std::vector<uint32_t> decal_cluster_buffer;
        TextureRef decal_cluster_texture;

        // int is a bitmask of Object::DrawType
        std::map<std::string, int> preload_shaders;

struct ClusterData {
	uint32_t next;
	uint16_t item;

    ClusterData()
	: next(0)
	, item(0)
	{
	}
};


	// in pixels
	// This value is hardcoded in shaders, so has to be modified there as well.
	unsigned int cluster_size;
	unsigned int num_z_clusters;

    // 3D texture of cluster pointers
    std::vector<uint32_t> cluster_list_heads;
    // 1D texture of cluster pointers
    std::vector<ClusterData> cluster_list_contents;

	// number of decals in the cluster, size = number of clusters
	std::vector<uint16_t> cluster_decal_counts;
	std::vector<uint32_t> decal_grid_lookup;
	std::vector<unsigned int> cluster_decals;

	// number of lights in each cluster, size = number of clusters
	std::vector<unsigned int> cluster_light_counts;
	std::vector<uint32_t> light_grid_lookup;
	std::vector<unsigned int> cluster_lights;
public:
	void LoadReflectionCaptureCubemaps();
    void UnloadReflectionCaptureCubemaps();
};

vec3 ColorFromString(const char* str);
