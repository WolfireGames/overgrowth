//-----------------------------------------------------------------------------
//           Name: modelsurfacewalker.cpp
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
#include "modelsurfacewalker.h"

#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Logging/logdata.h>

#include <map>
#include <cfloat>
#include <cstdlib>

namespace {
    struct Edge {
        int points[2];

        bool operator<(const Edge& other) const{
            if(points[0] < other.points[0]){
                return true;
            } else if(points[0] > other.points[0]){
                return false;
            }
            if(points[1] < other.points[1]){
                return true;    
            } else {
                return false;
            }
        }
        bool operator==(const Edge& other) const{
            return (points[0] == other.points[0] && points[1] == other.points[1]);
        }
    };

    struct Tri {
        Edge edges[3];
        bool edge_swapped[3];
    };

    bool line_intersect_2d(const vec2 &a, const vec2 &b, const vec2 &c, const vec2 &d, float &t) {
        float denom = ((d[1] - c[1])*(b[0] - a[0]) - (d[0] - c[0])*(b[1] - a[1]));
        if(denom == 0.0f){
            return false;
        }
        t = ((d[0] - c[0])*(a[1] - c[1]) - (d[1] - c[1])*(a[0] - c[0]))/denom;
        return true;
    }

    void GetVertexDuplicates( const std::vector<GLfloat> &vertices, std::vector<int> &first_dup ) {
        // For each vertex, store the id of the first vertex with the same position
        first_dup.resize(vertices.size()/3);
        std::map<vec3, int> point_map;
        std::map<vec3, int>::iterator iter;
        vec3 vec;
        int index = 0;
        for(int i=0, len=vertices.size()/3; i<len; i++){
            vec = vec3(vertices[index+0], vertices[index+1], vertices[index+2]);
            index += 3;
            iter = point_map.find(vec);
            if(iter == point_map.end()){
                point_map[vec] =  i;
                first_dup[i] = i;
            } else {
                first_dup[i] = iter->second;
            }
        }
    }
} // namespace ""

vec3 bary_coords(const vec2 &p, const vec2 &a, const vec2 &b, const vec2&c) {
    float denom = (b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1]);
    if(denom == 0.0f){
        return vec3(0.0f);
    }
    vec3 coord;
    coord[0] = ((b[1]-c[1])*(p[0]-c[0])+(c[0]-b[0])*(p[1]-c[1]))/denom;
    coord[1] = ((c[1]-a[1])*(p[0]-c[0])+(a[0]-c[0])*(p[1]-c[1]))/denom;
    coord[2] = 1.0f - coord[0] - coord[1];
    return coord;
}

