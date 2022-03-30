//-----------------------------------------------------------------------------
//           Name: simplify.cpp
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
#include "simplify.hpp"

#include <Graphics/halfedge.h>
#include <Graphics/model.h>
#include <Logging/logdata.h>
#include <Wrappers/glm.h>

#include <algorithm>
#include <list>
#include <vector>
#include <set>

using namespace WOLFIRE_SIMPLIFY;

namespace {
    struct TextureEdge {
        int id[2];
        bool swapped;
    };
    /*
    bool EdgeCompare(TextureEdge &a, TextureEdge &b){
        if(a.id[0] != b.id[0]){
            return a.id[0] < b.id[0];
        } else {
            return a.id[1] < b.id[1];
        }
    }
    */
    struct EdgeCollapseRecord {
        int vert[2];
        float pos;
    };

    class VertTris {
    public:
        void Initialize(const std::vector<float> &vertices, const std::vector<int> &vert_indices){
            // Make space to store the number of triangles connected to each vertex
            unsigned num_verts = vertices.size()/3;
            num_vert_tris.clear();
            num_vert_tris.resize(num_verts, 0);
            // Loop through vertex indices and increment triangle count for each vertex
            unsigned num_vert_indices = vert_indices.size();
            for(unsigned i=0; i<num_vert_indices; ++i){
                ++num_vert_tris[vert_indices[i]];
            }
            // Index list of vertex triangles for fast lookup
            int index = 0;
            vert_tris_index.clear();
            vert_tris_index.resize(num_verts);
            for(unsigned i=0; i<num_verts; ++i){
                vert_tris_index[i] = index;
                index += num_vert_tris[i];
                num_vert_tris[i] = 0; // Reset so we can use this to fill vert_tris
            }
            // Fill list of triangles for each vertex
            vert_tris.clear();
            vert_tris.resize(num_vert_indices);
            for(unsigned i=0; i<num_vert_indices; ++i){
                int vert = vert_indices[i];
                int &nvt = num_vert_tris[vert];
                vert_tris[vert_tris_index[vert]+nvt] = i;
                ++nvt;
            }
        }

        // Get number of tris that include vertex
        inline int NumTrisConnectedToVert(int i) const {
            return num_vert_tris[i];
        }

        // Get array of tris that include vertex
        // The int is the index of vert_indices, so the tri
        // id is really vert_tris[i]%3
        inline const int* GetTrisConnectedToVert(int i) const {
            return &vert_tris[vert_tris_index[i]];
        }
    private:
        std::vector<int> num_vert_tris;
        std::vector<int> vert_tris;
        std::vector<int> vert_tris_index;
    };
} // namespace ""

namespace {
    struct TexSort {
        glm::vec2 coord;
        int old_index;
        int old_id;
    };

    struct VertSort {
        glm::vec3 coord;
        int old_index;
        int old_id;
    };

    bool operator<(const TexSort &a, const TexSort &b) {
        if(a.coord[0] == b.coord[0]){
            return a.coord[1] < b.coord[1];
        } else {
            return a.coord[0] < b.coord[0];
        }
    }

    bool operator<(const VertSort &a, const VertSort &b) {
        if(a.coord[0] == b.coord[0]){
            if(a.coord[1] == b.coord[1]){
                return a.coord[2] < b.coord[2];
            } else {
                return a.coord[1] < b.coord[1];
            }
        } else {
            return a.coord[0] < b.coord[0];
        }
    }
} // namespace ""

static void GetTexCoordNumPerVertex(const SimplifyModel *model, std::vector<int> &num_tc){
    // Count number of times each vertex is called in vert_indices
    num_tc.clear();
    num_tc.resize(model->vertices.size()/3, 0);
    for(int i=0, len=model->vert_indices.size(); i<len; ++i){
        if(model->vert_indices[i] != -1){
            ++num_tc[model->vert_indices[i]];
        }
    }
    // Index for fast access
    std::vector<int> tc_index(num_tc.size(), 0);
    for(int i=0, len=num_tc.size(), index=0; i<len; ++i){
        tc_index[i] = index;
        index += num_tc[i];
    }
    // Clear num_tc for reuse
    for(int i=0, len=num_tc.size(); i<len; ++i){
        num_tc[i] = 0;
    }
    // Record texture indices
    std::vector<int> tc(model->vert_indices.size());
    for(int i=0, len=model->vert_indices.size(); i<len; ++i){
        int vert = model->vert_indices[i];
        if(vert != -1){
            tc[tc_index[vert]+num_tc[vert]] = model->tex_indices[i];
            ++num_tc[vert];
        }
    }
    // Sort texture indices
    for(int i=0, len=num_tc.size(); i<len; ++i){
        std::sort(tc.begin()+tc_index[i], tc.begin()+tc_index[i]+num_tc[i]);
    }
    // Clear num_tc for reuse
    for(int i=0, len=num_tc.size(); i<len; ++i){
        if(num_tc[i] != 0) {
            int count = 1;
            for(int j=1, index=tc_index[i]+1; j<num_tc[i]; ++j, ++index){
                if(tc[index] != tc[index-1]){
                    ++count;
                }
            }
            num_tc[i] = count;
        }
    }
}

