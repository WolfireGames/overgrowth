//-----------------------------------------------------------------------------
//           Name: navmesh.cpp
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
#include "navmesh.h"

#include <Graphics/graphics.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>

#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>

#include <Math/vec3math.h>
#include <Math/vec2math.h>
#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <XML/xml_helper.h>
#include <XML/level_loader.h>

#include <Utility/commonregex.h>
#include <Utility/hash.h>
#include <Utility/strings.h>

#include <Internal/profiler.h>
#include <Internal/path.h>
#include <Internal/config.h>
#include <Internal/zip_util.h>

#include <Asset/Asset/filehash.h>
#include <Version/version.h>
#include <Compat/fileio.h>
#include <UserInput/input.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>
#include <Recast.h>
#include <DetourNavMeshQuery.h>

#include <float.h>
#include <cmath>
#include <string>
#include <vector>
#include <list>
#include <iostream>

using std::endl;
using std::list;
using std::ofstream;
using std::string;
using std::swap;
using std::vector;

extern Config config;

// So, while developing, we can assume that each new version of the game _might_ involve a code change in the navmesh, it's the conservative assumption.
// static const char* NAVMESH_VERSION = "1";
//#define NAVMESH_VERSION GetBuildVersion()
#define NAVMESH_VERSION "1"

NavPoint::NavPoint(const NavPoint& other) {
    this->pos = other.pos;
    this->valid = other.valid;
}

NavPoint::NavPoint(vec3 pos) : pos(pos), valid(true) {}

NavPoint::NavPoint(vec3 pos, bool success) : pos(pos), valid(success) {}

NavPoint::NavPoint() : pos(), valid(false) {}

vec3 NavPoint::GetPoint() {
    return pos;
}

bool NavPoint::IsSuccess() {
    return valid;
}

