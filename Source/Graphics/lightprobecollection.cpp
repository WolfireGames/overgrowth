//-----------------------------------------------------------------------------
//           Name: lightprobecollection.cpp
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
#include "lightprobecollection.hpp"

#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/sky.h>
#include <Graphics/textures.h>
#include <Graphics/models.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Physics/bulletworld.h>
#include <Memory/allocation.h>
#include <Internal/profiler.h>
#include <Game/hardcoded_assets.h>
#include <Math/vec3math.h>
#include <Wrappers/glm.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <cfloat>

const float kQuantize = 10.0f;

LightProbeCollection::LightProbeCollection():
    tet_mesh_viz_enabled(false),
    show_probes(false),
    show_probes_through_walls(false),
    probe_lighting_enabled(true),
    light_volume_enabled(true),
	probe_model_id(-1)
{
    cube_map_fbo = INVALID_FRAMEBUFFER;
    next_id = 0;
    tet_mesh.display_id = -1;
    tet_mesh_needs_update = false;
    grid_lookup.bounds[0] = vec3(0.0f, 0.0f, 0.0f);
    grid_lookup.bounds[1] = vec3(0.0f, 0.0f, 0.0f);
    grid_lookup.subdivisions[0] = 0;
    grid_lookup.subdivisions[1] = 0;
    grid_lookup.subdivisions[2] = 0;
}

LightProbeCollection::~LightProbeCollection() {
    LOG_ASSERT(cube_map_fbo == INVALID_FRAMEBUFFER);
}

int LightProbeCollection::AddProbe(const vec3& pos, bool negative, const float *coeff) {
    light_probes.resize(light_probes.size()+1);
    LightProbe &probe = light_probes.back();
    probe.pos = pos;
    probe.negative = negative;
    probe.id = next_id++;
    if(!negative && !coeff){
        LightProbeUpdateEntry entry;
        entry.id = probe.id;
        entry.pass = 0;
        to_process.push(entry);
    } else if (coeff) {
        for (unsigned int i = 0; i < 6; i++) {
            probe.ambient_cube_color[i].entries[0] = coeff[i * 3 + 0];
            probe.ambient_cube_color[i].entries[1] = coeff[i * 3 + 1];
            probe.ambient_cube_color[i].entries[2] = coeff[i * 3 + 2];
        }
    }
    tet_mesh_needs_update = true;
    return probe.id;
}

bool LightProbeCollection::MoveProbe(int id, const vec3& pos) {
    LightProbe* probe = GetProbeFromID(id);
    if(probe){
        probe->pos = pos;
        LightProbeUpdateEntry entry;
        entry.id = id;
        entry.pass = 0;
        to_process.push(entry);
        tet_mesh_needs_update = true;
        return true;
    } else {
        return false;
    }
}

bool LightProbeCollection::SetNegative(int id, bool negative) {
    LightProbe* probe = GetProbeFromID(id);
    if(probe){
        probe->negative = negative;
        LightProbeUpdateEntry entry;
        entry.id = id;
        entry.pass = 0;
        to_process.push(entry);
        tet_mesh_needs_update = true;
        return true;
    } else {
        return false;
    }
}

bool LightProbeCollection::DeleteProbe(int id) {
    for(int i=0, len=light_probes.size(); i<len; ++i){
        if(light_probes[i].id == id) {
            light_probes.erase(light_probes.begin() + i);
            tet_mesh_needs_update = true;
            return true;
        }
    }
    return false;
}

void LightProbeCollection::Init() {
	if(cube_map_fbo == INVALID_FRAMEBUFFER){
		Graphics::Instance()->genFramebuffers(&cube_map_fbo, "light_probe_cube_map");
	}
	if(!cube_map.valid()){
		cube_map = Textures::Instance()->makeCubemapTexture(128, 128, GL_RGBA16F, GL_RGBA, Textures::MIPMAPS);
	}
	Textures::Instance()->SetTextureName(cube_map, "Light Probe Collection Cubemap");
	probe_model_id = Models::Instance()->loadModel(HardcodedPaths::paths[HardcodedPaths::kLightProbe]);
    if(!light_probes.empty()){
        tet_mesh_needs_update = true;
    }
    light_probe_texture_buffer_id = -1;
    light_probe_buffer_object_id = -1;
    grid_texture_buffer_id = -1;
    grid_buffer_object_id = -1;
}

void LightProbeCollection::Dispose() {
    Graphics::Instance()->deleteFramebuffer(&cube_map_fbo);
    cube_map_fbo = INVALID_FRAMEBUFFER;
    cube_map.clear();
    ambient_3d_tex.clear();
    light_probes.clear();
}

int LightProbeCollection::ShaderNumLightProbes()
{
    return probe_lighting_enabled ? light_probes.size() : 0;
}

int LightProbeCollection::ShaderNumTetrahedra()
{
    return probe_lighting_enabled ? tet_mesh.points.size()/4 : 0;
}

LightProbe* LightProbeCollection::GetNextProbeToProcess() {
    if(!to_process.empty()){
        LightProbeUpdateEntry entry = to_process.front();
        int id = entry.id;
        to_process.pop();
        LightProbe* probe = GetProbeFromID(id);
        if(probe){
            return probe;
        }
    }
    return NULL;
}

LightProbe* LightProbeCollection::GetProbeFromID(int id) {
    for(auto & light_probe : light_probes){
        if(light_probe.id == id) {
            return &light_probe;
        }
    }
    return NULL;
}