static void ProcessModel( const SimplifyModelInput &smi, SimplifyModel *model, float merge_threshold, bool include_tex) {
    // Create sorted list of vertices
    int num_indices = smi.vertices.size()/3;
    std::vector<VertSort> sorted_verts(num_indices);
    std::vector<TexSort> sorted_tex(num_indices);
    for(int i=0, vci=0, tci=0; i<num_indices; ++i, vci+=3, tci+=2){
        for(int k=0; k<3; ++k){
            sorted_verts[i].coord[k] = smi.vertices[vci+k];
        }
        if(include_tex) {
            for(int k=0; k<2; ++k){
                sorted_tex[i].coord[k] = smi.tex_coords[tci+k];
            }
        }
        sorted_verts[i].old_index = i;
        sorted_verts[i].old_id = smi.old_vert_id[i];
        if(include_tex) {
            sorted_tex[i].old_index = i;
            sorted_tex[i].old_id = smi.old_tex_id[i];
        }
    }
    std::sort(sorted_verts.begin(), sorted_verts.end());
    if(include_tex) {
        std::sort(sorted_tex.begin(), sorted_tex.end());
    }
    // Remove duplicate vertices and form vertex index list
    std::vector<int> vert_indices(num_indices);
    if(!sorted_verts.empty()) {
        vert_indices[sorted_verts[0].old_index] = 0;
        int index = 1;
        for(int i=1, len=sorted_verts.size(); i<len; ++i){
            // Check if squared distance between vertices is greater than merge_threshold
            glm::vec3 vec = sorted_verts[i].coord - sorted_verts[i-1].coord;
            if(glm::dot(vec, vec) > merge_threshold) {
                sorted_verts[index].coord = sorted_verts[i].coord;
                sorted_verts[index].old_id = sorted_verts[i].old_id;
                ++index;
            }
            vert_indices[sorted_verts[i].old_index] = index-1;
        }
        sorted_verts.resize(index);
    }

    // Remove duplicate tex coords and form tex index list
    std::vector<int> tex_indices(num_indices);
    if(include_tex) {
        if(!sorted_tex.empty()) {
            tex_indices[sorted_tex[0].old_index] = 0;
            int index = 1;
            for(int i=1, len=sorted_tex.size(); i<len; ++i){
                if(sorted_tex[i].coord != sorted_tex[i-1].coord) {
                    sorted_tex[index].coord = sorted_tex[i].coord;
                    sorted_tex[index].old_id = sorted_tex[i].old_id;
                    ++index;
                }
                tex_indices[sorted_tex[i].old_index] = index-1;
            }
            sorted_tex.resize(index);
        }
    }

    // Remove degenerate triangles
    int garbage_index = num_indices;
    for(int i=0; i<garbage_index; i+=3){
        while(i<=garbage_index &&
            (vert_indices[i+0] == vert_indices[i+1] ||
            vert_indices[i+1] == vert_indices[i+2] ||
            vert_indices[i+2] == vert_indices[i+0])) 
        {
            garbage_index -= 3;
            for(int j=0; j<3; ++j){
                vert_indices[i+j] = vert_indices[garbage_index+j];
            }
            if(include_tex) {
                for(int j=0; j<3; ++j){
                    tex_indices[i+j] = tex_indices[garbage_index+j];
                }
            }
        }
    }
    vert_indices.resize(garbage_index);
    if(include_tex) {
        tex_indices.resize(garbage_index);
    }
    // Copy processed info into SimplifyModel
    if(include_tex) {
        model->tex_indices = tex_indices; 
    }
    model->vert_indices = vert_indices; 
    model->vertices.resize(sorted_verts.size()*3);
    model->old_vert_id.resize(sorted_verts.size());
    for(int i=0, len=sorted_verts.size(), index=0; i<len; ++i, index+=3){
        for(int j=0; j<3; ++j){
            model->vertices[index+j] = sorted_verts[i].coord[j];
        }
        model->old_vert_id[i] = sorted_verts[i].old_id;
    }

    if(include_tex) {
        model->tex_coords.resize(sorted_tex.size()*2);
        model->old_tex_id.resize(sorted_tex.size());
        for(int i=0, len=sorted_tex.size(), index=0; i<len; ++i, index+=2){
            for(int j=0; j<2; ++j){
                model->tex_coords[index+j] = sorted_tex[i].coord[j];
            }
            model->old_tex_id[i] = sorted_tex[i].old_id;
        }
    }
}