namespace NavMeshTemp {
int compareInt(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

struct SortableVert {
    float coords[3];
    int used_id;
    int true_id;
};

inline bool rcVequal(const float* v1, const float* v2) {
    return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
}

static int compareSortableVert(const void* va, const void* vb) {
    const SortableVert* a = (const SortableVert*)va;
    const SortableVert* b = (const SortableVert*)vb;
    for (int i = 0; i < 3; ++i) {
        if (a->coords[i] < b->coords[i])
            return -1;
        if (a->coords[i] > b->coords[i])
            return 1;
    }
    return 0;
}

struct IntTuple {
    int val[2];
};

static int compareTupleFirst(const void* va, const void* vb) {
    const IntTuple* a = (const IntTuple*)va;
    const IntTuple* b = (const IntTuple*)vb;
    return a->val[0] - b->val[0];
}

static int compareTupleSecond(const void* va, const void* vb) {
    const IntTuple* a = (const IntTuple*)va;
    const IntTuple* b = (const IntTuple*)vb;
    return a->val[1] - b->val[1];
}

void MergeOverlappingVerts(const float* verts_in, int n_verts_in, const unsigned* tris_in, int n_tris_in, int* tris_out) {
    // Create an array storing the vertex indices that are referred to by the
    // triangles, and then sort it and remove duplicates to get an array of
    // used vertices. This is important when using a tile mesh, so we don't
    // have to merge all the overlapping verts in the whole scene for each
    // tile.
    int n_tri_indices = n_tris_in * 3;
    int* used_verts = new int[n_tri_indices];
    for (int i = 0; i < n_tri_indices; ++i) {
        used_verts[i] = tris_in[i];
    }

    qsort(used_verts, n_tri_indices, sizeof(int), compareInt);

    int n_used_verts = 0;
    int last_vert = -1;
    for (int i = 0; i < n_tri_indices; ++i) {
        if (used_verts[i] != last_vert) {
            used_verts[n_used_verts] = used_verts[i];
            last_vert = used_verts[i];
            ++n_used_verts;
        }
    }

    // Create an array storing the coordinates and ids of every used vertex,
    // and then sort it so duplicates are next to each other.
    SortableVert* sort_verts = new SortableVert[n_used_verts];

    for (int i = 0; i < n_used_verts; ++i) {
        rcVcopy(sort_verts[i].coords, &verts_in[used_verts[i] * 3]);
        sort_verts[i].used_id = i;
        sort_verts[i].true_id = used_verts[i];
    }

    qsort(sort_verts, n_used_verts, sizeof(SortableVert), compareSortableVert);

    // Create an array storing the id of the first of any set of duplicate
    // vertices. For example, if vertices 6 and 15 are duplicates, then
    // base_vert[6] == 6 and base_vert[15] == 6.
    int* base_vert = new int[n_used_verts];

    int num_merged = 0;
    base_vert[sort_verts[0].used_id] = sort_verts[0].true_id;
    for (int i = 1; i < n_used_verts; ++i) {
        if (rcVequal(sort_verts[i].coords, sort_verts[i - 1].coords)) {
            base_vert[sort_verts[i].used_id] = base_vert[sort_verts[i - 1].used_id];
            ++num_merged;
        } else {
            base_vert[sort_verts[i].used_id] = sort_verts[i].true_id;
        }
    }

    IntTuple* tri_remap = new IntTuple[n_tri_indices];
    for (int i = 0; i < n_tri_indices; ++i) {
        tri_remap[i].val[0] = tris_in[i];
        tri_remap[i].val[1] = i;
    }

    qsort(tri_remap, n_tri_indices, sizeof(IntTuple), compareTupleFirst);
    int a_index = 0;
    int b_index = 0;
    while (a_index < n_tri_indices && b_index < n_used_verts) {
        if (tri_remap[a_index].val[0] < used_verts[b_index]) {
            ++a_index;
        } else if (tri_remap[a_index].val[0] > used_verts[b_index]) {
            ++b_index;
        } else {
            tri_remap[a_index].val[0] = b_index;
            ++a_index;
        }
    }
    qsort(tri_remap, n_tri_indices, sizeof(IntTuple), compareTupleSecond);

    for (int i = 0; i < n_tri_indices; ++i) {
        tris_out[i] = base_vert[tri_remap[i].val[0]];
    }

    delete[] sort_verts;
    delete[] used_verts;
    delete[] tri_remap;
    delete[] base_vert;
}
}  // namespace NavMeshTemp

void NavMesh::AddMesh(const vector<float>& vertices, const vector<unsigned>& faces, const mat4& transform) {
    size_t face_index_offset = vertices_.size() / 3;
    vector<int> merge_tris(faces.size());
    NavMeshTemp::MergeOverlappingVerts(&vertices[0], (int)vertices.size() / 3, &faces[0], (int)faces.size() / 3, &merge_tris[0]);
    // Add transformed vertices to vertices_
    vertices_.reserve(vertices_.size() + vertices.size());
    for (size_t i = 0, len = vertices.size(); i < len; i += 3) {
        vec3 temp;
        for (int j = 0; j < 3; ++j) {
            temp[j] = vertices[i + j];
        }
        temp = transform * temp;
        for (int j = 0; j < 3; ++j) {
            vertices_.push_back(temp[j]);
        }
    }
    // Add offset faces
    faces_.reserve(faces_.size() + faces.size());
    for (size_t i = 0; i < faces.size(); i++) {
        faces_.push_back((uint32_t)(merge_tris[i] + face_index_offset));
    }
}

void NavMesh::SetNavMeshParameters(NavMeshParameters& nmp) {
    nav_mesh_parameters_ = nmp;
}

void NavMesh::Update() {
    Mouse& mouse = Input::Instance()->getMouse();
    Keyboard& keyboard = Input::Instance()->getKeyboard();
    if (keyboard.isKeycodeDown(SDLK_CTRL, KIMF_LEVEL_EDITOR_GENERAL) && (mouse.mouse_down_[Mouse::LEFT] == 1 || mouse.mouse_down_[Mouse::RIGHT] == 1)) {
        bool shift = (mouse.mouse_down_[Mouse::RIGHT] == 1);
        vec3 start = ActiveCameras::Get()->GetPos();
        vec3 end = start + ActiveCameras::Get()->GetMouseRay() * 1000.0f;
        float rays[] = {start[0], start[1], start[2]};
        float raye[] = {end[0], end[1], end[2]};
        float t;
        if (geom_.raycastMesh(rays, raye, t)) {
            float pos[3];
            pos[0] = rays[0] + (raye[0] - rays[0]) * t;
            pos[1] = rays[1] + (raye[1] - rays[1]) * t;
            pos[2] = rays[2] + (raye[2] - rays[2]) * t;
            // nav_mesh_tester_.handleClick(NULL, pos, shift);

            if (!shift) {
                m_start_ = vec3(pos[0], pos[1], pos[2]);
            } else {
                m_end_ = vec3(pos[0], pos[1], pos[2]);
            }

            NavPath path = FindPath(m_start_, m_end_, SAMPLE_POLYFLAGS_ALL, SAMPLE_POLYFLAGS_NONE);
            path_points_ = path.waypoints;
        }
    }
}

void WriteObj(string path,
              vector<float> vertices,
              vector<unsigned> faces) {
    CreateParentDirs(path.c_str());  // the Levels folder in users home folder might be missing.
    ofstream file;
    my_ofstream_open(file, path.c_str());

    for (unsigned i = 0; i < vertices.size(); i += 3) {
        file << "v " << vertices[i + 0] << " "
             << vertices[i + 1] << " "
             << vertices[i + 2] << "\n";
    }
    for (unsigned i = 0; i < faces.size(); i += 3) {
        file << "f " << (int)faces[i + 0] + 1 << " "
             << (int)faces[i + 1] + 1 << " "
             << (int)faces[i + 2] + 1 << "\n";
    }

    file.close();
}

namespace {
// see http://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
/*
float orient2d(const float *a, const float *b, const float *c) {
    return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0]);
}
*/

bool Convex2DCollide(const vector<vec2> points[2], const vector<vec2>& axes) {
    bool intersects = true;
    for (const auto& axis : axes) {
        float proj_bounds[2][2];
        for (size_t i = 0; i < 2; ++i) {
            const vector<vec2>& point_vec = points[i];
            for (size_t j = 0, len = point_vec.size(); j < len; ++j) {
                float proj = dot(point_vec[j], axis);
                if (j == 0 || proj < proj_bounds[i][0]) {
                    proj_bounds[i][0] = proj;
                }
                if (j == 0 || proj > proj_bounds[i][1]) {
                    proj_bounds[i][1] = proj;
                }
            }
        }
        if (proj_bounds[0][1] < proj_bounds[1][0] || proj_bounds[0][0] > proj_bounds[1][1]) {
            intersects = false;
        }
    }
    return intersects;
}

void ClipPolygon(vector<vec3>* points_a, vector<vec3>* points_b, vector<vec3> plane_normals, vector<float> plane_ds) {
    vector<vec3>* old_points = points_a;
    vector<vec3>* new_points = points_b;
    for (size_t i = 0, len = plane_normals.size(); i < len; ++i) {
        const vec3& plane_normal = plane_normals[i];
        float plane_d = plane_ds[i];
        new_points->clear();
        for (size_t j = 0, len = old_points->size(); j < len; ++j) {
            const vec3& point = old_points->at(j);
            const vec3& next_point = old_points->at((j + 1) % len);
            float plane_offset = dot(point, plane_normal) - plane_d;
            float next_plane_offset = dot(next_point, plane_normal) - plane_d;
            if (plane_offset > 0) {
                new_points->push_back(point);
            }
            if ((plane_offset > 0) != (next_plane_offset > 0)) {
                float t = (plane_offset) / (plane_offset - next_plane_offset);
                vec3 new_point = mix(point, next_point, t);
                new_points->push_back(new_point);
            }
        }
        swap(new_points, old_points);
    }
    swap(new_points, old_points);
    if (new_points != points_b) {
        *points_b = *new_points;
    }
}

void AddVoxelDraw(const vec3& pos, vector<float>* debug_line_vertices, vector<unsigned>* debug_line_indices, float size) {
    for (int j = 0; j < 3; ++j) {
        debug_line_indices->push_back((uint32_t)debug_line_vertices->size() / 3);
        for (int i = 0; i < 3; ++i) {
            debug_line_vertices->push_back(pos[i]);
        }
        debug_line_indices->push_back((uint32_t)debug_line_vertices->size() / 3);
        for (int i = 0; i < 3; ++i) {
            if (i != j) {
                debug_line_vertices->push_back(pos[i]);
            } else {
                debug_line_vertices->push_back(pos[i] + size);
            }
        }
    }
}

struct ColumnEntry {
    int tri;
    int y;
};
}  // namespace

// This function is only for vizualization purpose, it's not used for logic.
void VoxellizeMesh(const vector<unsigned int>& faces, const vector<float>& vertices) {
    vector<float> debug_line_vertices;
    vector<unsigned> debug_line_indices;
    if (vertices.size() < 3 || faces.size() < 3) {
        DisplayError("Error", "Invalid input to VoxellizeMesh");
    }
    // Calculate bounding box for every vertex
    float bounds[6];
    if (vertices.size() >= 3) {
        for (int i = 0; i < 3; ++i) {
            bounds[i] = vertices[i];
            bounds[i + 3] = vertices[i];
        }
    }
    for (size_t i = 3, len = vertices.size(); i < len; ++i) {
        size_t axis = i % 3;
        bounds[axis] = min(vertices[i], bounds[axis]);
        bounds[axis + 3] = max(vertices[i], bounds[axis + 3]);
    }
    // Calculate voxel dimensions
    int voxel_dim[3];
    static const float VOXEL_SIZE = 0.5f;
    for (int i = 0; i < 3; ++i) {
        voxel_dim[i] = (int)ceil((bounds[i + 3] - bounds[i]) / VOXEL_SIZE) + 1;
    }
    // Create space for column lists
    typedef list<ColumnEntry> ColumnEntryList;
    vector<ColumnEntryList> columns(voxel_dim[0] * voxel_dim[2]);
    // Rasterize triangles
    for (size_t face_index = 0, len = faces.size(); face_index < len; face_index += 3) {
        // Get pointers for quick access to tri vertices
        const float* vert[3];
        for (int j = 0; j < 3; ++j) {
            vert[j] = &vertices[faces[face_index + j] * 3];
        }
        // Get bounding box of triangle
        float tri_bounds[6];
        for (int j = 0; j < 3; ++j) {
            tri_bounds[j] = vert[0][j];
            tri_bounds[j + 3] = vert[0][j];
        }
        for (int k = 1; k < 3; ++k) {
            for (int j = 0; j < 3; ++j) {
                tri_bounds[j] = min(tri_bounds[j], vert[k][j]);
                tri_bounds[j + 3] = max(tri_bounds[j + 3], vert[k][j]);
            }
        }
        // Get triangle boundaries in voxel coordinates
        int vox_bounds[6];
        for (int j = 0; j < 3; ++j) {
            vox_bounds[j] = (int)((tri_bounds[j] - bounds[j]) / VOXEL_SIZE);
            vox_bounds[j + 3] = (int)((tri_bounds[j + 3] - bounds[j]) / VOXEL_SIZE);
        }
        // Rasterize triangle into voxels
        vector<vec2> points[2];
        vector<vec2> axes;
        vector<vec3> old_points;
        vector<vec3> new_points;
        for (int vox_z = vox_bounds[2]; vox_z <= vox_bounds[5]; ++vox_z) {
            for (int vox_x = vox_bounds[0]; vox_x <= vox_bounds[3]; ++vox_x) {
                // Does column intersect triangle?
                points[0].clear();
                for (int i = 0; i < 4; ++i) {
                    points[0].push_back(vec2((vox_x + i % 2) * VOXEL_SIZE + bounds[0], (vox_z + i / 2) * VOXEL_SIZE + bounds[2]));
                }
                points[1].clear();
                for (auto& i : vert) {
                    points[1].push_back(vec2(i[0], i[2]));
                }
                axes.clear();
                axes.push_back(vec2(1, 0));
                axes.push_back(vec2(0, 1));
                // Add axes perpendicular to each triangle side
                for (int i = 0; i < 3; ++i) {
                    vec2 temp = points[1][(i + 1) % 3] - points[1][i];
                    axes.push_back(vec2(-temp[1], temp[0]));
                }
                if (Convex2DCollide(points, axes)) {
                    old_points.clear();
                    new_points.clear();
                    for (auto& i : vert) {
                        old_points.push_back(vec3(i[0], i[1], i[2]));
                    }
                    vector<vec3> plane_normal;
                    vector<float> plane_d;
                    plane_normal.push_back(vec3(1, 0, 0));
                    plane_d.push_back((vox_x)*VOXEL_SIZE + bounds[0]);
                    plane_normal.push_back(vec3(-1, 0, 0));
                    plane_d.push_back(((vox_x + 1) * VOXEL_SIZE + bounds[0]) * -1.0f);
                    plane_normal.push_back(vec3(0, 0, 1));
                    plane_d.push_back((vox_z)*VOXEL_SIZE + bounds[2]);
                    plane_normal.push_back(vec3(0, 0, -1));
                    plane_d.push_back(((vox_z + 1) * VOXEL_SIZE + bounds[2]) * -1.0f);
                    ClipPolygon(&old_points, &new_points, plane_normal, plane_d);
                    if (!new_points.empty()) {
                        float clip_tri_bounds[2] = {0.0f, 0.0f};
                        for (size_t i = 0, len = new_points.size(); i < len; ++i) {
                            float y = new_points[i][1];
                            if (i == 0 || y < clip_tri_bounds[0]) {
                                clip_tri_bounds[0] = y;
                            }
                            if (i == 0 || y > clip_tri_bounds[1]) {
                                clip_tri_bounds[1] = y;
                            }
                        }
                        int clip_tri_vox_bounds[2];
                        clip_tri_vox_bounds[0] = (int)((clip_tri_bounds[0] - bounds[1]) / VOXEL_SIZE);
                        clip_tri_vox_bounds[1] = (int)((clip_tri_bounds[1] - bounds[1]) / VOXEL_SIZE);
                        int column_index = vox_z * voxel_dim[0] + vox_x;
                        for (int i = 0; i < 1; ++i) {
                            ColumnEntry entry;
                            entry.tri = -1;
                            entry.y = clip_tri_vox_bounds[i];
                            columns[column_index].push_back(entry);
                        }
                    }
                }
            }
        }
    }

    // Draw voxels
    for (int vox_z = 0; vox_z < voxel_dim[2]; ++vox_z) {
        for (int vox_x = 0; vox_x < voxel_dim[0]; ++vox_x) {
            int column_index = vox_z * voxel_dim[0] + vox_x;
            ColumnEntryList list = columns[column_index];
            for (auto& entry : list) {
                vec3 pos;
                pos[0] = vox_x * VOXEL_SIZE + bounds[0];
                pos[1] = entry.y * VOXEL_SIZE + bounds[1];
                pos[2] = vox_z * VOXEL_SIZE + bounds[2];
                AddVoxelDraw(pos, &debug_line_vertices, &debug_line_indices, VOXEL_SIZE);
            }
        }
    }
    if (!debug_line_indices.empty()) {
        DebugDraw::Instance()->AddLines(debug_line_vertices, debug_line_indices, vec4(1.0f), _persistent, _DD_NO_FLAG);
    }
}

void NavMesh::CalcNavMesh() {
    // VoxellizeMesh(faces_, vertices_);
    // return;

    LOGI << "Navmesh: Writing out object..." << endl;
    char path[kPathSize];
    FormatString(path, kPathSize, "%sData/Temp/navmesh.obj", GetWritePath(CoreGameModID).c_str());
    WriteObj(path, vertices_, faces_);
    // CalcFaceNormals(face_normals_);

    BuildContext ctx;
    LOGI << "Navmesh: Loading object into geom..." << endl;
    geom_.loadMesh(&ctx, path);

    sample_tile_mesh_.applySettings(nav_mesh_parameters_);
    sample_tile_mesh_.setContext(&ctx);
    LOGI << "Navmesh: Adding mesh to sample..." << endl;
    sample_tile_mesh_.handleMeshChanged(&geom_);

    sample_tile_mesh_.handleSettings();

    LOGI << "Navmesh: Building sample..." << endl;
    sample_tile_mesh_.handleBuild();

    LOGI << "Navmesh: Done building sample..." << endl;

    // nav_mesh_tester_.init(&sample_tile_mesh_);
    loaded_ = true;
}

void NavMesh::Save(const string& level_name, const Path& level_path) {
    LOGW << level_name << endl;

    if (!loaded_) {
        return;
    }

    string nav_path = GenerateParallelPath("Data/Levels", "Data/LevelNavmeshes", ".nav", level_path);

    if (config["allow_game_dir_save"].toBool() == false) {  // change save location to user directory
        nav_path = AssemblePath(GetWritePath(level_path.GetModsource()), nav_path);
    }

    char zip_path[kPathSize];  // Path of the outermost file
    FormatString(zip_path, kPathSize, "%s.zip", nav_path.c_str());

    char in_zip_file_name[kPathSize];                                         // Name of the file inside the outermost zip file
    FormatString(in_zip_file_name, kPathSize, "%s.nav", level_name.c_str());  // used to be .zip

    LOGI << "Saving nav mesh to " << zip_path << endl;

    string temp_navmesh_path = AssemblePath(GetWritePath(CoreGameModID), "Data/Temp/navmesh.nav");

    CreateParentDirs(temp_navmesh_path);

    // Save the .NAV file to the Temp directory
    sample_tile_mesh_.Save(temp_navmesh_path.c_str());

    CreateParentDirs(zip_path);

    // Copy the .NAV file from the Temp directory and save to the Zip file as 'level_name.zip' (why .zip?)
    Zip(temp_navmesh_path, string(zip_path), in_zip_file_name, _YES_OVERWRITE);

    // Write the .OBJ file to the LevelNavmeshes directory
    if (!vertices_.empty() && !faces_.empty()) {
        char model_path[kPathSize];
        FormatString(model_path, kPathSize, "%s.obj", nav_path.c_str());
        WriteObj(model_path, vertices_, faces_);
    }

    // Write the .XML file to the LevelNavmeshes directory
    char meta_path[kPathSize];
    FormatString(meta_path, kPathSize, "%s.xml", nav_path.c_str());

    {
        TiXmlDocument meta_doc;

        TiXmlDeclaration decl("2.0", "", "");

        TiXmlElement meta_root("NavMeshMeta");

        {
            TiXmlElement e("Level");
            TiXmlText et(level_name.c_str());
            e.InsertEndChild(et);
            meta_root.InsertEndChild(e);
        }

        {
            TiXmlElement e("IgnoreHash");
            TiXmlText et("false");
            e.InsertEndChild(et);
            meta_root.InsertEndChild(e);
        }

        {
            TiXmlElement e("LevelHash");

            FileHashAssetRef fha = Engine::Instance()->GetAssetManager()->LoadSync<FileHashAsset>(level_path.GetOriginalPath());
            TiXmlText et(fha->hash.ToString());
            e.InsertEndChild(et);

            meta_root.InsertEndChild(e);
        }

        {
            TiXmlElement e("IgnoreVersion");
            TiXmlText et("false");
            e.InsertEndChild(et);
            meta_root.InsertEndChild(e);
        }

        {
            TiXmlElement e("Version");
            TiXmlText et(NAVMESH_VERSION);
            e.InsertEndChild(et);
            meta_root.InsertEndChild(e);
        }

        meta_doc.InsertEndChild(decl);
        meta_doc.InsertEndChild(meta_root);

        meta_doc.SaveFile(meta_path);
    }
}

bool NavMesh::Load(const string& level_name, const Path& level_path) {
    rcContext ctx;
    char rel_model_path[kPathSize];
    char rel_nav_path[kPathSize];
    char rel_meta_path[kPathSize];
    char rel_zip_path[kPathSize];

    char abs_model_path[kPathSize];
    char abs_nav_path[kPathSize];
    char abs_meta_path[kPathSize];
    char abs_zip_path[kPathSize];

    string base_path = GenerateParallelPath("Data/Levels", "Data/LevelNavmeshes", ".nav", level_path);

    FormatString(rel_model_path, kPathSize, "%s.obj", base_path.c_str());
    FormatString(rel_nav_path, kPathSize, "%s", base_path.c_str());
    FormatString(rel_meta_path, kPathSize, "%s.xml", base_path.c_str());
    FormatString(rel_zip_path, kPathSize, "%s.zip", base_path.c_str());

    bool valid_path_group = true;
    bool has_zip = false;

    if (FindFilePath(rel_meta_path, abs_meta_path, kPathSize, kWriteDir | kModWriteDirs, false) == -1) {
        LOGE << "Missing file for navmesh " << abs_meta_path << endl;
        valid_path_group = false;
    } else {
        LOGI << "Found:" << abs_meta_path << endl;
    }

    if (FindFilePath(rel_model_path, abs_model_path, kPathSize, kWriteDir | kModWriteDirs, false) == -1) {
        LOGI << "Missing file for navmesh " << abs_model_path << endl;
        // return false;
    } else {
        LOGI << "Found:" << abs_model_path << endl;
    }

    if (FindFilePath(rel_zip_path, abs_zip_path, kPathSize, kWriteDir | kModWriteDirs, false) == -1) {
        has_zip = false;
    } else {
        has_zip = true;
        LOGI << "Found Zip:" << abs_zip_path << endl;
    }

    if (FindFilePath(rel_nav_path, abs_nav_path, kPathSize, kWriteDir | kModWriteDirs, false) == -1) {
        if (has_zip == false) {
            LOGE << "Missing file for navmesh " << abs_nav_path << endl;
            valid_path_group = false;
        }
    } else {
        LOGI << "Found:" << abs_nav_path << endl;
    }

    if (valid_path_group) {
        if (LoadFromAbs(level_path, abs_meta_path, abs_model_path, abs_nav_path, abs_zip_path, has_zip)) {
            return true;
        }
    }

    has_zip = false;
    valid_path_group = true;

    if (FindFilePath(rel_meta_path, abs_meta_path, kPathSize, kModPaths | kDataPaths, false) == -1) {
        LOGE << "Missing file for navmesh " << abs_meta_path << endl;
        valid_path_group = false;
    } else {
        LOGI << "Found:" << abs_meta_path << endl;
    }

    if (FindFilePath(rel_model_path, abs_model_path, kPathSize, kModPaths | kDataPaths, false) == -1) {
        LOGI << "Missing file for navmesh " << abs_model_path << endl;
        // return false;
    } else {
        LOGI << "Found:" << abs_model_path << endl;
    }

    if (FindFilePath(rel_zip_path, abs_zip_path, kPathSize, kModPaths | kDataPaths, false) == -1) {
        has_zip = false;
    } else {
        has_zip = true;
        LOGI << "Found Zip:" << abs_zip_path << endl;
    }

    if (FindFilePath(rel_nav_path, abs_nav_path, kPathSize, kModPaths | kDataPaths, false) == -1) {
        if (has_zip == false) {
            LOGE << "Missing file for navmesh " << abs_nav_path << endl;
            valid_path_group = false;
        }
    } else {
        LOGI << "Found:" << abs_nav_path << endl;
    }

    if (valid_path_group) {
        if (LoadFromAbs(level_path, abs_meta_path, abs_model_path, abs_nav_path, abs_zip_path, has_zip)) {
            return true;
        }
    }

    return false;
}

bool NavMesh::LoadFromAbs(const Path& level_path, const char* abs_meta_path, const char* abs_model_path, const char* abs_nav_path, const char* abs_zip_path, bool has_zip) {
    {
        // Load the XML file
        PROFILER_ZONE(g_profiler_ctx, "Loading XML file");
        TiXmlDocument doc(abs_meta_path);
        doc.LoadFile();

        CommonRegex cr;

        TiXmlElement* pRoot = doc.RootElement();

        if (pRoot) {
            TiXmlHandle hRoot(pRoot);

            TiXmlElement* pIgnoreVersion = hRoot.FirstChild("IgnoreVersion").Element();
            if (pIgnoreVersion) {
                string ignoreversion = pIgnoreVersion->GetText();

                if (cr.saysFalse(ignoreversion)) {
                    TiXmlElement* pVersion = hRoot.FirstChild("Version").Element();

                    if (pVersion) {
                        string version = pVersion->GetText();

                        // If the version for the navmesh mismatches, recalculate
                        if (version != string(NAVMESH_VERSION)) {
                            LOGW << "Mismatching navmesh version" << endl;
                            return false;
                        }
                    } else {
                        LOGE << "Missing Version" << endl;
                        return false;
                    }
                } else if (cr.saysTrue(ignoreversion)) {
                    // Do nothing continue with loading
                } else {
                    LOGE << "Value in IgnoreVersion in file \"" << abs_meta_path << "\" is invalid." << endl;
                    return false;
                }
            } else {
                LOGE << "Missing IgnoreVersion" << endl;
                return false;
            }

            TiXmlElement* pIgnoreHash = hRoot.FirstChild("IgnoreHash").Element();

            if (pIgnoreHash) {
                string ignorehash = pIgnoreHash->GetText();

                if (cr.saysFalse(ignorehash)) {
                    TiXmlElement* pLevelHash = hRoot.FirstChild("LevelHash").Element();
                    if (pLevelHash) {
                        string levelHash = pLevelHash->GetText();
                        FileHashAssetRef file_hash = Engine::Instance()->GetAssetManager()->LoadSync<FileHashAsset>(level_path.GetOriginalPath());
                        if (levelHash != file_hash->hash.ToString()) {
                            LOGW << "Mismatching LevelHash in navmesh" << endl;
                            return false;
                        }
                    } else {
                        LOGE << "Missing LevelHash" << endl;
                        return false;
                    }
                } else if (cr.saysTrue(ignorehash)) {
                    // Do nothing, just continue with loading.
                } else {
                    LOGE << "Value in IgnoreHash in file \"" << abs_meta_path << "\" is invalid." << endl;
                    return false;
                }
            } else {
                LOGE << "Missing IgnoreHash" << endl;
                return false;
            }
        } else {
            LOGE << "Error parsing meta file for \"" << abs_meta_path << "\"" << endl;
            return false;
        }
    }

    {
        // PROFILER_ZONE(g_profiler_ctx, "Loading mesh");
        // geom_.loadMesh(&ctx, abs_model_path);
    }
    LOGI << "Navmesh: Adding mesh to sample..." << endl;
    {
        PROFILER_ZONE(g_profiler_ctx, "handleMeshChanged");
        sample_tile_mesh_.handleMeshChanged(&geom_);
    }

    {
        if (has_zip) {  // Attempt to load data from the zip file
            ExpandedZipFile ezf;
            UnZip(abs_zip_path, ezf);

            if (ezf.GetNumEntries() == 1) {
                const char* filename;
                const char* data;
                unsigned size;
                ezf.GetEntry(0, filename, data, size);

                if (data && size > 0) {
                    sample_tile_mesh_.LoadMem(data, size);
                } else {
                    LOGE << "Zip file data or size is NULL/zero" << endl;
                }
            } else {
                LOGE << "Zip contains more than one item, this is unexpected" << endl;
            }

        } else {
            PROFILER_ZONE(g_profiler_ctx, "Load");
            sample_tile_mesh_.Load(abs_nav_path);
        }
    }
    // nav_mesh_tester_.init(&sample_tile_mesh_);
    loaded_ = true;
    return true;
}

NavPath NavMesh::FindPath(const vec3& start, const vec3& end, uint16_t include_filter, uint16_t exclude_filter) {
    NavPath nav_path;
    nav_path.success = false;
    if (!loaded_) {
        nav_path.waypoints.push_back(end);
        nav_path.success = true;
        return nav_path;
    }

    const float extents[] = {2.0f, 4.0f, 2.0f};

    dtQueryFilter filter;
    filter.setIncludeFlags(include_filter);
    filter.setExcludeFlags(exclude_filter);

    dtNavMeshQuery* mesh = sample_tile_mesh_.getNavMeshQuery();
    dtPolyRef start_poly_ref;
    dtStatus status;
    status = mesh->findNearestPoly(start.entries,
                                   extents,
                                   &filter,
                                   &start_poly_ref,
                                   0);

    dtPolyRef end_poly_ref;
    status = mesh->findNearestPoly(end.entries,
                                   extents,
                                   &filter,
                                   &end_poly_ref,
                                   0);

    if (start_poly_ref && end_poly_ref) {
        static const int MAX_POLYS = 256;
        dtPolyRef polys[MAX_POLYS];
        int num_path_polys;
        status = mesh->findPath(start_poly_ref,
                                end_poly_ref,
                                start.entries,
                                end.entries,
                                &filter,
                                polys,
                                &num_path_polys,
                                MAX_POLYS);
        float straight_path_points[MAX_POLYS * 3];
        unsigned char straight_path_flags[MAX_POLYS];
        dtPolyRef straight_path_polys[MAX_POLYS];

        if (dtStatusSucceed(status) && num_path_polys) {
            int straight_path_length;
            status = mesh->findStraightPath(start.entries,
                                            end.entries,
                                            polys,
                                            num_path_polys,
                                            straight_path_points,
                                            straight_path_flags,
                                            straight_path_polys,
                                            &straight_path_length,
                                            MAX_POLYS);
            if (straight_path_length) {
                nav_path.waypoints.resize(straight_path_length);

                unsigned index = 0;
                for (int i = 0; i < straight_path_length; ++i) {
                    nav_path.waypoints[i][0] = straight_path_points[index++];
                    nav_path.waypoints[i][1] = straight_path_points[index++];
                    nav_path.waypoints[i][2] = straight_path_points[index++];

                    nav_path.flags.push_back(straight_path_flags[i]);
                }
            }
            if (dtStatusSucceed(status)) {
                nav_path.success = true;
            }
            if (dtStatusDetail(status, DT_PARTIAL_RESULT)) {
                nav_path.success = false;
            }
            if (polys[num_path_polys - 1] != end_poly_ref) {
                nav_path.success = false;
            }
        }
    }

    return nav_path;
}

vec3 NavMesh::RayCast(const vec3& start, const vec3& end) {
    if (!loaded_) {
        return vec3(0.0f);
    }

    const float extents[] = {2.0f, 4.0f, 2.0f};

    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL);
    filter.setExcludeFlags(0);