float GetTetrahedronVolume(vec3 points[4]){
    glm::mat4 tet_volume_mat;
    for(int column=0; column<4; ++column){
        tet_volume_mat[column] = glm::vec4(points[column][0], points[column][1], points[column][2], 1.0f);
    }
    float volume = fabs(glm::determinant(tet_volume_mat) / 6.0f);
    return volume;
}

void LightProbeCollection::UpdateTetMesh(BulletWorld& bw) {
    PROFILER_ZONE(g_profiler_ctx, "LightProbeCollection::UpdateTetMesh");
    tet_mesh_needs_update = false;
    // Clear existing tet mesh
    tet_mesh.points.clear();
    tet_mesh.point_id.clear();
    if(tet_mesh.display_id != -1){
        DebugDraw::Instance()->Remove(tet_mesh.display_id);
    }
    // Don't bother if we don't even have four probes
    if(light_probes.size() < 4) {
        return;
    }

    //Following block is commented out, disabling light probes, to disconnect tetgen from the project
//    // Prepare tetgen
//    tetgenio in, out;
//    std::vector<REAL> points;
//    std::vector<REAL> point_attributes;
//    points.resize(light_probes.size()*3);
//    for(int tet=0, index=0, len=light_probes.size(); tet<len; ++tet, index+=3){
//        // Quantize points how they will be bit-packed later for shaders
//        for(int j=0; j<3; ++j){
//            unsigned encoded = (unsigned)(light_probes[tet].pos[j] * kQuantize + 32767.5f);
//            float decoded = (encoded - 32767.5f) / kQuantize;
//            points[index+j] = (REAL)decoded;
//        }
//        // Add point id
//        point_attributes.push_back((REAL)((float)tet+0.5f));
//    }
//    in.pointlist = &points[0];
//    in.numberofpoints = light_probes.size();
//    in.numberofpointattributes = 1;
//    in.pointattributelist = &point_attributes[0];
//
//    bool success = true;
//    tetgenbehavior behavior;
//    behavior.quiet = true;
//    behavior.neighout = true; // We want to preserve neighbor info so we don't have to recalculate that
//    try {
//        tetrahedralize(&behavior, &in, &out);
//    } catch(int) {
//        success = false;
//    }
//    in.initialize();
//    if(success && out.numberofpoints > 0){
//        tet_mesh.points.resize(out.numberofpoints);
//        tet_mesh.tet_points.resize(out.numberoftetrahedra*4);
//        tet_mesh.neighbors.resize(out.numberoftetrahedra*4);
//
//        for(int i=0, index=0, len=out.numberofpoints; 
//            i<len; 
//            ++i, index+=3)
//        {
//            for(int j=0; j<3; ++j){
//                tet_mesh.points[i][j] = (float)out.pointlist[index+j];
//            }
//            tet_mesh.point_id.push_back((int)out.pointattributelist[i]);
//        }
//
//        for(int i=0, len=out.numberoftetrahedra*4; i<len; ++i) {
//            tet_mesh.tet_points[i] = out.tetrahedronlist[i];
//        }
//
//        for(int i=0, len=out.numberoftetrahedra*4; i<len; ++i) {
//            tet_mesh.neighbors[i] = out.neighborlist[i];
//        }
//        
//        if(tet_mesh_viz_enabled) {
//            std::vector<GLfloat> line_verts;
//            std::vector<GLuint> line_indices;
//
//            for(int i=0, index=0, len=tet_mesh.tet_points.size()/4; 
//                i<len; 
//                ++i, index+=4)
//            {
//                for(int j=0; j<6; ++j){
//                    int points[2];
//                    const int *tet_points = &tet_mesh.tet_points[index];
//                    switch(j){
//                    case 0: points[0] = tet_points[0]; points[1] = tet_points[1]; break;
//                    case 1: points[0] = tet_points[0]; points[1] = tet_points[2]; break;
//                    case 2: points[0] = tet_points[0]; points[1] = tet_points[3]; break;
//                    case 3: points[0] = tet_points[1]; points[1] = tet_points[2]; break;
//                    case 4: points[0] = tet_points[1]; points[1] = tet_points[3]; break;
//                    case 5: points[0] = tet_points[2]; points[1] = tet_points[3]; break;
//                    }
//                    //if(!light_probes[tet_mesh.point_id[points[0]]].negative && 
//                    //   !light_probes[tet_mesh.point_id[points[1]]].negative) 
//                    //{
//                        for(int i=0; i<2; ++i){
//                            vec3 pos = light_probes[tet_mesh.point_id[points[i]]].pos;
//                            line_indices.push_back(line_verts.size()/3);
//                            for(int j=0; j<3; ++j){
//                                unsigned encoded = (unsigned)(pos[j] * kQuantize + 32767.5f);
//                                float decoded = (encoded - 32767.5f) / kQuantize;
//                                line_verts.push_back(decoded);
//                            }
//                        }
//                    //}
//                }
//            }
//            tet_mesh.display_id = DebugDraw::Instance()->AddLines(line_verts, line_indices, vec4(vec3(1.0f), 0.2f), _persistent, _DD_NO_FLAG);
//        }
//    }
//
//    // Update grid lookup
//    grid_lookup.bounds[0] = vec3(FLT_MAX);
//    grid_lookup.bounds[1] = vec3(-FLT_MAX);
//    for(int tet=0, len=tet_mesh.points.size(); tet<len; ++tet){
//        for(int j=0; j<3; ++j){
//            grid_lookup.bounds[0][j] = min(grid_lookup.bounds[0][j], tet_mesh.points[tet][j]);
//            grid_lookup.bounds[1][j] = max(grid_lookup.bounds[1][j], tet_mesh.points[tet][j]);
//        }
//    }
//    static const float kCellSize = 4.0f; // Target dimensions of each grid cell
//    for(int tet=0; tet<3; ++tet){
//        grid_lookup.subdivisions[tet] = (int)ceilf((grid_lookup.bounds[1][tet] - grid_lookup.bounds[0][tet]) / kCellSize);
//    }
//    int total_cells = grid_lookup.subdivisions[0] * grid_lookup.subdivisions[1] * grid_lookup.subdivisions[2];
//    if(total_cells > GridLookup::kMaxGridCells){
//        float scale = 0.99f;
//        while((int)ceilf(grid_lookup.subdivisions[0]*scale) * 
//              (int)ceilf(grid_lookup.subdivisions[1]*scale) * 
//              (int)ceilf(grid_lookup.subdivisions[2]*scale) > GridLookup::kMaxGridCells)
//        {
//            scale -= 0.01f;
//        }
//        for(int i=0; i<3; ++i){
//            grid_lookup.subdivisions[i] = (int)ceilf(grid_lookup.subdivisions[i]*scale);
//        }
//    }
//    const bool kDebugGridInfo = false;
//    if(kDebugGridInfo){
//        LOGI << "Grid bounds: " << "(" << grid_lookup.bounds[0][0] << " " << grid_lookup.bounds[0][1] << " " << grid_lookup.bounds[0][2] << ")"
//                                                       << " (" << grid_lookup.bounds[1][0] << " " <<  grid_lookup.bounds[1][1] << " " << grid_lookup.bounds[1][2] << ")" << std::endl;
//        LOGI << "Grid subdivisions: (" << grid_lookup.subdivisions[0] << " " << grid_lookup.subdivisions[1] << " " << grid_lookup.subdivisions[2] << ")" << std::endl;
//    }
//    grid_lookup.cell_tet.resize(grid_lookup.subdivisions[0] * grid_lookup.subdivisions[1] * grid_lookup.subdivisions[2], UINT_MAX);
//    // Go through each tetrahedron and tag overlapping cells
//    for(int tet=0, index=0, len=tet_mesh.tet_points.size()/4; 
//        tet<len; 
//        ++tet, index+=4)
//    {
//        const int *tet_points = &tet_mesh.tet_points[index];
//        // Get world-space bounds of tetrahedron
//        vec3 ws_bounds[2] = {vec3(FLT_MAX), vec3(-FLT_MAX)};
//        for(int vert=0; vert<4; ++vert){
//            for(int axis=0; axis<3; ++axis){
//                ws_bounds[0][axis] = min(ws_bounds[0][axis], tet_mesh.points[tet_points[vert]][axis]);
//                ws_bounds[1][axis] = max(ws_bounds[1][axis], tet_mesh.points[tet_points[vert]][axis]);
//            }
//        }
//        // Get grid-space bounds
//        int gs_bounds[2][3];
//        for(int corner=0; corner<2; ++corner){
//            for(int axis=0; axis<3; ++axis){
//                gs_bounds[corner][axis] = (int)((ws_bounds[corner][axis] - grid_lookup.bounds[0][axis]) / (grid_lookup.bounds[1][axis] - grid_lookup.bounds[0][axis]) * (float)grid_lookup.subdivisions[axis]);
//                gs_bounds[corner][axis] = min(gs_bounds[corner][axis], grid_lookup.subdivisions[axis]-1);
//            }
//        }
//        // Get tetrahedron planes for later use
//        vec4 tet_planes[4];
//        for(int face=0; face<4; ++face){
//            int vert_ids[3];
//            switch(face) {
//                case 0: vert_ids[0] = 0; vert_ids[1] = 1; vert_ids[2] = 2; break;
//                case 1: vert_ids[0] = 0; vert_ids[1] = 2; vert_ids[2] = 3; break;
//                case 2: vert_ids[0] = 0; vert_ids[1] = 3; vert_ids[2] = 1; break;
//                case 3: vert_ids[0] = 1; vert_ids[1] = 3; vert_ids[2] = 2; break;
//            }
//            vec3 verts[3];
//            for(int i=0; i<3; ++i){
//                verts[i] = tet_mesh.points[tet_points[vert_ids[i]]];
//            }
//            vec3 normal = normalize(cross(verts[1] - verts[0], verts[2] - verts[0]));
//            float d = dot(normal, verts[0]);
//            tet_planes[face] = vec4(normal, d);
//        }
//        // TODO: use the tetrahedron info to calculate the volume that overlaps each grid cell
//        vec3 tet_verts[4];
//        for(int vert=0; vert<4; ++vert){
//            tet_verts[vert] = tet_mesh.points[tet_points[vert]];
//        }
//        //float volume = GetTetrahedronVolume(tet_verts);
//
//        const bool kCheckMidpointIsInTet = true;
//        if(kCheckMidpointIsInTet){ // This is a sanity check -- if the midpoint of the tet is not inside all the planes, something is weird
//            vec3 mid_point(0.0f);
//            for(int vert=0; vert<4; ++vert){
//                mid_point += tet_mesh.points[tet_points[vert]] * 0.25f;
//            }
//            for(int face=0; face<4; ++face){
//                float dot_val = dot(mid_point, tet_planes[face].xyz());
//                LOG_ASSERT(dot_val > tet_planes[face][3]);
//            }
//        }
//
//        vec3 grid_dims = grid_lookup.bounds[1] - grid_lookup.bounds[0];
//        vec3 grid_cell_dims;
//        for(int axis=0; axis<3; ++axis){
//            grid_cell_dims[axis] = grid_dims[axis] / (float)grid_lookup.subdivisions[axis];
//        }
//        btBoxShape* box_shape = new btBoxShape(btVector3(grid_cell_dims[0]*0.5f,
//                                                         grid_cell_dims[1]*0.5f,
//                                                         grid_cell_dims[2]*0.5f));
//        btCollisionObject aabb_col_obj;
//        aabb_col_obj.setCollisionShape(box_shape);
//        btTransform aabb_transform;
//        aabb_transform.setIdentity(); 
//
//        btCollisionObject tet_col_obj;
//
//        // Tag each of these grid cells with the current tetrahedron
//        for(int x_cell = gs_bounds[0][0]; x_cell <= gs_bounds[1][0]; ++x_cell){
//            for(int y_cell = gs_bounds[0][1]; y_cell <= gs_bounds[1][1]; ++y_cell){
//                for(int z_cell = gs_bounds[0][2]; z_cell <= gs_bounds[1][2]; ++z_cell){
//                    /*aabb_transform.setOrigin(btVector3(
//                        grid_lookup.bounds[0][0] + (x_cell + 0.5f) * grid_cell_dims[0],
//                        grid_lookup.bounds[0][1] + (y_cell + 0.5f) * grid_cell_dims[1],
//                        grid_lookup.bounds[0][2] + (z_cell + 0.5f) * grid_cell_dims[2]));
//                    aabb_col_obj.setWorldTransform(aabb_transform);
//
//                    btConvexHullShape* tet_shape = new btConvexHullShape();
//                    for(int i=0; i<4; ++i){
//                        tet_shape->addPoint(btVector3(tet_verts[i][0],tet_verts[i][1],tet_verts[i][2]));
//                    }
//                    tet_col_obj.setCollisionShape(tet_shape);
//
//                    ContactInfoCallback cb;
//                    bw.GetPairCollisions(aabb_col_obj, tet_col_obj, cb);
//
//                    delete tet_shape;
//
//                    if(cb.contact_info.size() != 0){*/
//                        // TODO: eventually only label the tet with most volume in cell
//                        int cell_id = ((x_cell * grid_lookup.subdivisions[1]) + y_cell)*grid_lookup.subdivisions[2] + z_cell;
//                        grid_lookup.cell_tet[cell_id] = tet;
//                    //}
//                }
//            }
//        }
//
//        delete box_shape; 
//    }
}