static bool GenerateEdgePairs(WOLFIRE_SIMPLIFY::SimplifyModel& processed_model, vector<HalfEdge>& half_edges, bool include_tex) {
    half_edges.resize(processed_model.vert_indices.size());
    LOGI << "Setting up half-edges..." << std::endl;
    // Set up edges
    {
        int edge_index = 0;
        for(size_t i=0, len=processed_model.vert_indices.size(); i<len; i+=3){
            for(int j=0; j<3; ++j){
                HalfEdge he;
                he.vert[0] = processed_model.vert_indices[i+j];
                he.vert[1] = processed_model.vert_indices[i+(j+1)%3];
                if(include_tex) {
                    he.tex[0] = processed_model.tex_indices[i+j];
                    he.tex[1] = processed_model.tex_indices[i+(j+1)%3];
                }
                he.next = &half_edges[edge_index+(j+1)%3];
                he.prev = &half_edges[edge_index+(j+2)%3];
                he.twin = NULL;
                he.err = UNDEFINED_ERROR;
                he.id = edge_index + j;
                he.valid = true;
                half_edges[edge_index+j] = he;
            }
            edge_index += 3;
        }
    }
    LOGI << "Finding edge pairs..." << std::endl;
    // Find pairs
    {
        std::vector<HalfEdge> half_edge_pairs = half_edges;
        std::sort(half_edge_pairs.begin(), half_edge_pairs.end(), HalfEdgePairFind);

        for(size_t i=1, len=half_edge_pairs.size(); i<len; ++i){
            if(half_edge_pairs[i].vert[0] == half_edge_pairs[i-1].vert[1] &&
                half_edge_pairs[i].vert[1] == half_edge_pairs[i-1].vert[0])
            {
                half_edges[half_edge_pairs[i].id].twin = &half_edges[half_edge_pairs[i-1].id];
                half_edges[half_edge_pairs[i-1].id].twin = &half_edges[half_edge_pairs[i].id];
            }
        }
    }
    LOGI << "Validating edge pairs." << std::endl;
    bool missing_pair = false;
    bool invalid_pair = false;
    for(size_t i=0, len=half_edges.size(); i<len; ++i){
        if(half_edges[i].twin == NULL){
            missing_pair = true;
        } else if(half_edges[i].twin->twin == NULL || half_edges[i].twin->twin != &half_edges[i]){
            invalid_pair = true;
        }
    }
    if(!missing_pair && !invalid_pair){
        LOGI << "All edge pairs are valid!" << std::endl;
    } else if(missing_pair && !invalid_pair){
        LOGI << "All edge pairs are valid but some are missing." << std::endl;
    } else if(!missing_pair && invalid_pair){
        LOGI << "No edge pairs are missing but some are INVALID." << std::endl;
    } else if(missing_pair && invalid_pair){
        LOGI << "MISSING AND INVALID EDGE PAIRS." << std::endl;
    }
    if(invalid_pair){
        LOGI << "Aborting simplification due to invalid edge pairs" << std::endl;
        return false;
    }
    return true;
}

static void CalculateEdgeErrors(vector<HalfEdge>& half_edges, SimplifyModel& processed_model, vector<glm::mat4>& quadrics_, bool include_tex) {
    LOGI << "Calculating edge error..." << std::endl;
    for(int i=0, len=half_edges.size(); i<len; ++i){
        HalfEdge& edge = half_edges[i];
        if(edge.err == UNDEFINED_ERROR){
            int edge_tc[2];
            if(include_tex) {
                edge_tc[0] = GetNumTexCoords(&edge, 0);
                edge_tc[1] = GetNumTexCoords(&edge, 1);
            } else {
                edge_tc[0] = 0;
                edge_tc[1] = 0;
            }
            edge.err = WOLFIRE_SIMPLIFY::CalculateError(&edge.pos, edge.vert, processed_model.vertices, quadrics_, edge_tc);
            if(edge.twin){
                edge.twin->err = edge.err;
                edge.twin->pos = 1.0f - edge.pos;
            }
        }
    }
}