    dtNavMeshQuery* mesh = sample_tile_mesh_.getNavMeshQuery();
    dtPolyRef start_poly_ref;
    dtStatus status;
    status = mesh->findNearestPoly(start.entries, extents, &filter, &start_poly_ref, 0);

    if (!start_poly_ref) {
        return vec3(0.0f);
    }
    static const int MAX_POLYS = 256;
    dtPolyRef polys[MAX_POLYS];
    int num_path_polys;

    float t_val;
    vec3 hit_normal;
    status = mesh->raycast(start_poly_ref,
                           start.entries,
                           end.entries,
                           &filter,
                           &t_val,
                           hit_normal.entries,
                           polys,
                           &num_path_polys,
                           MAX_POLYS);

    if (status != DT_SUCCESS) {
        return vec3(0.0f);
    }

    vec3 result;
    if (t_val == FLT_MAX) {
        result = end;
    } else {
        result = mix(start, end, t_val);
    }

    float height;
    status = mesh->getPolyHeight(polys[num_path_polys - 1], result.entries, &height);
    if (status == DT_SUCCESS) {
        result[1] = height;
    }
    return result;

    /*vec3 result;
    status = mesh->moveAlongSurface(start_poly_ref, start.entries, end.entries, &filter, result.entries, polys, &num_path_polys, MAX_POLYS);
    return result;*/
}

