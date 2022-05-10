//-----------------------------------------------------------------------------
//           Name: kdtreecluster.h
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

#include <vector>

struct ClusterInfo {
    std::vector<int> closest_clusters;
};

struct KDNode {
    vec3 bounds[2];
    vec3 content_bounds[2];
    int axis;
    float plane_coord;
    ClusterInfo cluster_info;
    KDNode *child[2];
    KDNode(const vec3 &_min_bounds, const vec3 &_max_bounds, std::vector<vec3> points, int depth = 0);
    ~KDNode();
    KDNode *GetLeaf(const vec3 &point);
};
