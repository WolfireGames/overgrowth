//-----------------------------------------------------------------------------
//           Name: navmesh.h
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

#include <AI/pathfind.h>
#include <AI/tilemesh.h>
#include <AI/input_geom.h>
#include <AI/navmeshparameters.h>

#include <Math/mat4.h>
#include <Math/vec3.h>

#include <Internal/path.h>

#include <vector>
#include <string>

using std::vector;
using std::string;

class rcMeshLoaderObj;

class NavPoint
{
private:
    vec3 pos;
    bool valid;
public:
    NavPoint( vec3 pos );
	NavPoint( vec3 pos, bool success );
    NavPoint( );
    NavPoint( const NavPoint& other );

    vec3 GetPoint();
    bool IsSuccess();
};

class NavMesh {
public:
	NavMesh():loaded_(false){}
    void AddMesh( const vector<float>& vertices, const vector<unsigned>& faces, const mat4 &transform );
    void SetNavMeshParameters(NavMeshParameters& nmp);
    void CalcNavMesh();
    void Save( const string &level_name, const Path &level_path );
    bool Load( const string &level_name, const Path &level_path );
    bool LoadFromAbs(const Path& level_path, const char* abs_meta_path, const char* abs_model_path, const char * abs_nav_path, const char* abs_zip_path, bool has_zip );
    void Update();
    NavPath FindPath(const vec3 &start, const vec3 &end, uint16_t include_filter, uint16_t exclude_filter);
    vec3 RayCast( const vec3 &start, const vec3 &end );
    vec3 RayCastSlide( const vec3 &start, const vec3 &end, int depth );
    NavPoint GetNavPoint( const vec3 &point );

    const rcMeshLoaderObj* getCollisionMesh() const;
    const dtNavMesh* getNavMesh() const;
    const rcPolyMesh* getPolyMesh() const;
    InputGeom &getInputGeom(); 
    void SetExplicitBounderies(vec3 min, vec3 max );

    unsigned short GetOffMeshConnectionFlag( int userid );
private:
    bool loaded_;
    TileMesh sample_tile_mesh_;
    InputGeom geom_;
    NavMeshParameters nav_mesh_parameters_;
    //TileMeshTesterTool nav_mesh_tester_;

    //TODO: Replace this middle storage with direct append into InputGeom
    //These arrays are only used for loading storage and don't represent internal state.
    vector<float> vertices_;
    vector<unsigned> faces_;

    float meshBMin_[3];
    float meshBMax_[3];
    
    vec3 m_start_;
    vec3 m_end_;
    vector<vec3> path_points_;

    void CalcFaceNormals(vector<float> &face_normals);
};