//The following is adapted from http://dennis2society.de/main/painless-tetrahedral-barycentric-mapping
namespace {
 
/**
 * Calculate the determinant for a 4x4 matrix based on this example:
 * http://www.euclideanspace.com/maths/algebra/matrix/functions/determinant/fourD/index.htm
 * This function takes four Vec4f as row vectors and calculates the resulting matrix' determinant
 * using the Laplace expansion.
 *
 */
const float Determinant4x4( const vec4& v0,
                            const vec4& v1,
                            const vec4& v2,
                            const vec4& v3 )
{
    float det = v0[3]*v1[2]*v2[1]*v3[0] - v0[2]*v1[3]*v2[1]*v3[0] -
                v0[3]*v1[1]*v2[2]*v3[0] + v0[1]*v1[3]*v2[2]*v3[0] +

                v0[2]*v1[1]*v2[3]*v3[0] - v0[1]*v1[2]*v2[3]*v3[0] -
                v0[3]*v1[2]*v2[0]*v3[1] + v0[2]*v1[3]*v2[0]*v3[1] +

                v0[3]*v1[0]*v2[2]*v3[1] - v0[0]*v1[3]*v2[2]*v3[1] -
                v0[2]*v1[0]*v2[3]*v3[1] + v0[0]*v1[2]*v2[3]*v3[1] +

                v0[3]*v1[1]*v2[0]*v3[2] - v0[1]*v1[3]*v2[0]*v3[2] -
                v0[3]*v1[0]*v2[1]*v3[2] + v0[0]*v1[3]*v2[1]*v3[2] +

                v0[1]*v1[0]*v2[3]*v3[2] - v0[0]*v1[1]*v2[3]*v3[2] -
                v0[2]*v1[1]*v2[0]*v3[3] + v0[1]*v1[2]*v2[0]*v3[3] +

                v0[2]*v1[0]*v2[1]*v3[3] - v0[0]*v1[2]*v2[1]*v3[3] -
                v0[1]*v1[0]*v2[2]*v3[3] + v0[0]*v1[1]*v2[2]*v3[3];
    return det;
}
 
/**
 * Calculate the actual barycentric coordinate from a point p0_ and the four 
 * vertices v0_ .. v3_ from a tetrahedron.
 */
const vec4 GetBarycentricCoordinate( const vec3& v0_,
                                     const vec3& v1_,
                                     const vec3& v2_,
                                     const vec3& v3_,
                                     const vec3& p0_)
{
    vec4 v0(v0_, 1.0f);
    vec4 v1(v1_, 1.0f);
    vec4 v2(v2_, 1.0f);
    vec4 v3(v3_, 1.0f);
    vec4 p0(p0_, 1.0f);
    vec4 barycentricCoord;
    const float det0 = Determinant4x4(v0, v1, v2, v3);
    const float det1 = Determinant4x4(p0, v1, v2, v3);
    const float det2 = Determinant4x4(v0, p0, v2, v3);
    const float det3 = Determinant4x4(v0, v1, p0, v3);
    const float det4 = Determinant4x4(v0, v1, v2, p0);
    barycentricCoord[0] = (det1/det0);
    barycentricCoord[1] = (det2/det0);
    barycentricCoord[2] = (det3/det0);
    barycentricCoord[3] = (det4/det0);
    return barycentricCoord;
}

} // namespace ""