static void InitHeap(vector<HalfEdge>& half_edges, HalfEdgeNodeHeap& heap, HalfEdgeSetVec& vert_edges, HalfEdgeSetVec& tex_edges, bool include_tex) {
    LOGI << "Adding edges to heap..." << std::endl;
    for(int i=0, len=half_edges.size(); i<len; ++i){
        HalfEdgeNode node;
        node.edge = &half_edges[i];
        half_edges[i].handle = heap.insert(node);
    }
    for(int i=0, len=half_edges.size(); i<len; ++i){
        HalfEdge *edge = &half_edges[i];
        vert_edges[edge->vert[0]].insert(edge);
        vert_edges[edge->vert[1]].insert(edge);
        if(include_tex) {
            tex_edges[edge->tex[0]].insert(edge);
            tex_edges[edge->tex[1]].insert(edge);
        }
    }
}

static void InitParents(ParentRecordListVec& vert_parents, ParentRecordListVec& tex_parents, bool include_tex) {
    for(int i=0, len=vert_parents.size(); i<len; ++i){
        ParentRecord pr;
        pr.id = i;
        pr.weight = 1.0f;
        vert_parents[i].push_back(pr);
    }
    if(include_tex) {
        for(int i=0, len=tex_parents.size(); i<len; ++i){
            ParentRecord pr;
            pr.id = i;
            pr.weight = 1.0f;
            tex_parents[i].push_back(pr);
        }
    }
}

static void ToModel(SimplifyModel& from_simplify_model, Model* to_model, bool include_tex) {
    to_model->vertices.clear();
    to_model->tex_coords.clear();
    to_model->faces.clear();

    for(int i = 0; i < from_simplify_model.vert_indices.size(); i++) {
        for(int k = 0; k < 3; k++) {
            to_model->vertices.push_back(from_simplify_model.vertices[from_simplify_model.vert_indices[i] * 3 + k]);
        }

        if(include_tex) {
            if(i < from_simplify_model.tex_indices.size()) {
                for(int k = 0; k < 2; k++) {
                    to_model->tex_coords.push_back(from_simplify_model.tex_coords[from_simplify_model.tex_indices[i] * 2 + k]);
                }
            }
        }
        to_model->faces.push_back(i);
    }
}

float WOLFIRE_SIMPLIFY::CalculateError(float* pos, int edge[2], const std::vector<float> &vertices, const std::vector<glm::mat4> &quadrics, const int edge_tc[]) {
    int vert_index[2];
    vert_index[0] = edge[0]*3;
    vert_index[1] = edge[1]*3;
    glm::vec4 vert_vec[2];
    vert_vec[0] = glm::vec4(vertices[vert_index[0]+0], 
        vertices[vert_index[0]+1], 
        vertices[vert_index[0]+2],
        1.0f);
    vert_vec[1] = glm::vec4(vertices[vert_index[1]+0], 
        vertices[vert_index[1]+1], 
        vertices[vert_index[1]+2],
        1.0f);
    float quadric_error = 0;
    glm::mat4 total_quadric = quadrics[edge[0]] + quadrics[edge[1]];
    if(edge_tc[0] > edge_tc[1]){
        *pos = 0.0f;
        quadric_error = glm::dot(vert_vec[0], total_quadric * vert_vec[0]);
    } else if(edge_tc[0] < edge_tc[1]){
        *pos = 1.0f;
        quadric_error = glm::dot(vert_vec[1], total_quadric * vert_vec[1]);
    } else {
        //glm::vec4 offset = vert_vec[1] - vert_vec[0];
        float best_err = FLT_MAX;
        for(int i=0; i<=10; ++i){
            float amount = i*0.1f; 
            glm::vec4 temp_vec;
            for(int j=0; j<3; ++j){
                temp_vec[j] = vertices[vert_index[0]+j] * (1.0f - amount) + vertices[vert_index[1]+j] * amount;
            }
            temp_vec[3] = 1.0f;
            float err = glm::dot(temp_vec, total_quadric * temp_vec);
            if(i==0 || err<best_err){
                best_err = err;
                *pos = amount;
            }
        }
        quadric_error = best_err;
    }
    return quadric_error;
}