vec3 NavMesh::RayCastSlide(const vec3& start, const vec3& end, int depth) {
    if (!loaded_) {
        return vec3(0.0f);
    }

    const float extents[] = {2.0f, 4.0f, 2.0f};

    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL);
    filter.setExcludeFlags(0);

    dtNavMeshQuery* mesh = sample_tile_mesh_.getNavMeshQuery();
    dtPolyRef start_poly_ref;
    dtStatus status;
    status = mesh->findNearestPoly(start.entries, extents, &filter, &start_poly_ref, 0);

    if (!start_poly_ref) {
        return vec3(0.0f);
    }
    static const int MAX_POLYS = 256;
    dtPolyRef polys[MAX_POLYS];
    int num_path_polys;

    float t_val;
    vec3 hit_normal;
    status = mesh->raycast(start_poly_ref,
                           start.entries,
                           end.entries,
                           &filter,
                           &t_val,
                           hit_normal.entries,
                           polys,
                           &num_path_polys,
                           MAX_POLYS);

    if (status != DT_SUCCESS) {
        return vec3(0.0f);
    }

    vec3 result;
    if (t_val == FLT_MAX) {
        result = end;
    } else {
        result = mix(start, end, t_val);
        float wall_d = dot(hit_normal, result);
        float end_d = dot(hit_normal, end);
        vec3 slide_result = end + hit_normal * (wall_d - end_d);
        if (depth > 0) {
            return RayCastSlide(result, slide_result, depth - 1);
        }
    }

    float height;
    status = mesh->getPolyHeight(polys[num_path_polys - 1], result.entries, &height);
    if (status == DT_SUCCESS) {
        result[1] = height;
    }
    return result;

    /*vec3 result;
    status = mesh->moveAlongSurface(start_poly_ref, start.entries, end.entries, &filter, result.entries, polys, &num_path_polys, MAX_POLYS);
    return result;*/
}

