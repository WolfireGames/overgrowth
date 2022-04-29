//-----------------------------------------------------------------------------
//           Name: navmeshrenderer.cpp
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
#include "navmeshrenderer.h"

#include <Graphics/camera.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/textures.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>
#include <Math/overgrowth_geometry.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Memory/allocation.h>
#include <AI/mesh_loader_obj.h>
#include <AI/navmesh.h>
#include <Logging/logdata.h>

NavMeshRenderer::NavMeshRenderer() :
updated_nav_mesh_(false),
nav_mesh_(NULL),
nav_mesh_visible_(false),
collision_mesh_visible_(false)
{
}

NavMeshRenderer::~NavMeshRenderer()
{
}

void NavMeshRenderer::LoadNavMesh( const NavMesh* _nav_mesh )
{
    nav_mesh_ = _nav_mesh;
    updated_nav_mesh_ = true;
    //We can't do any OPENGL fill from here because this function might be called from a thread.
}

void NavMeshRenderer::Draw(  )
{
    DrawCollision();
    DrawNavMesh();

    updated_nav_mesh_ = false;
}


void NavMeshRenderer::SetCollisionMeshVisible( bool v )
{
    collision_mesh_visible_ = v;
}

void NavMeshRenderer::SetNavMeshVisible( bool v )
{
    nav_mesh_visible_ = v;
}

bool NavMeshRenderer::IsCollisionMeshVisible()
{
    return collision_mesh_visible_;
}

bool NavMeshRenderer::IsNavMeshVisible()
{
    return nav_mesh_visible_;
}

void NavMeshRenderer::DrawCollision(  )
{
    if( updated_nav_mesh_ && nav_mesh_ && nav_mesh_->getCollisionMesh() )
    {
        const rcMeshLoaderObj* col_mesh = nav_mesh_->getCollisionMesh();
    
        collision_mesh_vertices_.Fill( kVBODynamic | kVBOFloat, col_mesh->getVertCount() * 3 * sizeof(float), (void*)col_mesh->getVerts());

        GLuint *faces = (GLuint*)OG_MALLOC(col_mesh->getTriCount()*3*sizeof(GLuint));

        for( int i = 0; i < col_mesh->getTriCount()*3; i++ )
        {
            faces[i] = (GLuint)col_mesh->getTris()[i];
        }

        collision_mesh_faces_.Fill( kVBODynamic | kVBOElement, col_mesh->getTriCount() * 3 * sizeof(GLuint), (void*)faces);
    }

    if( IsCollisionMeshVisible() )
    {
        if( collision_mesh_vertices_.valid() && collision_mesh_faces_.valid() )
        {
            Shaders* shaders = Shaders::Instance();
            Camera* cam = ActiveCameras::Get();
            Graphics* graphics = Graphics::Instance();

            int shader_id = shaders->returnProgram("debug_fill #NAV_COLLISION_MESH", Shaders::kGeometry);
            shaders->setProgram(shader_id);
            int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
        
            shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix());
            shaders->SetUniformVec3("camera_forward", cam->GetFacing());

            graphics->EnableVertexAttribArray(vert_attrib_id);
            collision_mesh_vertices_.Bind();
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);

            collision_mesh_faces_.Bind();
            graphics->DrawElements(GL_TRIANGLES, collision_mesh_faces_.size() / sizeof(unsigned), GL_UNSIGNED_INT, 0);

            graphics->ResetVertexAttribArrays();
        }
        else
        {
            LOGE << "Error" << std::endl;
        }
    }
}

