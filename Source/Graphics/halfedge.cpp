//-----------------------------------------------------------------------------
//           Name: halfedge.cpp
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
#include "halfedge.h"

#include <Graphics/simplify.hpp>

using namespace WOLFIRE_SIMPLIFY;

bool operator<(const HalfEdgeNode& a, const HalfEdgeNode& b) {
    return a.edge->err < b.edge->err;
}

// Sort half edges with sorted vert lists, so pairs end up next to one another
// Warning: This might not swap the contents fully enough for general use.
bool HalfEdgePairFind(const HalfEdge& a, const HalfEdge& b){
    int vert_a[2];
    int vert_b[2];
    vert_a[0] = a.vert[0];
    vert_a[1] = a.vert[1];
    vert_b[0] = b.vert[0];
    vert_b[1] = b.vert[1];
    if(vert_a[0] > vert_a[1]){
        std::swap(vert_a[0], vert_a[1]);
    }
    if(vert_b[0] > vert_b[1]){
        std::swap(vert_b[0], vert_b[1]);
    }
    if(vert_a[0] == vert_b[0]){
        return vert_a[1]<vert_b[1];
    } else {
        return vert_a[0]<vert_b[0];
    }
}

void CollapseHalfEdge(HalfEdgeNodeHeap &heap, HalfEdge *edge) {
    edge->valid = false;
    heap.erase(edge->handle);
    edge->next->valid = false;
    heap.erase(edge->next->handle);
    if(edge->next->twin){
        edge->next->twin->twin = edge->prev->twin;
    }
    edge->prev->valid = false;
    heap.erase(edge->prev->handle);
    if(edge->prev->twin){
        edge->prev->twin->twin = edge->next->twin;
    }
}

void CollapseVertPositions(std::vector<float> &verts, int edge[], float pos){
    int vert_index[2];
    for(int j=0; j<2; ++j){
        vert_index[j] = edge[j]*3;
    }
    for(int i=0; i<3; ++i){
        float mid = verts[vert_index[0]+i]*(1.0f-pos) + verts[vert_index[1]+i]*pos;
        verts[vert_index[0]+i] = mid;
    }
}

void CollapseTexPositions(std::vector<float> &tex, int edge[], float pos){
    int tex_index[2];
    for(int j=0; j<2; ++j){
        tex_index[j] = edge[j]*2;
    }
    for(int i=0; i<2; ++i){
        float mid = tex[tex_index[0]+i]*(1.0f-pos) + tex[tex_index[1]+i]*pos;
        tex[tex_index[0]+i] = mid;
    }
}

int GetNumTexCoords(const HalfEdge *edge, int which) {
    std::set<int> tex_ids;
    const HalfEdge *spin_edge = edge;
    if(which == 0){
        while(true){
            tex_ids.insert(spin_edge->tex[0]);
            spin_edge = spin_edge->prev;
            if(!spin_edge->twin){
                break;
            }
            spin_edge = spin_edge->twin;
            if(spin_edge == edge){
                break;
            }
        }
    } else {
        while(true){
            tex_ids.insert(spin_edge->tex[1]);
            spin_edge = spin_edge->next;
            if(!spin_edge->twin){
                break;
            }
            spin_edge = spin_edge->twin;
            if(spin_edge == edge){
                break;
            }
        }
    }
    return tex_ids.size();
}

void CollapseParentRecord(ParentRecordList &a, ParentRecordList &b, float weight){
    if(&a == &b){
        return;
    }
    for(auto & pr : a){
        pr.weight *= (1.0f - weight);
    }
    for(auto & pr : b){
        pr.weight *= weight;
    }
    a.splice(a.end(), b);
    for(ParentRecordList::iterator iter = a.begin(); iter != a.end(); ++iter){
        //ParentRecord &pr = *iter;
    }
}

