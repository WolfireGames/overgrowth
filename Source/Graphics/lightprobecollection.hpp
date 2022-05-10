//-----------------------------------------------------------------------------
//           Name: lightprobecollection.h
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

#include <Math/vec3.h>
#include <Asset/Asset/texture.h>
#include <Graphics/model.h>

#include <opengl.h>

#include <vector>
#include <queue>

struct LightProbe {
    int id;
    vec3 pos;
    bool negative;
    vec3 ambient_cube_color[6];
    vec3 ambient_cube_color_buf[6];  // Used for calculating second bounce
};

struct TetMesh {
    std::vector<vec3> points;
    std::vector<int> tet_points;
    std::vector<int> neighbors;
    std::vector<int> point_id;
    int display_id;
};

struct LightProbeUpdateEntry {
    int id;
    int pass;  // 0 for first bounce, 1 for second bounce
};

struct GridLookup {
    static const int kMaxGridCells = 1000000;
    vec3 bounds[2];
    int subdivisions[3];
    std::vector<unsigned> cell_tet;  // What tetrahedron is closest to the center of each cell
};

class BulletWorld;

class LightProbeCollection {
   public:
    LightProbeCollection();
    ~LightProbeCollection();
    int AddProbe(const vec3& pos, bool negative, const float* coeff = NULL);
    bool MoveProbe(int id, const vec3& pos);
    bool SetNegative(int id, bool negative);
    bool DeleteProbe(int id);
    void Draw(BulletWorld& bw);
    void Init();
    void Dispose();
    int ShaderNumLightProbes();
    int ShaderNumTetrahedra();
    LightProbe* GetNextProbeToProcess();
    // Returns id of tetrahedron containing position,
    // and fills interpolated ambient cube color
    int GetTetrahedron(const vec3& position, vec3 ambient_cube_color[], int best_guess);
    GLuint cube_map_fbo;
    TextureRef cube_map;
    TextureRef ambient_3d_tex;
    std::vector<LightProbe> light_probes;
    TetMesh tet_mesh;
    LightProbe* GetProbeFromID(int id);
    int light_probe_texture_buffer_id;
    int light_probe_buffer_object_id;
    int grid_texture_buffer_id;
    int grid_buffer_object_id;
    void UpdateTextureBuffer(BulletWorld& bw);
    std::queue<LightProbeUpdateEntry> to_process;
    GridLookup grid_lookup;
    bool tet_mesh_viz_enabled;
    bool show_probes;
    bool show_probes_through_walls;
    bool probe_lighting_enabled;
    bool light_volume_enabled;
    int probe_model_id;

   private:
    bool tet_mesh_needs_update;
    int next_id;
    void UpdateTetMesh(BulletWorld& bw);
};

static const bool kLightProbeEnabled = true;
static const unsigned int kLightProbeNumCoeffs = 18;
extern bool kLightProbe2pass;
