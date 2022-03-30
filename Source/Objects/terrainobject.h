//-----------------------------------------------------------------------------
//           Name: terrainobject.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The terrain object is an entity representing some terrain
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

#include <Graphics/terrain.h>
#include <Graphics/glstate.h>
#include <Graphics/textureref.h>

#include <Objects/object.h>

#include <Internal/levelxml.h>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

struct MaterialEvent;
struct MaterialDecal;
class BulletObject;

class TerrainObject: public Object {
    public:   
        const TerrainInfo& terrain_info() const;
        virtual EntityType GetType() const { return _terrain_type; }
        
        TerrainObject(const TerrainInfo &terrain_info);
        ~TerrainObject();
        virtual void HandleMaterialEvent( const std::string &the_event, const vec3 &event_pos, int* tri );
        virtual void PreDrawCamera(float curr_game_time);
        virtual void PreDrawFrame(float curr_game_time);
        virtual void Draw();
        virtual void DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type);
        virtual int lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal=0);
        virtual bool Initialize();
        virtual void GetShaderNames(std::map<std::string, int>& preload_shaders);
        virtual const MaterialEvent& GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, int* tri );
        virtual const MaterialEvent& GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, const std::string &mod, int* tri );
        virtual const MaterialDecal& GetMaterialDecal( const std::string &type, const vec3 &pos, int* tri );
        virtual const MaterialParticle& GetMaterialParticle( const std::string &type, const vec3 &pos, int* tri );
        virtual void GetDisplayName(char* buf, int buf_size);
        virtual MaterialRef GetMaterial( const vec3 &pos, int* tri = NULL );
        virtual vec3 GetColorAtPoint( const vec3 &pos, int* tri );
        
        void PreparePhysicsMesh();
        const Model* GetModel() const;
        void DrawTerrain();

        bool preview_mode;

        virtual void ReceiveObjectMessageVAList( OBJECT_MSG::Type type, va_list args );
        BulletObject* bullet_object_;
        Terrain terrain_;

        void SetTerrainColorTexture(const char* path);
        void SetTerrainWeightTexture(const char* path);
        void SetTerrainDetailTextures(const std::vector<DetailMapInfo>& detail_map_info);
private:  
        GLState gl_state_;
        GLState edge_gl_state_;
        int bullet_entry_;
        bool added_to_physics_scene_;
        std::string shader_extra;
        
        TerrainInfo terrain_info_;
    
};
