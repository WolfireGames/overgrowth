//-----------------------------------------------------------------------------
//           Name: envobject.h
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

#include <Online/online_datastructures.h>
#include <Online/time_interpolator.h>

#include <Asset/Asset/objectfile.h>
#include <Asset/Asset/averagecolorasset.h>

#include <Editors/editor_types.h>
#include <Editors/editor_utilities.h>

#include <Objects/object.h>
#include <Graphics/textureref.h>

#include <Math/overgrowth_geometry.h>
#include <Game/color_tint_component.h>

#include <memory>
#include <list>

class DetailObjectSurface;
class ObjectEditor;
class ObjectsEditor;
class DecalObject;
class Collision;
class BulletObject;
class BulletWorld;
class quaternion;

using std::list;

const char _hull_cache_version = 2;
struct HullCache {
    std::vector<vec3> point_cloud;
    std::vector<vec3> verts;
    std::vector<int> faces;
};
typedef std::map<std::string, HullCache> HullCacheMap;

class PlantComponent {
    float plant_shake;
    vec3 angle;
    vec3 ang_vel;
    vec3 pivot;
    bool was_active;
    bool needs_set_inactive;
    HullCache *hull_cache;
public:
    PlantComponent();
    bool IsPivotCalculated() const;
    mat4 GetTransform(float scale) const;
    quaternion GetQuaternion(float scale) const;
    float GetPlantShake( float scale ) const;
    vec3 GetPivot() const;

    bool IsActive();
    bool NeedsSetInactive();
    void SetPivot(BulletWorld &bw, const vec3 &pos, float radius);
    void Update(float timestep);
    void HandleCollision( const vec3 &position, const vec3 &velocity );
    void ClearPivot();
    HullCache *GetHullCache();
    void SetHullCache( HullCache * hc );
};

class Model;
class MovementObject;
struct MaterialEvent;
struct MaterialParticle;

class EnvObject: public Object {
    public:
        float timestamp_for_sendoff = 0.0f;
        std::auto_ptr<PlantComponent> plant_component_;
        ObjectFileRef ofr;

        BulletObject* bullet_object_;
        
        float sphere_radius_;
        vec3 sphere_center_;

        TextureAssetRef ambient_cube_ref_[6];

        bool no_navmesh;

        ModID modsource_;

        std::vector<TextureAssetRef> texture_ref_;
        std::vector<TextureAssetRef> normal_texture_ref_;
        std::vector<TextureAssetRef> translucency_texture_ref_;
        
        TextureAssetRef weight_map_ref_;
        std::vector<unsigned int> detail_texture_color_indices_;
        std::vector<unsigned int> detail_texture_normal_indices_;
        std::vector<vec4> detail_texture_color_;
        std::vector<vec4> detail_texture_color_srgb_;

        std::vector<vec3> normal_override;
        std::vector<vec4> normal_override_custom;
        std::vector<int> ledge_lines;

        TextureRef normal_override_buffer;
        bool normal_override_buffer_dirty;

        bool csg_modified_;
    
        int model_id_;
        bool added_to_physics_scene_;
        bool winding_flip;
        MovementObject* attached_;
        bool placeholder_;

        list<OnlineMessageRef> incoming_online_env_update;

        EnvObject();
        virtual ~EnvObject();

        virtual bool Initialize();
        virtual void GetShaderNames(std::map<std::string, int>& shaders);
        virtual void Update(float timestep);
        void UpdateBoundingSphere();

		std::string GetLabel();

        // Drawing
        virtual void Draw();
        void DrawInstances(EnvObject** instance_array, int num_instances, const mat4& proj_view_matrix, const mat4& prev_proj_view_matrix, const std::vector<mat4>* shadow_matrix, const vec3& cam_pos, Object::DrawType type);
        bool HasDetailObjectSurfaces() const { return !detail_object_surfaces.empty(); }
        void DrawDetailObjectInstances(EnvObject** instance_array, int num_instances, Object::DrawType type);
        virtual void PreDrawCamera(float curr_game_time);
        void DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type);
        virtual void SetEnabled(bool val);
        virtual void SetCollisionEnabled(bool val);
        virtual void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args);

        void GetObj2World(float *obj2world);
        
		void EnvInterpolate(uint16_t pending_updates);

        // Line hits
        virtual int lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal=0);
        
        bool UpdatePhysicsTransform();
        virtual void GetDisplayName(char* buf, int buf_size);
        
        void CreatePhysicsShape();

        virtual void Moved(Object::MoveType type);
        void RemovePhysicsShape();
        const Model* GetModel() const;
        vec3 GetBoundingBoxSize();
        void HandleMaterialEvent( const std::string &the_event, const vec3 &event_pos );
        const MaterialEvent& GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, int *tri );
        const MaterialEvent& GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, const std::string &mod, int *tri);
        const MaterialDecal& GetMaterialDecal( const std::string &type, const vec3 &pos );
        const MaterialParticle& GetMaterialParticle( const std::string &type, const vec3 &pos );
        MaterialRef GetMaterial( const vec3 &pos, int* tri = NULL );
        void UpdateDetailScale();
        bool Load( const std::string& type_file );
        void Reload();
        const vec3 &GetColorTint();
        const float& GetOverbright();
        void LoadModel();
        BulletWorld* GetBulletWorld();
        int GetCollisionModelID();
        void CreateBushPhysicsShape();
        void ReceiveASVec3Message( int type, const vec3 &vec_a, const vec3 &vec_b );
        void CreateLeaf(vec3 pos, vec3 vel, int iterations);
        virtual bool SetFromDesc( const EntityDescription& desc );
        virtual void GetDesc(EntityDescription &desc) const;
		virtual void UpdateParentHierarchy();
        void SetCSGModified();
        typedef std::vector<DetailObjectSurface*> DOSList;
    protected:
        virtual EntityType GetType() const { return _env_object; }
        DOSList detail_object_surfaces;

        vec3 GetDisplayTint();
		ColorTintComponent color_tint_component_;
        vec3 base_color_tint;
        vec3 cached_combined_tint_;

        vec3 m_transform_starting_sphere_center;
        float m_transform_starting_sphere_radius;

        std::set<AverageColorRef> average_color_refs;
        MaterialRef ofr_material;

        TimeInterpolator network_time_interpolator;

    private:
        void CalculateDisplayTint_();
};

void DefineEnvObjectTypePublic(ASContext* as_context);
