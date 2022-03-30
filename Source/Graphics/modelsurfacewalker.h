//-----------------------------------------------------------------------------
//           Name: modelsurfacewalker.h
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
#include <Math/vec2.h>

#include <opengl.h>

#include <vector>

struct MSWTriInfo {
    int neighbors[3];
    float edge_length[3];
    bool edge_swapped[3];
    MSWTriInfo();
};

struct SurfaceWalker {
    enum Type {BLOOD, WATER, FIRE};
    int on_edge;
    int tri;
    int last_tri;
    vec3 pos;
    float amount;
    float delay;
    bool can_drip;
    Type type;
    SurfaceWalker();
};

struct SWresults {
    float dist_moved;
    int at_point;
    SWresults();
};

struct WalkLine {
    int tri;
    vec3 start;
    vec3 end;
};

class ModelSurfaceWalker {
public:
    std::vector<MSWTriInfo> tri_info; // Neighbors and edge lengths of each triangle
    void AttachTo(const std::vector<float> &vertices, const std::vector<unsigned> &faces);
    int GetNeighbor( int tri, int neighbor );
    SWresults Move( SurfaceWalker& sw, vec3* points, const vec3& dir, float drip_dist, std::vector<WalkLine>* trace );
};

vec3 bary_coords(const vec2 &p, const vec2 &a, const vec2 &b, const vec2&c);