void CollapseEdge(HalfEdgeNodeHeap &heap, HalfEdge *edge, std::vector<float>& vertices, std::vector<float>& tex_coords, std::vector<glm::mat4> &quadrics, ParentRecordListVec &vert_parents, ParentRecordListVec &tex_parents, HalfEdgeSetVec &vert_edges, HalfEdgeSetVec &tex_edges, bool include_tex) {
    quadrics[edge->vert[0]] += quadrics[edge->vert[1]];

    int c = 0;
    CollapseParentRecord(vert_parents[edge->vert[0]], vert_parents[edge->vert[1]], edge->pos);
    CollapseVertPositions(vertices, edge->vert, edge->pos);
    HalfEdgeSet vert_edge_set = vert_edges[edge->vert[1]];
    for(auto change_edge : vert_edge_set){
        if(change_edge == edge || !change_edge->valid){
            continue;
        }
        if(change_edge->vert[0] == edge->vert[1]){
            change_edge->vert[0] = edge->vert[0];
        }
        if(change_edge->vert[1] == edge->vert[1]){
            change_edge->vert[1] = edge->vert[0];
        }
        vert_edges[edge->vert[0]].insert(change_edge);
    }
    vert_edge_set.clear();
    if(include_tex) {
        CollapseParentRecord(tex_parents[edge->tex[0]], tex_parents[edge->tex[1]], edge->pos);
        CollapseTexPositions(tex_coords, edge->tex, edge->pos);
        HalfEdgeSet tex_edge_set = tex_edges[edge->tex[1]];
        for(auto change_edge : tex_edge_set){
            if(change_edge == edge || !change_edge->valid){
                continue;
            }
            if(change_edge->tex[0] == edge->tex[1]){
                change_edge->tex[0] = edge->tex[0];
            }
            if(change_edge->tex[1] == edge->tex[1]){
                change_edge->tex[1] = edge->tex[0];
            }
            tex_edges[edge->tex[0]].insert(change_edge);
        }
        tex_edge_set.clear();
        if(edge->twin && (edge->tex[1] != edge->twin->tex[0] && edge->tex[0] != edge->twin->tex[1])){
            int rev_tex[2];
            rev_tex[0] = edge->twin->tex[1];
            rev_tex[1] = edge->twin->tex[0];
            CollapseTexPositions(tex_coords, rev_tex, edge->pos);
            CollapseParentRecord(tex_parents[rev_tex[0]], tex_parents[rev_tex[1]], edge->pos);
            HalfEdgeSet tex_edge_set = tex_edges[rev_tex[1]];
            for(auto change_edge : tex_edge_set){
                if(change_edge == edge || !change_edge->valid){
                    continue;
                }
                if(change_edge->tex[0] == rev_tex[1]){
                    change_edge->tex[0] = rev_tex[0];
                }
                if(change_edge->tex[1] == rev_tex[1]){
                    change_edge->tex[1] = rev_tex[0];
                }
                tex_edges[rev_tex[0]].insert(change_edge);
            }
            tex_edge_set.clear();
        }
    }
    HalfEdgeSet &affected_edges = vert_edges[edge->vert[0]];
    // Remove deleted edges
    CollapseHalfEdge(heap, edge);
    if(edge->twin){
        CollapseHalfEdge(heap, edge->twin);
    }
    // Recalc err and update heap
    for(auto affected_edge : affected_edges){
        if(!affected_edge->valid){
            continue;
        }
        int edge_tc[2];
        if(include_tex) {
            edge_tc[0] = GetNumTexCoords(affected_edge, 0);
            edge_tc[1] = GetNumTexCoords(affected_edge, 1);
        } else {
            edge_tc[0] = 0;
            edge_tc[1] = 0;
        }

        affected_edge->err = WOLFIRE_SIMPLIFY::CalculateError(&affected_edge->pos, affected_edge->vert, vertices, quadrics, edge_tc);

		HalfEdgeNode node;
		node.edge = affected_edge;
		
		HalfEdgeNodeHeap::iterator new_handle = heap.insert(node);
		heap.erase(affected_edge->handle);
		affected_edge->handle = new_handle;
		
		//heap.update(affected_edge->handle);
    }
}

void ReconstructModel(const std::vector<HalfEdge> &half_edges, SimplifyModel *model, bool include_tex){
    // Collapse vert edges
    std::vector<char> added(half_edges.size(), 0);
    model->vert_indices.clear();
    model->tex_indices.clear();
    for(int i=0, len=half_edges.size(); i<len; ++i){
        const HalfEdge* edge = &half_edges[i];
        if(!added[i] && edge->valid){
            model->vert_indices.push_back(edge->vert[0]);
            model->vert_indices.push_back(edge->next->vert[0]);
            model->vert_indices.push_back(edge->prev->vert[0]);
            if(include_tex) {
                model->tex_indices.push_back(edge->tex[0]);
                model->tex_indices.push_back(edge->next->tex[0]);
                model->tex_indices.push_back(edge->prev->tex[0]);
            }
            added[edge->id] = 1;
            added[edge->next->id] = 1;
            added[edge->prev->id] = 1;
        }
    }
}

bool SortEdgeSortable(const EdgeSortable& a, const EdgeSortable& b){
    if(a.verts[0] < b.verts[0]){
        return false;
    } else if(a.verts[0] > b.verts[0]){
        return true;
    } else if(a.verts[1] < b.verts[1]){
        return false;
    } else {
        return true;
    }
}

bool SortVertSortable(const VertSortable& a, const VertSortable& b){
    if(a.vert[0] == b.vert[0]){
        if(a.vert[1] == b.vert[1]){
            return a.vert[2] < b.vert[2];
        } else {
            return a.vert[1] < b.vert[1];
        }
    } else {
        return a.vert[0] < b.vert[0];
    }
}
