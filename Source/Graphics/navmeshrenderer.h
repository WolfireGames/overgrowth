//-----------------------------------------------------------------------------
//           Name: navmeshrenderer.h
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

#include <AI/navmesh.h>
#include <Graphics/vbocontainer.h>

//Note that this renderer only renders the state which the nav-mesh was in when it was last set. 
//The data will be reloaded if updated_nav_mesh_ flag is set to true.

class NavMeshRenderer {
public:
    NavMeshRenderer();
    ~NavMeshRenderer();
    void LoadNavMesh( const NavMesh* _navmesh );
    void Draw();
   
    void SetCollisionMeshVisible( bool v );
    void SetNavMeshVisible( bool v );

    bool IsCollisionMeshVisible();
    bool IsNavMeshVisible();

private:
    void DrawCollision();
    void DrawNavMesh();

    bool updated_nav_mesh_;
    const NavMesh* nav_mesh_;

    VBOContainer collision_mesh_vertices_;
    VBOContainer collision_mesh_faces_;

    VBOContainer navmesh_vertices_;
    VBOContainer navmesh_edges_;
    VBOContainer navmesh_thick_edges_;

    bool nav_mesh_visible_, collision_mesh_visible_;
};