void ModelSurfaceWalker::AttachTo(const std::vector<float> &vertices, const std::vector<unsigned> &faces) {
    std::vector<int> first_dup;
    GetVertexDuplicates(vertices, first_dup);

    // Store the edges of each triangle, with the vert ids in ascending order
    std::vector<Tri> tris(faces.size()/3);

    int index = 0;
    for(int i=0, len=tris.size(); i<len; ++i){
        for(int j=0; j<3; ++j) {
            for(int k=0; k<2; ++k){
                tris[i].edges[j].points[k] = first_dup[faces[index+(j+k)%3]];
            }
            if(tris[i].edges[j].points[0] > tris[i].edges[j].points[1]){
                std::swap(tris[i].edges[j].points[0], tris[i].edges[j].points[1]);
                tris[i].edge_swapped[j] = true;
            } else {
                tris[i].edge_swapped[j] = false;
            }
        }
        index += 3;
    }

    // Store a list of the triangles that share each edge
    std::map<Edge, std::vector<int> > edge_tris;
    for(int i=0, len=tris.size(); i<len; ++i){
        edge_tris[tris[i].edges[0]].push_back(i);
        edge_tris[tris[i].edges[1]].push_back(i);
        edge_tris[tris[i].edges[2]].push_back(i);
    }

    // Use the list of triangles for each edge to get a list of neighbors for
    // each triangle
    tri_info.resize(tris.size());
    for(std::map<Edge, std::vector<int> >::iterator iter = edge_tris.begin(); 
        iter != edge_tris.end(); 
        ++iter)
    {
        const Edge& edge = iter->first;
        const std::vector<int> &faces = iter->second;
        if(faces.size() == 2){
            for(unsigned i=0; i<2; ++i){
                const int &face = faces[i];
                int which_edge = 0;
                if(tris[face].edges[1] == edge){
                    which_edge = 1;
                }
                if(tris[face].edges[2] == edge){
                    which_edge = 2;
                }
                tri_info[face].neighbors[which_edge] = faces[1-i];
                tri_info[face].edge_swapped[which_edge] = tris[face].edge_swapped[which_edge];
            }
        }
    }

    // Get the length of each edge of each triangle
    vec3 points[3];
    int face_index = 0;
    for(unsigned i=0; i<tri_info.size(); ++i){
        for(unsigned j=0; j<3; ++j){
            points[j] = vec3(vertices[faces[face_index]*3+0],
                             vertices[faces[face_index]*3+1],
                             vertices[faces[face_index]*3+2]);
            ++face_index;
        }
        tri_info[i].edge_length[0] = distance(points[0], points[1]);
        tri_info[i].edge_length[1] = distance(points[1], points[2]);
        tri_info[i].edge_length[2] = distance(points[2], points[0]);
    }
}

int ModelSurfaceWalker::GetNeighbor( int tri, int neighbor ) {
    return tri_info[tri].neighbors[neighbor];
}