static unsigned LeftShift(unsigned val, int amount) {
    if(amount >-32 && amount < 32){
        if(amount >= 0){
            return val << amount; 
        } else {
            return val >> -amount;
        }
    } else {
        return 0;
    }
}

void LightProbeCollection::UpdateTextureBuffer(BulletWorld& bw) {
    PROFILER_ZONE(g_profiler_ctx, "LightProbeCollection::UpdateTextureBuffer");
    if(tet_mesh_needs_update){
        UpdateTetMesh(bw);
    }

    const int kMaxTextureBufferSize = 4 * 1024 * 1024;
    // Create texture buffer object if it is not already created
    if(light_probe_texture_buffer_id == -1){
        PROFILER_ZONE(g_profiler_ctx, "Create light probe texture buffer");
        int max_size;
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_size);
        LOG_ASSERT(max_size >= kMaxTextureBufferSize);
        GLuint buffer_object;
        glGenBuffers(1, &buffer_object);
        light_probe_buffer_object_id = buffer_object;
        glBindBuffer(GL_TEXTURE_BUFFER, light_probe_buffer_object_id);
        char* buf = new char[kMaxTextureBufferSize];
        float *buf_f = (float*)buf;
        // Start by filling buffer with noise
        for(int i=0, len=kMaxTextureBufferSize/sizeof(float); i<len; ++i){
            buf_f[i] = RangedRandomFloat(0.0f, 1.0f);
        }
        glBufferData(GL_TEXTURE_BUFFER, kMaxTextureBufferSize, buf, GL_DYNAMIC_DRAW);
        delete[] buf;
        GLuint tex;
        glGenTextures(1, &tex);
        light_probe_texture_buffer_id = tex;
        glActiveTexture(GL_TEXTURE0 + TEX_AMBIENT_COLOR_BUFFER);
        glBindTexture(GL_TEXTURE_BUFFER, light_probe_texture_buffer_id);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, light_probe_buffer_object_id);
    }
    LOG_ASSERT(light_probe_texture_buffer_id != -1);
    if(tet_mesh.point_id.size() >= 4){
        // Calculate colors of tet corners, flood-filling from positive probes to negative probes
        // TODO: allocate this using stack allocator for speed
        std::vector<unsigned char> tet_colors(tet_mesh.tet_points.size() * 6 * 3);
        {
            PROFILER_ZONE(g_profiler_ctx, "Flood fill tet mesh corners");
            // Start by filling in positive probes
            for(int i=0, len=tet_mesh.tet_points.size(); 
                i<len; 
                i+=4)
            {
                int point_id[4];
                for(int j=0; j<4; ++j){
                    point_id[j] = tet_mesh.point_id[tet_mesh.tet_points[i+j]];
                }
                for(int point=0; point<4; ++point){
                    int index = (i+point)*18;
                    LightProbe& probe = light_probes[point_id[point]];
                    if(probe.negative){
                        for(int face=0; face<6; ++face){
                            tet_colors[index] = 255;
                            ++index;
                            tet_colors[index] = 0;
                            ++index;
                            tet_colors[index] = 255;
                            ++index;
                        }
                    } else {
                        for(auto & face : probe.ambient_cube_color){
                            for(int channel=0; channel<3; ++channel){
                                unsigned val = (unsigned)(face[channel] * 255.0f / 4.0f);
                                tet_colors[index] = (val > 255)?255:val;
                                ++index;
                            }
                        }
                    }
                }
            }
            // Next fill in negatives that share tet with positives
            for(int i=0, len=tet_mesh.tet_points.size(); 
                i<len; 
                i+=4)
            {
                int num_negative = 0;
                int num_positive = 0;
                int point_id[4];
                for(int j=0; j<4; ++j){
                    point_id[j] = tet_mesh.point_id[tet_mesh.tet_points[i+j]];
                    LightProbe& probe = light_probes[point_id[j]];
                    if(probe.negative){
                        ++num_negative;
                    } else {
                        ++num_positive;
                    }
                }
                if(num_negative && num_positive){
                    // Find average color of positive probes in this tet
                    int avg_col[18] = {0};
                    for(int point=0; point<4; ++point){
                        int index = (i+point)*18;
                        LightProbe& probe = light_probes[point_id[point]];
                        if(!probe.negative){
                            for(int avg_index=0; avg_index<18; ++avg_index){
                                avg_col[avg_index] += tet_colors[index+avg_index];
                            }
                        }
                    }
                    for(int & avg_index : avg_col){
                        avg_index /= num_positive;
                    }
                    // Assign average color to negative probes in this tet
                    for(int point=0; point<4; ++point){
                        int index = (i+point)*18;
                        LightProbe& probe = light_probes[point_id[point]];
                        if(probe.negative){
                            for(int avg_index=0; avg_index<18; ++avg_index){
                                tet_colors[index+avg_index] = avg_col[avg_index]; 
                            }
                        }
                    }
                }
            }
        }
        PROFILER_ZONE(g_profiler_ctx, "Fill tet mesh buffers");
        // We can load 128-bits at a time from the shader
        // Each tetrahedron contains:
        //     16 bits * 4 points * 3 axes = 192 bits for coords ~ 2 * 128
        //     32 bits * 4 uints = 128 bits for neighbor ids ~ 1 * 128
        //     8 bits * 3 channels * 6 cube faces = 576 bits ~ 5 * 128
        //     total ~ 8 * 128
        glBindBuffer(GL_TEXTURE_BUFFER, light_probe_buffer_object_id);
        int space = 16 * tet_mesh.tet_points.size() * 8;
        char* buf = new char[space];
        unsigned *buf_u = (unsigned*)buf;
        int buf_index = 0;

        for(int i=0, len=tet_mesh.tet_points.size(); 
            i<len; 
            i+=4)
        {
            int point_id[4];
            for(int j=0; j<4; ++j){
                point_id[j] = tet_mesh.point_id[tet_mesh.tet_points[i+j]];
            }

            vec3 pos[4];
            for(int j=0; j<4; ++j){
                pos[j] = light_probes[point_id[j]].pos;
            }

            // Store coordinates of each point as 16-bit unsigned ints
            unsigned points[12];
            for(int j=0; j<12; ++j){
                int axis = j%3;
                int point = j/3;
                points[j] = (unsigned)(pos[point][axis] * kQuantize + 32767.5f);
                LOG_ASSERT(points[j] == (points[j] & 0xFFFF)); // Make sure coord fits in 16 bits
            }
            // Pack point values together
            unsigned point_coord_u[6];
            for(int j=0; j<6; ++j){
                point_coord_u[j] = (points[j*2] << 16) + points[j*2+1];
            }
            // Pack neighbors
            unsigned neighbors_u[4];
            for(int j=0; j<4; ++j) {
                neighbors_u[j] = (unsigned)(tet_mesh.neighbors[i+j]);
            }
            // Store negative info
            unsigned negative = 0;
            for(int j=0; j<4; ++j){
                negative += (light_probes[point_id[j]].negative?0:1) << j;
            }
            // Pack colors
            unsigned colors_u[18] = {0};
            int offset = 0;
            for(int point=0; point<4; ++point){
                int index = (i+point)*18;
                for(int face=0; face<6; ++face){
                    for(int channel=0; channel<3; ++channel){
                        unsigned val = tet_colors[index];
                        colors_u[offset/32] |= LeftShift(val, (24-offset%32));
                        offset += 8;
                        ++index;
                    }

                }
            }
            buf_u[buf_index++] = point_coord_u[0];
            buf_u[buf_index++] = point_coord_u[1];
            buf_u[buf_index++] = point_coord_u[2];
            buf_u[buf_index++] = point_coord_u[3];

            buf_u[buf_index++] = point_coord_u[4];
            buf_u[buf_index++] = point_coord_u[5];
            buf_u[buf_index++] = negative;
            buf_u[buf_index++] = 0;

            buf_u[buf_index++] = neighbors_u[0];
            buf_u[buf_index++] = neighbors_u[1];
            buf_u[buf_index++] = neighbors_u[2];
            buf_u[buf_index++] = neighbors_u[3];

            for(unsigned int i : colors_u){
                buf_u[buf_index++] = i;
            }
            buf_u[buf_index++] = 0;
            buf_u[buf_index++] = 0;
        }

        glBufferData(GL_TEXTURE_BUFFER, kMaxTextureBufferSize, NULL, GL_DYNAMIC_DRAW); // Orphan buffer?

        void* mapped = glMapBufferRange(GL_TEXTURE_BUFFER, 0, space, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
        memcpy(mapped, buf, space);
        glUnmapBuffer(GL_TEXTURE_BUFFER);

        delete[] buf;
    }
    if(grid_texture_buffer_id == -1 && !grid_lookup.cell_tet.empty()){
        PROFILER_ZONE(g_profiler_ctx, "Generate grid texture buffer");
        int max_size;
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_size);
        LOG_ASSERT(max_size >= kMaxTextureBufferSize);
        GLuint buffer_object;
        glGenBuffers(1, &buffer_object);
        grid_buffer_object_id = buffer_object;
        glBindBuffer(GL_TEXTURE_BUFFER, grid_buffer_object_id);
        char* buf = new char[kMaxTextureBufferSize];
        float *buf_f = (float*)buf;
        for(int i=0, len=kMaxTextureBufferSize/sizeof(float); i<len; ++i){
            buf_f[i] = RangedRandomFloat(0.0f, 1.0f);
        }
        glBufferData(GL_TEXTURE_BUFFER, kMaxTextureBufferSize, buf, GL_DYNAMIC_DRAW);
        delete[] buf;
        GLuint tex;
        glGenTextures(1, &tex);
        grid_texture_buffer_id = tex;
        glActiveTexture(GL_TEXTURE0 + TEX_AMBIENT_GRID_DATA);
        glBindTexture(GL_TEXTURE_BUFFER, grid_texture_buffer_id);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, grid_buffer_object_id);
    }
    if(grid_texture_buffer_id != -1 && !grid_lookup.cell_tet.empty()){
        PROFILER_ZONE(g_profiler_ctx, "Update grid texture buffer");
        glBindBuffer(GL_TEXTURE_BUFFER, grid_buffer_object_id);
        int space = 4 * grid_lookup.cell_tet.size();
        char* buf = new char[space];
        unsigned *buf_u = (unsigned*)buf;
        //int buf_index = 0;

        for(int i=0, len=grid_lookup.cell_tet.size(); 
            i<len; 
            ++i)
        {
            buf_u[i] = grid_lookup.cell_tet[i];
        }

        glBufferData(GL_TEXTURE_BUFFER, kMaxTextureBufferSize, NULL, GL_DYNAMIC_DRAW); // Orphan buffer?

        void* mapped = glMapBufferRange(GL_TEXTURE_BUFFER, 0, space, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
        memcpy(mapped, buf, space);
        glUnmapBuffer(GL_TEXTURE_BUFFER);

        delete[] buf;
    }
}