NavPoint NavMesh::GetNavPoint(const vec3& point) {
    if (!loaded_) {
        return NavPoint();
    }

    const float extents[] = {2.0f, 4.0f, 2.0f};
    const float second_extents[] = {4.0f, 16.0f, 4.0f};

    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL);
    filter.setExcludeFlags(0);

    dtNavMeshQuery* mesh = sample_tile_mesh_.getNavMeshQuery();
    dtPolyRef start_poly_ref;
    dtStatus status;
    status = mesh->findNearestPoly(point.entries, extents, &filter, &start_poly_ref, 0);

    if (status != DT_SUCCESS) {
        status = mesh->findNearestPoly(point.entries, second_extents, &filter, &start_poly_ref, 0);
        if (status != DT_SUCCESS) {
            return NavPoint();
        }
    }

    vec3 output;
    bool isOverPoly;
    status = mesh->closestPointOnPoly(start_poly_ref, point.entries, output.entries, &isOverPoly);

    if (status == DT_SUCCESS) {
        return NavPoint(output);
    } else {
        return NavPoint(output, false);
    }
}

const rcMeshLoaderObj* NavMesh::getCollisionMesh() const {
    return geom_.getMesh();
}

const dtNavMesh* NavMesh::getNavMesh() const {
    return sample_tile_mesh_.getNavMesh();
}