void WOLFIRE_SIMPLIFY::CalculateQuadrics(std::vector<glm::mat4> *quadrics, const std::vector<float> &vertices, const std::vector<int> &vert_indices){
    VertTris vert_tris;
    vert_tris.Initialize(vertices, vert_indices);
    int num_verts = vertices.size()/3;
    quadrics->clear();
    quadrics->resize(num_verts, glm::mat4(0.0f));
    for(int i=0, len = num_verts; i<len; ++i){
        int num_tris = vert_tris.NumTrisConnectedToVert(i);
        const int *tris = vert_tris.GetTrisConnectedToVert(i);
        for(int j=0; j<num_tris; ++j) {
            glm::vec3 vec[3];
            int tri = tris[j]/3;
            for(int k=0; k<3; ++k){
                int vert_index = vert_indices[tri*3+k]*3;
                vec[k] = glm::vec3(vertices[vert_index+0],
                    vertices[vert_index+1],
                    vertices[vert_index+2]);
            }
            glm::vec3 cross_prod = glm::cross(vec[2]-vec[0], vec[1]-vec[0]);
            float area = glm::length(cross_prod) * 0.5f;
            if(area != 0.0f){
                glm::vec3 normal = glm::normalize(cross_prod);
                float offset = glm::dot(normal, vec[0]);
                glm::vec4 plane = glm::vec4(normal[0], normal[1], normal[2], offset * -1.0f);
                glm::mat4 quadric;
                quadric[0][0] = plane[0] * plane[0];
                quadric[0][1] = plane[0] * plane[1];
                quadric[0][2] = plane[0] * plane[2];
                quadric[0][3] = plane[0] * plane[3];
                quadric[1][1] = plane[1] * plane[1];
                quadric[1][2] = plane[1] * plane[2];
                quadric[1][3] = plane[1] * plane[3];
                quadric[2][2] = plane[2] * plane[2];
                quadric[2][3] = plane[2] * plane[3];
                quadric[3][3] = plane[3] * plane[3];
                quadric[1][0] = quadric[0][1];
                quadric[2][0] = quadric[0][2];
                quadric[2][1] = quadric[1][2];
                quadric[3][0] = quadric[0][3];
                quadric[3][1] = quadric[1][3];
                quadric[3][2] = quadric[2][3];
                for(int a=0; a<4; ++a){
                    for(int b=0; b<4; ++b){
                        quadric[a][b] *= area;
                    }
                }
                quadrics->at(i) += quadric;
            }
        }
    }
}

bool WOLFIRE_SIMPLIFY::Process(const Model &model, WOLFIRE_SIMPLIFY::SimplifyModel& processed_model, std::vector<HalfEdge>& half_edges, bool include_tex) {
    WOLFIRE_SIMPLIFY::SimplifyModelInput model_input;
    // Prepare model to pass to simplify algorithm
    for(size_t i=0, len=model.faces.size(); i<len; ++i){
        int vert_index = model.faces[i]*3;
        for(int j=0; j<3; ++j){
            model_input.vertices.push_back(model.vertices[vert_index+j]);
        }
        model_input.old_vert_id.push_back(model.faces[i]);
        if(include_tex) {
            int tex_index = model.faces[i]*2;
            for(int j=0; j<2; ++j){
                model_input.tex_coords.push_back(model.tex_coords[tex_index+j]);
            }
            model_input.old_tex_id.push_back(model.faces[i]);
        }
    }

    LOGI << "Starting to simplify mesh..." << std::endl;
    ProcessModel(model_input, &processed_model, 0.0f, include_tex);

    return GenerateEdgePairs(processed_model, half_edges, include_tex);
}