int LightProbeCollection::GetTetrahedron(const vec3& position, vec3 ambient_cube_color[], int best_guess) {
    std::set<int> visited_nodes;
    if(!tet_mesh.points.empty()){
        int prev_to_check = -1;
        int prev_prev_to_check = -1;
        int to_check = best_guess;

        if(to_check == -1){
            if(position[0] > grid_lookup.bounds[0][0] &&
               position[1] > grid_lookup.bounds[0][1] &&
               position[2] > grid_lookup.bounds[0][2] &&
               position[0] < grid_lookup.bounds[1][0] &&
               position[1] < grid_lookup.bounds[1][1] &&
               position[2] < grid_lookup.bounds[1][2])
            {
                vec3 grid_coord;
                grid_coord[0] = (float) int((position[0] - grid_lookup.bounds[0][0]) / (grid_lookup.bounds[1][0] - grid_lookup.bounds[0][0]) * float(grid_lookup.subdivisions[0]));
                grid_coord[1] = (float) int((position[1] - grid_lookup.bounds[0][1]) / (grid_lookup.bounds[1][1] - grid_lookup.bounds[0][1]) * float(grid_lookup.subdivisions[1]));
                grid_coord[2] = (float) int((position[2] - grid_lookup.bounds[0][2]) / (grid_lookup.bounds[1][2] - grid_lookup.bounds[0][2]) * float(grid_lookup.subdivisions[2]));
                int cell_id = (int) (((grid_coord[0] * grid_lookup.subdivisions[1]) + grid_coord[1])*grid_lookup.subdivisions[2] + grid_coord[2]);
                to_check = grid_lookup.cell_tet[cell_id];
                if(to_check == -1){
                    to_check = 0;
                }
            } else {
                to_check = 0;
            }
        }

        bool success = false;
        while(!success){
            vec3 points[4];    
            for(int j=0, index = to_check*4; j<4; ++j){
                points[j] = tet_mesh.points[tet_mesh.tet_points[index+j]];
            }
            vec4 bary_coords = GetBarycentricCoordinate(points[0], points[1], points[2], points[3], position);
            float lowest_val = FLT_MAX;
            int lowest_id = -1;
            for(int i=0; i<4; ++i){
                if(bary_coords[i] < lowest_val){
                    lowest_val = bary_coords[i];
                    lowest_id = i;
                }
            }
            if(lowest_val >= 0.0f){
                success = true;
            } else {
                visited_nodes.insert(to_check);

                to_check = tet_mesh.neighbors[to_check*4+lowest_id];
    
                //We've already been here before.
                if( visited_nodes.find(to_check) != visited_nodes.end() )
                {
                    return to_check;
                    //LOGE << "Returned to node we have already visited, exit execution" << std::endl;
                    //return -1;
                }
            }

            if(to_check == -1){
                return -1;
            }

            if(success || prev_to_check == to_check || prev_prev_to_check == to_check){
                vec3 ambient_cube[24];
                vec4 modified_bary_coords = bary_coords;
                float total_modified_bary_coords = FLT_MIN;
                for(int j=0, index=0; j<4; ++j, index+=6){
                    LightProbe* probe = &light_probes[tet_mesh.point_id[tet_mesh.tet_points[to_check*4+j]]];
                    if(probe) {
                        for(int k=0; k<6; ++k){
                            ambient_cube[index+k] = probe->ambient_cube_color[k];
                        }
                        if(probe->negative){
                            modified_bary_coords[j] = min(modified_bary_coords[j], FLT_MIN);
                        }
                        total_modified_bary_coords += modified_bary_coords[j];
                    }
                }
                for(int j=0; j<4; ++j){
                    modified_bary_coords[j] /= total_modified_bary_coords;
                }

                for(int j=0; j<6; ++j){
                    ambient_cube_color[j] = ambient_cube[0+j]  * modified_bary_coords[0] + 
                                            ambient_cube[6+j]  * modified_bary_coords[1] + 
                                            ambient_cube[12+j] * modified_bary_coords[2] + 
                                            ambient_cube[18+j] * modified_bary_coords[3];
                }
                return to_check;
            }

            prev_prev_to_check = prev_to_check;
            prev_to_check = to_check;
        }
    }
    return -1;
}