const rcPolyMesh* NavMesh::getPolyMesh() const {
    return sample_tile_mesh_.getPolyMesh();
}

InputGeom& NavMesh::getInputGeom() {
    return geom_;
}

void NavMesh::SetExplicitBounderies(vec3 min, vec3 max) {
    sample_tile_mesh_.SetExplicitBounderies(min, max);
}

unsigned short NavMesh::GetOffMeshConnectionFlag(int userid) {
    unsigned short ret = 0U;

    const dtNavMesh* mesh = sample_tile_mesh_.getNavMesh();

    if (mesh) {
        for (int i = 0; i < mesh->getMaxTiles(); ++i) {
            const dtMeshTile* tile = mesh->getTile(i);
            if (!tile->header) continue;

            for (int j = 0; j < tile->header->offMeshConCount; j++) {
                const dtOffMeshConnection* off_mesh_con = &tile->offMeshCons[j];

                if (off_mesh_con) {
                    if ((int)off_mesh_con->userId == userid) {
                        dtPoly* mesh_poly = &tile->polys[off_mesh_con->poly];

                        if (mesh_poly && mesh_poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) {
                            return mesh_poly->flags;
                        } else {
                            LOGE << "Unable to get the poly associated with offmesh connection" << endl;
                        }
                    }
                }
            }
        }
    }

    return ret;
}