bool WOLFIRE_SIMPLIFY::SimplifySimpleModel(const Model& input, Model* output, int edge_target, bool include_tex) {
    SimplifyModelInput smi;
    for(int i=0, len=input.faces.size(); i<len; ++i){
        int vert_index = input.faces[i]*3;
        for(int j=0; j<3; ++j){
            smi.vertices.push_back(input.vertices[vert_index+j]);
        }
        smi.old_vert_id.push_back(input.faces[i]);

        if(include_tex) {
            int tex_index = input.faces[i]*2;
            for(int j=0; j<2; ++j){
                smi.tex_coords.push_back(input.tex_coords[tex_index+j]);
            }

            smi.old_tex_id.push_back(input.faces[i]);
        }
    }

    LOGI << "Starting to simplify mesh..." << std::endl;
    SimplifyModel processed_model;
    std::vector<HalfEdge> half_edges;
    ProcessModel(smi, &processed_model, 0.0f, include_tex);

    bool edge_pair_res = GenerateEdgePairs(processed_model, half_edges, include_tex);

    if(edge_pair_res == false) return false;

    LOGI << "Calculating quadrics..." << std::endl;
    // Calculate quadrics and edge error
    std::vector<glm::mat4> quadrics_;
    WOLFIRE_SIMPLIFY::CalculateQuadrics(&quadrics_, processed_model.vertices, processed_model.vert_indices);

    CalculateEdgeErrors(half_edges, processed_model, quadrics_, include_tex);

    // Add edges to heap
    HalfEdgeNodeHeap heap;
    HalfEdgeSetVec vert_edges(processed_model.vertices.size()/3);
    HalfEdgeSetVec tex_edges(processed_model.tex_coords.size()/2);

    InitHeap(half_edges, heap, vert_edges, tex_edges, include_tex);

    // Get parents
    ParentRecordListVec vert_parents(processed_model.vertices.size()/3);
    ParentRecordListVec tex_parents(processed_model.tex_coords.size()/2);
    InitParents(vert_parents, tex_parents, include_tex);

    vector<float> vert_target;
    vector<float> tex_target;

    LOGI << "Collapsing edges... heap size: " << heap.size() << " edge_target: " << edge_target << " half_edges: " << half_edges.size() << std::endl;
    int starting_heap_size = heap.size();
    int count = 0;
    while(!heap.empty() && (int)heap.size() > edge_target){
        HalfEdge* lowest_err_edge = heap.begin()->edge;
        if(lowest_err_edge->valid){
            CollapseEdge(heap, lowest_err_edge, processed_model.vertices, processed_model.tex_coords, quadrics_, vert_parents, tex_parents, vert_edges, tex_edges, include_tex);
        } else {
            heap.erase(heap.begin());
        }
        if(count > 1000) {
            LOGI << "Collapsing " << 100 - ((heap.size() - edge_target) / ((starting_heap_size - edge_target)/100)) << "%..." << std::endl;
            count = 0;
        }
        count++;
    }
    LOGI << "Done collapsing ... heap size: " << heap.size() << " edge_target: " << edge_target << " half_edges: " << half_edges.size() << std::endl;

    ReconstructModel(half_edges, &processed_model, include_tex);

    vert_target.resize(processed_model.vertices.size());
    std::vector<int> reverse_vert_parents(vert_parents.size()), reverse_tex_parents(tex_parents.size());
    for(int i=0, len=vert_parents.size(); i<len; ++i) {
        for(ParentRecordList::iterator iter = vert_parents[i].begin(); iter != vert_parents[i].end(); ++iter) {
            ParentRecord &pr = (*iter);
            for(int j=0; j<3; ++j){
                vert_target[pr.id*3+j] = processed_model.vertices[i*3+j];
            }
            reverse_vert_parents[pr.id] = i;
        }
    }

    tex_target.resize(processed_model.tex_coords.size());
    for(int i=0, len=tex_parents.size(); i<len; ++i) {
        for(ParentRecordList::iterator iter = tex_parents[i].begin(); iter != tex_parents[i].end(); ++iter) {
            ParentRecord &pr = (*iter);
            for(int j=0; j<2; ++j){
                tex_target[pr.id*2+j] = processed_model.tex_coords[i*2+j];
            }
            reverse_tex_parents[pr.id] = i;
        }
    }

    for(int i=0, len=processed_model.vert_indices.size(); i<len; ++i){
        if(vert_parents[processed_model.vert_indices[i]].empty()) {
            processed_model.vert_indices[i] = reverse_vert_parents[processed_model.vert_indices[i]];
        }
    }

    for(int i=0, len=processed_model.tex_indices.size(); i<len; ++i){
        if(tex_parents[processed_model.tex_indices[i]].empty()){
            processed_model.tex_indices[i] = reverse_tex_parents[processed_model.tex_indices[i]];
        }
    }

    LOGI << "Simplification completed." << std::endl;

    ToModel(processed_model, output, include_tex);

    return true;
}