void LightProbeCollection::Draw(BulletWorld& bw) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Draw light probe collection");
    Shaders* shaders = Shaders::Instance();
    Graphics* graphics = Graphics::Instance();

    if(show_probes){
        for(int i=0; i<2; ++i){
            GLState gl_state;
            gl_state.blend = false;
            gl_state.cull_face = true;
            gl_state.depth_test = true;
            gl_state.depth_write = true;

            if(i==0 && !show_probes_through_walls){
                continue;
            }
            if(i==0 && show_probes_through_walls){
                gl_state.depth_test = false;
                gl_state.depth_write = false;
            }

            graphics->setGLState(gl_state);

            int shader_id = shaders->returnProgram("lightprobe");
            if(i==0 && show_probes_through_walls){
                shader_id = shaders->returnProgram("lightprobe #STIPPLE");
            }
            shaders->setProgram(shader_id);

            int programHandle = shaders->programs[shader_id].gl_program;
            GLuint blockIndex = glGetUniformBlockIndex(programHandle, "LightProbeInfo");

            const GLchar *names[] = {
                "center[0]",
                "view_mat",
                "proj_mat",
                "cam_pos",
                "ambient_cube_color[0]",
                // These long names were necessary on a Mac OS 10.7 ATI card
                "LightProbeInfo.center[0]",
                "LightProbeInfo.view_mat",
                "LightProbeInfo.proj_mat",
                "LightProbeInfo.cam_pos",
                "LightProbeInfo.ambient_cube_color[0]"
            };

            GLuint indices[5] = {0};
            glGetUniformIndices(programHandle, 5, names, indices);
            CHECK_GL_ERROR();

            // Check long name if short name is not found
            for(unsigned int indice : indices){
                if(indice == GL_INVALID_INDEX){
                    glGetUniformIndices(programHandle, 5, &names[5], indices);
                    break;
                }
            }

            for(unsigned int indice : indices){
                if(indice == GL_INVALID_INDEX){
                    return;
                }
            }

            GLint offset[5] = {-1};
            glGetActiveUniformsiv(programHandle, 5, indices, GL_UNIFORM_OFFSET, offset);

            GLint blockSize;
            glGetActiveUniformBlockiv(programHandle, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
            GLubyte * blockBuffer= (GLubyte *)OG_MALLOC(blockSize);

            vec4* center = (vec4*)((uintptr_t)blockBuffer + offset[0]);
            mat4* view_mat = (mat4*)((uintptr_t)blockBuffer + offset[1]);
            mat4* proj_mat = (mat4*)((uintptr_t)blockBuffer + offset[2]);
            vec3* cam_pos = (vec3*)((uintptr_t)blockBuffer + offset[3]);
            vec4* ambient_cube_color_vec = (vec4*)((uintptr_t)blockBuffer + offset[4]);
            static int ubo_id = -1;
            if(ubo_id == -1){
                GLuint uboHandle;
                glGenBuffers( 1, &uboHandle );
                ubo_id = uboHandle;
                glBindBuffer( GL_UNIFORM_BUFFER, ubo_id );
                glBufferData( GL_UNIFORM_BUFFER, blockSize, blockBuffer, GL_DYNAMIC_DRAW );
            }
            glBindBuffer( GL_UNIFORM_BUFFER, ubo_id );
            glBindBufferBase( GL_UNIFORM_BUFFER, blockIndex, ubo_id );

            Camera* camera = ActiveCameras::Get();
            *cam_pos = camera->GetPos();
            *view_mat = camera->GetViewMatrix();
            *proj_mat = camera->GetProjMatrix();

            int vert_attrib_id = shaders->returnShaderAttrib("vertex", shader_id);
			Model* probe_model = &Models::Instance()->GetModel(probe_model_id);
			if(!probe_model->vbo_loaded){
				probe_model->createVBO();
			}
            probe_model->VBO_vertices.Bind();
            probe_model->VBO_faces.Bind();
            graphics->EnableVertexAttribArray(vert_attrib_id);
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3 * sizeof(GL_FLOAT), 0);
            const int kBatchSize = 128; // Max that fits into 16k UBO
            for(int i=0, len=light_probes.size(); i<len; i+=kBatchSize){
                int num_to_render = std::min(kBatchSize, (int)light_probes.size() - i);
                for(int j=0; j<num_to_render; ++j){
                    center[j] = vec4(light_probes[i+j].pos, 0.0f);
                    if(light_probes[i+j].negative){
                        for(int k=0; k<6; ++k){
                            ambient_cube_color_vec[j*6+k] = vec3(0.0f, 0.0f, 1.0f);
                        }
                    } else {
                        for(int k=0; k<6; ++k){
                            ambient_cube_color_vec[j*6+k] = light_probes[i+j].ambient_cube_color[k];
                        }
                    }
                }
                glBindBuffer( GL_UNIFORM_BUFFER, ubo_id );
                glBufferData( GL_UNIFORM_BUFFER, blockSize, NULL, GL_DYNAMIC_DRAW ); // orphan buffer

                void* mapped = glMapBufferRange(GL_UNIFORM_BUFFER, 0, blockSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
                memcpy(mapped, blockBuffer, blockSize);
                glUnmapBuffer(GL_UNIFORM_BUFFER);

                graphics->DrawElementsInstanced(GL_TRIANGLES, probe_model->faces.size(), GL_UNSIGNED_INT, 0, num_to_render);
            }

            OG_FREE(blockBuffer);
            graphics->ResetVertexAttribArrays();
        }
    }

    if(tet_mesh_needs_update){
        UpdateTetMesh(bw);
    }
}