void NavMeshRenderer::DrawNavMesh()
{
    //We could make this selective and only render nearby tiles instead of all global ones.
    if( updated_nav_mesh_ && nav_mesh_)
    {
        const dtNavMesh* mesh = nav_mesh_->getNavMesh();

        std::vector<float> data;
        std::vector<float> edge_data;
        std::vector<float> thick_edge_data;

        for (int i = 0; i < mesh->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = mesh->getTile(i);
            if (!tile->header) continue;
            //dtPolyRef base = mesh->getPolyRefBase(tile);
            //
            
            for (int j = 0; j < tile->header->offMeshConCount; j++ )
            {
                LOGI << "There is an off mesh con in tile " << i << std::endl;
                const dtOffMeshConnection* off_mesh_con = &tile->offMeshCons[j];
               
                if( off_mesh_con )
                {
                }  
            }

            for (int j = 0; j < tile->header->polyCount; ++j)
            {
                const dtPoly* poly = &tile->polys[j];
                
                const unsigned int ip = (unsigned int)(poly - tile->polys);

                if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                {
                    LOGI << "Offmesh connection!" << std::endl;
                    //dtOffMeshConnection* con = &tile->offMeshCons[ip - tile->header->offMeshBase];
                    //Off mesh connections are user defined connections between different vertices, they are signified by two points, creatig a line.
                }
                else
                {
                    const dtPolyDetail* pd = &tile->detailMeshes[ip];

                    for (int i = 0; i < pd->triCount; ++i)
                    {
                        const unsigned char* t = &tile->detailTris[(pd->triBase+i)*4];
                        for (int j = 0; j < 3; ++j)
                        {
                            if (t[j] < poly->vertCount)
                            {
                                data.push_back(tile->verts[poly->verts[t[j]]*3]);
                                data.push_back(tile->verts[poly->verts[t[j]]*3+1]);
                                data.push_back(tile->verts[poly->verts[t[j]]*3+2]);
                            }
                            else
                            {
                                data.push_back(tile->detailVerts[(pd->vertBase+t[j]-poly->vertCount)*3]);
                                data.push_back(tile->detailVerts[(pd->vertBase+t[j]-poly->vertCount)*3+1]);
                                data.push_back(tile->detailVerts[(pd->vertBase+t[j]-poly->vertCount)*3+2]);
                            }
                        }
                    }

                    for( int i = 1; i < poly->vertCount; i++ )
                    {

                        std::vector<float>* dest;
                        //Check if outer edge.
                        if( poly->neis[i-1] == 0 )
                        {
                            dest = &thick_edge_data;
                        }
                        else
                        {
                            dest = &edge_data;
                        }

                        dest->push_back(tile->verts[poly->verts[i-1]*3+0]);
                        dest->push_back(tile->verts[poly->verts[i-1]*3+1]);
                        dest->push_back(tile->verts[poly->verts[i-1]*3+2]);

                        dest->push_back(tile->verts[poly->verts[i]*3+0]);
                        dest->push_back(tile->verts[poly->verts[i]*3+1]);
                        dest->push_back(tile->verts[poly->verts[i]*3+2]);
                    }

                    if( poly->vertCount > 2 )
                    {
                        std::vector<float>* dest;
                        //Check if outer edge.
                        if( poly->neis[poly->vertCount-1] == 0 )
                        {
                            dest = &thick_edge_data;
                        }
                        else
                        {
                            dest = &edge_data;
                        }

                        dest->push_back(tile->verts[poly->verts[poly->vertCount-1]*3+0]);
                        dest->push_back(tile->verts[poly->verts[poly->vertCount-1]*3+1]);
                        dest->push_back(tile->verts[poly->verts[poly->vertCount-1]*3+2]);

                        dest->push_back(tile->verts[poly->verts[0]*3+0]);
                        dest->push_back(tile->verts[poly->verts[0]*3+1]);
                        dest->push_back(tile->verts[poly->verts[0]*3+2]);
                    }
                }
            }
        }

        if(!data.empty()){
            navmesh_vertices_.Fill( kVBODynamic | kVBOFloat, data.size() * sizeof(float), (void*)&data[0]);
        }
        if(!edge_data.empty()){
            navmesh_edges_.Fill( kVBODynamic | kVBOFloat, edge_data.size() * sizeof(float), (void*)&edge_data[0] );
        }
        if(!thick_edge_data.empty()){
            navmesh_thick_edges_.Fill( kVBODynamic | kVBOFloat, thick_edge_data.size() * sizeof(float), (void*)&thick_edge_data[0] );
        }
    }

    if( IsNavMeshVisible() )
    {
        if( navmesh_vertices_.valid() )
        {
            Shaders* shaders = Shaders::Instance();
            Camera* cam = ActiveCameras::Get();
            Graphics* graphics = Graphics::Instance();

            int shader_id = shaders->returnProgram("debug_fill #STIPPLING");
            shaders->setProgram(shader_id);
            int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
        
            shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix());
            shaders->SetUniformVec3("close_stipple_color", vec3(0.1f, 0.8f, 1.0f));
            shaders->SetUniformVec3("far_stipple_color", vec3(0.25f, 0.29f, 0.3f));
            shaders->SetUniformVec3("camera_forward", cam->GetFacing());
			shaders->SetUniformVec3("camera_position", cam->GetPos());

            graphics->EnableVertexAttribArray(vert_attrib_id);
            navmesh_vertices_.Bind();
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);

            graphics->DrawArrays(GL_TRIANGLES, 0, navmesh_vertices_.size() / (sizeof(float)*3));

            graphics->ResetVertexAttribArrays();
        }
        else
        {
            LOGE << "Error" << std::endl;
        }

        if( navmesh_edges_.valid() || navmesh_thick_edges_.valid() )
        {
            Shaders* shaders = Shaders::Instance();
            Graphics* graphics = Graphics::Instance();
            Camera* cam = ActiveCameras::Get();

            int shader_id = shaders->returnProgram("3d_color #COLOR_UNIFORM #NO_VELOCITY_BUF");
            shaders->setProgram(shader_id);
            int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
        
            shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix());

            graphics->EnableVertexAttribArray(vert_attrib_id);
            if(navmesh_edges_.valid()){
                shaders->SetUniformVec4("color_uniform",  vec4( 0.1f, 0.1f, 0.1f, 1.0f));
                navmesh_edges_.Bind();
                glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);
                graphics->DrawArrays(GL_LINES, 0, navmesh_edges_.size() / (sizeof(float)*3));
            }


            if(navmesh_thick_edges_.valid()){
                graphics->SetLineWidth(3);
                shaders->SetUniformVec4("color_uniform",  vec4( 0.0, 0.0, 0.0, 1.0));
                navmesh_thick_edges_.Bind();
                glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);
                graphics->DrawArrays(GL_LINES, 0, navmesh_thick_edges_.size() / (sizeof(float)*3));
            }

            graphics->ResetVertexAttribArrays();
        }
    }
}
