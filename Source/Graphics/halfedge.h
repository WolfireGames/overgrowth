//-----------------------------------------------------------------------------
//           Name: halfedge.h
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

#include <Math/enginemath.h> 
#include <Math/mat4.h>

#include <Wrappers/glm.h>
#include <Graphics/simplify_types.h>

#include <vector>
#include <set>

struct HalfEdge;

struct HalfEdgeNode {
    HalfEdge *edge;
}; 

typedef std::multiset<HalfEdgeNode> HalfEdgeNodeHeap;

struct HalfEdge {
    int vert[2];
    int tex[2];
    float err;
    float pos;
    HalfEdge *twin, *next, *prev;
    int id;
    bool valid;
    HalfEdgeNodeHeap::iterator handle;
};

struct EdgeSortable {
    int verts[2];
    int id;
};

struct VertSortable {
    vec3 vert;
    int id;
};

static const float UNDEFINED_ERROR = -1.0f;

bool operator<(const HalfEdgeNode& a, const HalfEdgeNode& b);

typedef std::set<HalfEdge*> HalfEdgeSet;
typedef std::vector<HalfEdgeSet> HalfEdgeSetVec;

bool HalfEdgePairFind(const HalfEdge& a, const HalfEdge& b);
void CollapseHalfEdge(HalfEdgeNodeHeap &heap, HalfEdge *edge);
void CollapseVertPositions(std::vector<float> &verts, int edge[], float pos);
void CollapseTexPositions(std::vector<float> &tex, int edge[], float pos);
int GetNumTexCoords(const HalfEdge *edge, int which);
void CollapseParentRecord(WOLFIRE_SIMPLIFY::ParentRecordList &a, WOLFIRE_SIMPLIFY::ParentRecordList &b, float weight);
void CollapseEdge(HalfEdgeNodeHeap &heap, HalfEdge *edge, std::vector<float>& vertices, std::vector<float>& tex_coords, std::vector<glm::mat4> &quadrics, WOLFIRE_SIMPLIFY::ParentRecordListVec &vert_parents, WOLFIRE_SIMPLIFY::ParentRecordListVec &tex_parents, HalfEdgeSetVec &vert_edges, HalfEdgeSetVec &tex_edges, bool include_tex);
void ReconstructModel(const std::vector<HalfEdge> &half_edges, WOLFIRE_SIMPLIFY::SimplifyModel *model, bool include_tex);

bool HalfEdgePairFind(const HalfEdge& a, const HalfEdge& b);
bool SortEdgeSortable(const EdgeSortable& a, const EdgeSortable& b);
bool SortVertSortable(const VertSortable& a, const VertSortable& b);