bool WOLFIRE_SIMPLIFY::SimplifyMorphLOD( const SimplifyModelInput &model_input, MorphModel* lod, ParentRecordListVec *vert_parent_vec , ParentRecordListVec *tex_parent_vec, int lod_levels) {
    bool include_tex = true;
    LOGI << "Starting to simplify mesh..." << std::endl;
    SimplifyModel processed_model;
    std::vector<HalfEdge> half_edges;
    ProcessModel(model_input, &processed_model, 0.0f, include_tex );

    bool edge_pair_res = GenerateEdgePairs(processed_model, half_edges, include_tex);

    if(edge_pair_res == false) return false;

    LOGI << "Calculating quadrics..." << std::endl;
    // Calculate quadrics and edge error
    std::vector<glm::mat4> quadrics_;
    WOLFIRE_SIMPLIFY::CalculateQuadrics(&quadrics_, processed_model.vertices, processed_model.vert_indices);

    CalculateEdgeErrors(half_edges, processed_model, quadrics_, include_tex);

    // Add edges to heap
    HalfEdgeNodeHeap heap;
    HalfEdgeSetVec vert_edges(processed_model.vertices.size()/3);
    HalfEdgeSetVec tex_edges(processed_model.tex_coords.size()/2);
    InitHeap(half_edges, heap, vert_edges, tex_edges, include_tex);

    // Get parents
    ParentRecordListVec vert_parents(processed_model.vertices.size()/3);
    ParentRecordListVec tex_parents(processed_model.tex_coords.size()/2);
    InitParents(vert_parents, tex_parents, include_tex);

    int curr_lod = 0;
    int tri_threshold = heap.size()/2;
    SimplifyModel last_lod_model = processed_model;
    SimplifyModel working_model = processed_model;
    LOGI << "Collapsing edges..." << std::endl;
    while(!heap.empty()){
        if(curr_lod < lod_levels && (int)heap.size() <= tri_threshold){
            ReconstructModel(half_edges, &working_model, include_tex);
            lod[curr_lod].model = last_lod_model;
            lod[curr_lod].vert_target.resize(working_model.vertices.size());
            std::vector<int> reverse_vert_parents(vert_parents.size()), reverse_tex_parents(tex_parents.size());
            for(int i=0, len=vert_parents.size(); i<len; ++i){
                for(ParentRecordList::iterator iter = vert_parents[i].begin(); iter != vert_parents[i].end(); ++iter){
                    ParentRecord &pr = (*iter);
                    for(int j=0; j<3; ++j){
                        lod[curr_lod].vert_target[pr.id*3+j] = working_model.vertices[i*3+j];
                    }
                    reverse_vert_parents[pr.id] = i;
                }
            }
            lod[curr_lod].tex_target.resize(working_model.tex_coords.size());
            for(int i=0, len=tex_parents.size(); i<len; ++i){
                for(ParentRecordList::iterator iter = tex_parents[i].begin(); iter != tex_parents[i].end(); ++iter){
                    ParentRecord &pr = (*iter);
                    for(int j=0; j<2; ++j){
                        lod[curr_lod].tex_target[pr.id*2+j] = working_model.tex_coords[i*2+j];
                    }
                    reverse_tex_parents[pr.id] = i;
                }
            }
            vert_parent_vec[curr_lod] = vert_parents;
            tex_parent_vec[curr_lod] = tex_parents;            
            for(int i=0, len=working_model.vert_indices.size(); i<len; ++i){
                if(vert_parents[working_model.vert_indices[i]].empty()){
                    working_model.vert_indices[i] = reverse_vert_parents[working_model.vert_indices[i]];
                }
            }
            for(int i=0, len=working_model.tex_indices.size(); i<len; ++i){
                if(tex_parents[working_model.tex_indices[i]].empty()){
                    working_model.tex_indices[i] = reverse_tex_parents[working_model.tex_indices[i]];
                }
            }
            last_lod_model = working_model;
            tri_threshold /= 2;
            ++curr_lod;
        }
        HalfEdge* lowest_err_edge = heap.begin()->edge;
        if(lowest_err_edge->valid){
            CollapseEdge(heap, lowest_err_edge, working_model.vertices, working_model.tex_coords, quadrics_, vert_parents, tex_parents, vert_edges, tex_edges, include_tex);
        } else {
            heap.erase(heap.begin());
        }
    }
    LOGI << "Simplification completed." << std::endl;
    return true;
}