SWresults ModelSurfaceWalker::Move( SurfaceWalker& sw, vec3* points, const vec3& dir, float drip_dist, std::vector<WalkLine>* trace ) {
    // Get transformed edges;
    vec3 edges[3];
    edges[0] = points[1]-points[0];
    edges[1] = points[2]-points[1];
    edges[2] = points[0]-points[2];

    // Use normal and first edge to construct local space
    vec3 normal = normalize(cross(edges[0], edges[1]));
    vec3 right = normalize(cross(normal, edges[0]));
    vec3 up = normalize(edges[0]);

    // Convert points and edges to local space
    vec2 points_local[3];
    for(unsigned i=0; i<3; ++i){
        vec3 temp = points[i]-points[0];
        points_local[i] = vec2(dot(temp, right), 
                               dot(temp, up));
    }
    vec2 edge_local[3];
    for(unsigned i=0; i<3; ++i){
        edge_local[i] = points_local[(i+1)%3]-points_local[i];
    }

    // Get inner edge normals
    vec2 edge_local_normal[3];
    for(unsigned i=0; i<3; ++i){
        edge_local_normal[i] = vec2(edge_local[i][1], -edge_local[i][0]);
    }

    // Calculate surface walker movement in local space
    vec2 old_pos_local = sw.pos[1] * points_local[1] + 
                         sw.pos[2] * points_local[2];
    vec2 dir_local = normalize(vec2(dot(dir, right), dot(dir, up)));
    
    // Are we exiting the edge we came in on? If so, run along edge instead
    bool drip_down_edge = false;
    if(sw.on_edge != -1 && 
       dot(dir_local, edge_local_normal[sw.on_edge]) <= 0.0f)
    {
        vec2 normalized_edge = normalize(edge_local[sw.on_edge]);
        dir_local = normalized_edge * dot(normalized_edge, dir_local);
        drip_down_edge = true;
        //printf("Drip down edge\n");
    }

    // Predict drip position
    vec2 new_pos_local = old_pos_local + dir_local * drip_dist;

    // Check for edge intersections
    bool edge_intersected[3] = {false, false, false};
    float edge_intersect_time[3];
    for(unsigned i=0; i<3; ++i){
        if(dot(dir_local, edge_local_normal[i]) >= 0.0f)
        {
            continue;
        }
        bool valid = line_intersect_2d(old_pos_local, 
                                       new_pos_local,
                                       points_local[i],
                                       points_local[i] + edge_local[i],
                                       edge_intersect_time[i]);
        if(valid && 
           edge_intersect_time[i] >= 0.0f &&
           edge_intersect_time[i] <= 1.0f)
        {
            edge_intersected[i] = true;
        }
    }

    int cie = -1; // Closest intersected edge
    float closest_time = FLT_MAX;
    for(unsigned i=0; i<3; ++i){
        if(edge_intersected[i] && 
           (cie == -1 || 
            edge_intersect_time[i] < closest_time))
        {
            cie = i;
            closest_time = edge_intersect_time[i];
        }
    }

    // If intersection occured, update surface walker to move to neighboring 
    // triangle
    if(cie != -1){
        if(tri_info[sw.tri].neighbors[cie] == -1){
            SWresults results;
            results.dist_moved = 1.0f;
            return results;
        }
        new_pos_local = old_pos_local + dir_local * drip_dist * closest_time;
        if(drip_down_edge){
            SWresults results;
            results.dist_moved = closest_time;
            float dists[3];
            for(int i=0; i<3; ++i){
                dists[i] = distance_squared(new_pos_local, points_local[i]);
            }
            if(dists[0] < dists[1] && dists[0] < dists[2]){
                results.at_point = 0;
            } else if(dists[1] < dists[2]){
                results.at_point = 1;
            } else {
                results.at_point = 2;
            }
            return results;
            LOGI << "Drip intersection" << std::endl;
        }
        trace->resize(trace->size()+1);
        trace->back().tri = sw.tri;
        trace->back().start = sw.pos;
        trace->back().end = bary_coords(new_pos_local, points_local[0], points_local[1], points_local[2]);
        if(sw.last_tri == tri_info[sw.tri].neighbors[cie]){
            //printf("Backwards progress\n");
        }
        sw.last_tri = sw.tri;
        sw.tri = tri_info[sw.tri].neighbors[cie];
        int reverse_edge = 0;
        bool reverse_edge_swapped = false;
        for(unsigned i=0; i<3; ++i){
            if(tri_info[sw.tri].neighbors[i] == sw.last_tri){
                reverse_edge = i;
                reverse_edge_swapped = tri_info[sw.tri].edge_swapped[i] != 
                                       tri_info[sw.last_tri].edge_swapped[cie];
            }
        }
        vec2 val;
        val[0] = distance(new_pos_local, points_local[(cie + 1)%3]) /
                 length(edge_local[cie]);
        val[1] = 1.0f - val[0];
        sw.pos = vec3(0.0f);
        sw.on_edge = reverse_edge;

        if(reverse_edge_swapped){
            sw.pos[reverse_edge] = val[1];
            sw.pos[(reverse_edge+1)%3] = val[0];
        } else {
            sw.pos[reverse_edge] = val[0];
            sw.pos[(reverse_edge+1)%3] = val[1];
        }
        SWresults results;
        results.dist_moved = closest_time;
        return results;
    }

    trace->resize(trace->size()+1);
    trace->back().tri = sw.tri;
    trace->back().start = sw.pos;
    trace->back().end = bary_coords(new_pos_local, points_local[0], points_local[1], points_local[2]);
    // If no intersection occured, keep moving on current triangle, and convert
    // local coords back into barycentric space
    sw.pos = bary_coords(new_pos_local, points_local[0], points_local[1], points_local[2]);
    SWresults results;
    results.dist_moved = 1.0f;
    if(!drip_down_edge){
        sw.on_edge = -1;
    }
    return results;
}

MSWTriInfo::MSWTriInfo() {
    neighbors[0] = -1;
    neighbors[1] = -1;
    neighbors[2] = -1;
}

SurfaceWalker::SurfaceWalker():
    on_edge(-1),
    delay(0.0f)
{}

SWresults::SWresults():
    at_point(-1)
{}
