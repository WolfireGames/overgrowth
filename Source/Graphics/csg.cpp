//-----------------------------------------------------------------------------
//           Name: csg.cpp
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
#include "csg.h"

#include <Graphics/model.h>
#include <Graphics/models.h>
#include <Graphics/pxdebugdraw.h>

#include <Math/enginemath.h>
#include <Math/triangle.h>
#include <Math/vec2math.h>

#include <Physics/bulletworld.h>
#include <Memory/allocation.h>
#include <Logging/logdata.h>
#include <Utility/compiler_macros.h>

#include <opengl.h>

#include <algorithm>
#include <vector>

namespace Test {
    struct HalfEdge {
        int points[2];
        HalfEdge *twin, *next;
    };

    struct HalfEdgeSort {
        HalfEdge *edge;
        int id;
    };

    bool operator<(const HalfEdgeSort& a, const HalfEdgeSort& b) {
        int temp_points[2][2];
        temp_points[0][0] = a.edge->points[0];
        temp_points[0][1] = a.edge->points[1];
        temp_points[1][0] = b.edge->points[0];
        temp_points[1][1] = b.edge->points[1];
        for(auto & temp_point : temp_points){
            if(temp_point[0] > temp_point[1]){
                std::swap(temp_point[0], temp_point[1]);
            }
        }
        if(temp_points[0][0] == temp_points[1][0]){
            return temp_points[0][1] < temp_points[1][1];
        } else {
            return temp_points[0][0] < temp_points[1][0];
        }
    }

    struct Edge {
        int points[2];
    };

    bool operator<(const Edge& a, const Edge& b){
        if(a.points[0] == b.points[0]){
            return a.points[1] < b.points[1];
        } else {
            return a.points[0] < b.points[0];
        }
    }

    bool operator!=(const Edge& a, const Edge& b){
        return (a.points[0] != b.points[0] || a.points[1] != b.points[1]);
    }
}

namespace {
    /*
    float orient2d(const float *a, const float *b, const float *c) {
        return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0]);
    }
    */

    double orient2d_double(const double *a, const double *b, const double *c) {
        return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0]);
    }

    struct TriangleEdge {
        int points[2];
        int tri;
    };
    
    /*
    bool operator<(const TriangleEdge& a, const TriangleEdge &b){
        if(a.points[0] == b.points[0]){
            return a.points[1] < b.points[1];
        } else {
            return a.points[0] < b.points[0];
        }
    }
    */
    
    bool NeighborSort(const TriangleEdge& a, const TriangleEdge &b){
        int points[2][2];
        for(int i=0; i<2; ++i){
            points[0][i] = a.points[i];
            points[1][i] = b.points[i];
        }
        for(auto & point : points){
            if(point[0] > point[1]){
                std::swap(point[0], point[1]);
            }
        }
        if(points[0][0] == points[1][0]){
            return points[0][1] < points[1][1];
        } else {
            return points[0][0] < points[1][0];
        }
    }

    typedef std::pair<int, int> TriNeighbor;

    /*
    bool operator<(const TriNeighbor& a, const TriNeighbor& b){
        return a.first < b.first;
    }
    */

    struct TriNeighborInfo {
        std::vector<TriNeighbor> tri_neighbors;
        std::vector<int> num_tri_neighbors;
        std::vector<int> tri_neighbor_index;
    };

    void GetTriNeighbors(const std::vector<int> &faces, const std::vector<double> &verts, TriNeighborInfo *info, bool display){
        info->tri_neighbors.clear();
        info->num_tri_neighbors.clear();
        info->tri_neighbor_index.clear();
        std::vector<TriangleEdge> edges;
        edges.reserve(faces.size());
        for(int i=0, len=faces.size(); i<len; i+=3){
            for(int j=0; j<3; ++j){
                TriangleEdge edge;
                edge.points[0] = faces[i+j];
                edge.points[1] = faces[i+(j+1)%3];
                edge.tri = i/3;
                edges.push_back(edge);
            }
        }
        std::sort(edges.begin(), edges.end(), NeighborSort);
        std::vector<int> edge_twin(edges.size(), -1);
        for(int i=1, len=edges.size(); i<len; ++i){
            if(edges[i].points[0] == edges[i-1].points[1] && edges[i].points[1] == edges[i-1].points[0]){
                edge_twin[i] = i-1;
                edge_twin[i-1] = i;
            }
        }
        for(int i=0, len=edge_twin.size(); i<len; ++i){
            if(edge_twin[i] != -1 && edge_twin[edge_twin[i]] == i){
                TriNeighbor neighbor;
                neighbor.first = edges[i].tri;
                neighbor.second = edges[edge_twin[i]].tri;
                info->tri_neighbors.push_back(neighbor);
            } else {
                if(display){
                    vec3 points[2];
                    TriangleEdge &edge = edges[i];
                    for(int j=0; j<2; ++j){
                        int vert_index = edge.points[j]*3;
                        for(int i=0; i<3; ++i){
                            points[j][i] = (float)verts[vert_index+i];
                        }    
                    }
                    DebugDraw::Instance()->AddLine(points[0], points[1], vec4(1.0f), _persistent, _DD_XRAY);
                }
            }
        }
        std::sort(info->tri_neighbors.begin(), info->tri_neighbors.end());
        info->num_tri_neighbors.resize(faces.size()/3, 0);
        info->tri_neighbor_index.resize(faces.size()/3, -1);
        for(int i=0, len=info->tri_neighbors.size(); i<len; ++i){
            int tri = info->tri_neighbors[i].first;
            if(info->tri_neighbor_index[tri] == -1){
                info->tri_neighbor_index[tri] = i;
            }
            ++info->num_tri_neighbors[tri];
        }
    }

    struct vec3d {
        double entries[3];
        vec3d(double val){
            for(double & entry : entries){
                entry = val;
            }
        }
        vec3d(){
            for(double & entry : entries){
                entry = 0.0;
            }
        }
        double& operator[](int which){
            return entries[which];
        }
        const double& operator[](int which) const {
            return entries[which];
        }
    };

    struct TriIntersect {
        std::pair<int, int> edge_id[2];
        vec3d true_intersect[2];
        bool valid;
    };

    struct TriIntersectInfo {
        int tris[2];
        TriIntersect tri_intersect;
    };

    bool TriIntersectInfoSortFirstTri(const TriIntersectInfo& a, const TriIntersectInfo& b) {
        return a.tris[0] < b.tris[0];
    }
    
    bool TriIntersectInfoSortSecondTri(const TriIntersectInfo& a, const TriIntersectInfo& b) {
        return a.tris[1] < b.tris[1];
    }

    vec3d operator-(const vec3d &a, const vec3d &b){
        vec3d result;
        for(int i=0; i<3; ++i){
            result[i] = a[i] - b[i];
        }
        return result;
    }

    vec3d operator+(const vec3d &a, const vec3d &b){
        vec3d result;
        for(int i=0; i<3; ++i){
            result[i] = a[i] + b[i];
        }
        return result;
    }

    vec3d operator*(const vec3d &a, double b){
        vec3d result;
        for(int i=0; i<3; ++i){
            result[i] = a[i] * b;
        }
        return result;
    }

    vec3d cross(const vec3d &vec_a, const vec3d &vec_b) {
        vec3d result;
        result[0] = vec_a[1] * vec_b[2] - vec_a[2] * vec_b[1];
        result[1] = vec_a[2] * vec_b[0] - vec_a[0] * vec_b[2];
        result[2] = vec_a[0] * vec_b[1] - vec_a[1] * vec_b[0];
        return result;
    }

    double length_squared(const vec3d &vec) {
        return vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
    }

    vec3d normalize(const vec3d &vec) {
        double length_squared_val = length_squared(vec);
        if(length_squared_val == 0.0){
            return vec3d(0.0);
        }
        return vec*(1.0/sqrt(length_squared_val));
    }

    double dot(const vec3d &vec_a, const vec3d &vec_b) {
        return vec_a[0]*vec_b[0] + vec_a[1]*vec_b[1] + vec_a[2]*vec_b[2];
    }

    void GetTriIntersection(const vec3d tri[][3], TriIntersect &tri_intersect){
        double tri_intersect_t[2][2];
        int tri_intersect_outlier[2];
        // Get normal for each triangle
        vec3d norm[2];
        for(int i=0; i<2; ++i){
            norm[i] = normalize(cross(tri[i][1] - tri[i][0], tri[i][2] - tri[i][0]));
        }
        // Find position of each triangle along its normal
        double plane_d[2];
        for(int i=0; i<2; ++i){
            plane_d[i] = dot(tri[i][0], norm[i]);
        }
        // Get position of each triangle vertex relative to the plane of the other triangle
        double other_plane_d[2][3];
        for(int i=0; i<2; ++i){
            for(int j=0; j<3; ++j){
                other_plane_d[i][j] = dot(tri[i][j], norm[1-i]) - plane_d[1-i];
            }
        }    
        // Get intersection points of each triangle with the other triangle's plane
        for(int i=0; i<2; ++i){
            // Get number of vertices on each side of other triangle's plane
            int num_pos = 0;
            int num_neg = 0;
            for(int j=0; j<3; ++j){
                if(other_plane_d[i][j] > 0.0){
                    ++num_pos;
                } else {
                    ++num_neg;
                }
            }
            if(num_pos == 3 || num_neg == 3){
                // All vertices of triangle are on the same side of the other triangle
                // No intersection!
                tri_intersect.valid = false;
                return;
            }
            // Get the vertex that is on the opposite side of the plane from the other vertices
            int outlier = -1;
            tri_intersect_outlier[i] = -1;
            if(num_pos == 1){
                for(int j=0; j<3; ++j){
                    if(other_plane_d[i][j] > 0.0){
                        outlier = j;
                    }
                }
            } else {
                for(int j=0; j<3; ++j){
                    if(other_plane_d[i][j] <= 0.0){
                        outlier = j;
                    }
                }
            }
            // Get intersection points
            double t[2];
            t[0] = other_plane_d[i][outlier] / (other_plane_d[i][outlier] - other_plane_d[i][(outlier+1)%3]);
            t[1] = other_plane_d[i][outlier] / (other_plane_d[i][outlier] - other_plane_d[i][(outlier+2)%3]);
            tri_intersect_outlier[i] = outlier;
            tri_intersect_t[i][0] = t[0];
            tri_intersect_t[i][1] = t[1];
        }    
        vec3d intersect[2][2];
        for(int i=0; i<2; ++i){
            intersect[i][0] = mix(tri[i][tri_intersect_outlier[i]], tri[i][(tri_intersect_outlier[i]+1)%3], tri_intersect_t[i][0]);
            intersect[i][1] = mix(tri[i][tri_intersect_outlier[i]], tri[i][(tri_intersect_outlier[i]+2)%3], tri_intersect_t[i][1]);
        }
        // Find the intersection of the two segments
        vec3d intersect_dir = normalize(intersect[0][1] - intersect[0][0]);
        int t_edge[2][2]; // Is this edge outlier+1 or outlier+2?
        double t[2][2];
        for(int i=0; i<2; ++i){
            for(int j=0; j<2; ++j){
                t[i][j] = dot(intersect[i][j] - intersect[0][0], intersect_dir);
                t_edge[i][j] = j;
            }
            if(t[i][0] > t[i][1]){
                std::swap(t[i][0], t[i][1]);
                std::swap(t_edge[i][0], t_edge[i][1]);
            }
        }
        if(t[0][1] < t[1][0] || t[1][1] < t[0][0]){
            // The intersection segments do not overlap
            // Therefore the triangles do not actually intersect
            tri_intersect.valid = false;
            return;
        }
        double intersect_bounds[2];
        if(t[0][0] > t[1][0]){
            tri_intersect.edge_id[0].first = 0;
            tri_intersect.edge_id[0].second = t_edge[0][0];
            intersect_bounds[0] = t[0][0];
        } else {
            tri_intersect.edge_id[0].first = 1;
            tri_intersect.edge_id[0].second = t_edge[1][0];
            intersect_bounds[0] = t[1][0];
        }
        if(t[0][1] < t[1][1]){
            tri_intersect.edge_id[1].first = 0;
            tri_intersect.edge_id[1].second = t_edge[0][1];
            intersect_bounds[1] = t[0][1];
        } else {
            tri_intersect.edge_id[1].first = 1;
            tri_intersect.edge_id[1].second = t_edge[1][1];
            intersect_bounds[1] = t[1][1];
        }
        tri_intersect.true_intersect[0] = intersect_dir * (float)intersect_bounds[0] + intersect[0][0];
        tri_intersect.true_intersect[1] = intersect_dir * (float)intersect_bounds[1] + intersect[0][0];
        tri_intersect.valid = true;
    }
    
    typedef std::pair<double, int> EdgeAngleSort;

    /*
    bool operator<(const EdgeAngleSort &a, const EdgeAngleSort &b) {
        return a.first < b.first;
    }
    */

    
    void TriangulateWrapper(std::vector<float> &points_in, std::vector<int> &edges, std::vector<GLuint> &faces_out) {
        struct triangulateio in;
        struct triangulateio out;

        in.numberofpoints = (int)points_in.size()/2;
        in.numberofpointattributes = 1;
        in.pointmarkerlist = 0;
        std::vector<tREAL> in_points(in.numberofpoints*2);
        for(int i=0, len=points_in.size(); i<len; ++i){
            in_points[i] = points_in[i];
        }
        in.pointlist = &in_points.front();
        std::vector<tREAL> in_point_attributes(in.numberofpoints);
        for (int i=0; i<in.numberofpoints; i++){
            in_point_attributes[i] = i;
        }
        in.pointattributelist = &in_point_attributes.front();

        std::vector<int> new_edges = edges;
        in.numberofsegments = new_edges.size()/2;
        in.segmentmarkerlist = NULL;
        if(!new_edges.empty()){
            in.segmentlist = &new_edges.front();
        } else {
            in.segmentlist = NULL;
        }

        in.numberofholes = 0;
        in.holelist = NULL;
        in.numberofregions = 0;
        in.regionlist = NULL;

        out.pointlist = NULL;
        out.pointattributelist = NULL;
        out.trianglelist = NULL;
        out.segmentlist = NULL;
        out.segmentmarkerlist = NULL;
        out.pointmarkerlist = NULL;

        triangulate("zQ", &in, &out, 0);

        faces_out.resize(out.numberoftriangles*3);

        int face_index = 0;
        for (int i = 0; i<out.numberoftriangles; i++){
            faces_out[face_index++]=(GLuint)(out.pointattributelist[out.trianglelist[i*3+2]]);
            faces_out[face_index++]=(GLuint)(out.pointattributelist[out.trianglelist[i*3+1]]);
            faces_out[face_index++]=(GLuint)(out.pointattributelist[out.trianglelist[i*3+0]]);
        }

        OG_FREE(out.pointlist);
        OG_FREE(out.trianglelist);
        OG_FREE(out.segmentlist);
        OG_FREE(out.segmentmarkerlist);
        OG_FREE(out.pointmarkerlist);
        OG_FREE(out.pointattributelist);
    }

    struct vec2d {
        double entries[2];
        double& operator[](int which){
            return entries[which];
        }
        const double& operator[](int which) const{
            return entries[which];
        }
    };

    vec2d operator-(const vec2d &a, const vec2d &b){
        vec2d result;
        for(int i=0; i<2; ++i){
            result[i] = a[i] - b[i];
        }
        return result;
    }

    vec2d operator+(const vec2d &a, const vec2d &b){
        vec2d result;
        for(int i=0; i<2; ++i){
            result[i] = a[i] + b[i];
        }
        return result;
    }

    vec2d operator*(const vec2d &a, double b){
        vec2d result;
        for(int i=0; i<2; ++i){
            result[i] = a[i] * b;
        }
        return result;
    }

    double length_squared(const vec2d &a){
        return a[0]*a[0] + a[1]*a[1];
    }

    double distance_squared(const vec2d &a, const vec2d &b){
        vec2d diff = b-a;
        return length_squared(diff);
    }

    struct TrianglePoint {
        vec2d flat_coord;
        vec3d coord;
        vec3d norm;
        vec3d bary;
        int id;
        bool on_edge[3];
        enum Side {
            OUTSIDE, EDGE, INSIDE, UNKNOWN
        };
        Side side;
    };

    bool operator<(const TrianglePoint& a, const TrianglePoint& b){
        if(a.flat_coord[0] == b.flat_coord[0]){
            return a.flat_coord[1] < b.flat_coord[1];
        } else {
            return a.flat_coord[0] < b.flat_coord[0];
        }
    }

    struct TriIntersectionShape {
        ShapeDisposalData sdd;      
        std::vector<double> vertices;
        std::vector<float> vertices_float;
        std::vector<int> faces;
        btGImpactMeshShape *gimpact_shape;
        btCollisionObject col_obj;
    };

    struct VertSorter {
        double coord[3];
        int old_id;
        int merge_target;
    };

    bool VertSortByCoord(const VertSorter &a, const VertSorter &b){
        if(a.coord[0] == b.coord[0]){
            if(a.coord[1] == b.coord[1]){
                return a.coord[2] < b.coord[2];
            }
            return a.coord[1] < b.coord[1];
        }
        return a.coord[0] < b.coord[0];
    }

    bool VertSortById(const VertSorter &a, const VertSorter &b){
        return a.old_id < b.old_id;
    }

    bool vec3d_equal(const double *a, const double *b){
        return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]);
    }

    void MergeDoubleVerts(const std::vector<double>& vertices, std::vector<int>& faces, bool remove_degenerate_faces){
        // Merge overlapping vertices
        // -- sort vertices by coordinate
        std::vector<VertSorter> vert_sort;
        vert_sort.resize(vertices.size()/3);
        for(int i=0, vert=0, len=vertices.size(); i<len; i+=3, ++vert){
            for(int j=0; j<3; ++j){
                vert_sort[vert].coord[j] = vertices[i+j];
            }
            vert_sort[vert].old_id = vert;
            vert_sort[vert].merge_target = vert;
        }
        std::sort(vert_sort.begin(), vert_sort.end(), VertSortByCoord);
        // -- get merge target for each vert
        for(int i=1, len=vert_sort.size(); i<len; ++i){
            if(vec3d_equal(vert_sort[i-1].coord, vert_sort[i].coord)){
                vert_sort[i].merge_target = vert_sort[i-1].merge_target;
            }
        }
        std::sort(vert_sort.begin(), vert_sort.end(), VertSortById);
        // -- set faces to use merge targets
        for(int & face : faces){
            face = vert_sort[face].merge_target;
        }
        if(remove_degenerate_faces){
            // -- remove degenerate faces
            for(int i=faces.size()-3; i>=0; i-=3){
                bool degenerate = false;
                for(int j=0; j<3; ++j){
                    if(faces[i+j] == faces[i+(j+1)%3]){
                        degenerate = true;
                    }
                }
                if(degenerate){
                    for(int j=0; j<3; ++j){
                        faces[i+j] = faces[faces.size()-3+j];
                    }
                    faces.resize(faces.size()-3);
                }
            }
        }
    }

    void FillTriIntersectionShape(const Model& model, const mat4& transform, TriIntersectionShape* shape){
        // Transform vertices
        for(int i=0, len=model.vertices.size(); i<len; i += 3){
            double vec[3];
            vec[0] = (double)model.vertices[i+0];
            vec[1] = (double)model.vertices[i+1];
            vec[2] = (double)model.vertices[i+2];
            MultiplyMat4Vec3D(transform, vec);
            shape->vertices.push_back(vec[0]);
            shape->vertices.push_back(vec[1]);
            shape->vertices.push_back(vec[2]);
        }
        shape->faces.reserve(model.faces.size());
        for(unsigned int face : model.faces){
            shape->faces.push_back(face);
        }
        // Create Bullet collision shape
        shape->vertices_float.reserve(shape->vertices.size());
        for(int i=0, len=shape->vertices.size(); i<len; ++i){
            shape->vertices_float.push_back((float)shape->vertices[i]);
        }
        shape->gimpact_shape = BulletWorld::CreateDynamicMeshShape(shape->faces, shape->vertices_float, shape->sdd);
        shape->col_obj.setCollisionShape((btCollisionShape*)shape->gimpact_shape);        
    }
    
    struct LabeledEdge {
        int points[3];
        TrianglePoint::Side side;
        vec3d norm;
        int id;
        int tri;
    };

    bool operator<(const LabeledEdge& a, const LabeledEdge& b){
        if(a.points[0] == b.points[0]){
            return a.points[1] < b.points[1];
        } else {
            return a.points[0] < b.points[0];
        }
    }

    struct TriIntersectOutput { 
        std::vector<int> *faces_to_remove;
        std::vector<double> *new_bary;
        std::vector<double> *new_vertices;
        std::vector<int> *new_vert_tri_points;
        std::vector<int> *new_faces;
        std::vector<TrianglePoint::Side> *new_triangle_sides;
    };

    void GetNewTris(int which, std::vector<TriIntersectInfo> &tri_intersect_info, TriIntersectionShape shape[],  TriIntersectOutput &output) {
        // sort triangle intersections based on which object we are dealing with
        if(which == 0){
            std::sort(tri_intersect_info.begin(), tri_intersect_info.end(), TriIntersectInfoSortFirstTri);
        } else {
            std::sort(tri_intersect_info.begin(), tri_intersect_info.end(), TriIntersectInfoSortSecondTri);
        }
        // Handle one triangle at a time
        int old_index = 0;
        for(int new_index=1, len=tri_intersect_info.size(); new_index<=len; ++new_index){
            if(new_index==len || tri_intersect_info[new_index].tris[which] != tri_intersect_info[old_index].tris[which]){            
                std::vector<LabeledEdge> labeled_edges;
                // Look at tri_intersect_info old_index - i to get intersecting edges
                int tri_index = tri_intersect_info[old_index].tris[which]*3;
                output.faces_to_remove->push_back(tri_intersect_info[old_index].tris[which]);
                vec3d tri_vert[3];
                int tri_vert_id[3];
                for(int j=0; j<3; ++j){
                    tri_vert_id[j] = shape[1-which].faces[tri_index+j];
                    int vert_index = tri_vert_id[j]*3;
                    for(int k=0; k<3; ++k){
                        tri_vert[j][k] = shape[1-which].vertices[vert_index+k];
                    }
                }
                // Create orthonormal basis on plane of triangle
                vec3d norm = normalize(cross(tri_vert[1]-tri_vert[0], tri_vert[2]-tri_vert[0]));
                vec3d basis[2];
                basis[0] = normalize(tri_vert[1] - tri_vert[0]);
                basis[1] = tri_vert[2] - tri_vert[0];
                basis[1] = normalize(basis[1] - basis[0] * dot(basis[0], basis[1]));
                // Add initial points to set
                std::vector<TrianglePoint> triangle_points;
                std::vector<int> on_edge;
                for(int j=0; j<3; ++j){
                    on_edge.push_back(triangle_points.size());
                    TrianglePoint point;
                    point.bary = vec3d(0.0);
                    point.bary[j] = 1.0;
                    point.coord = tri_vert[j];
                    point.flat_coord[0] = dot(basis[0], tri_vert[j]);
                    point.flat_coord[1] = dot(basis[1], tri_vert[j]);
                    point.id = triangle_points.size();
                    point.on_edge[j] = true;
                    point.on_edge[(j+2)%3] = true;
                    point.on_edge[(j+1)%3] = false;
                    point.side = TrianglePoint::UNKNOWN;
                    triangle_points.push_back(point);
                }
                std::vector<int> edges;
                // Loop through triangle intersections
                for(int j=old_index; j<new_index; ++j){
                    TriIntersect &tri_intersect = tri_intersect_info[j].tri_intersect;
                    // Add triangle point for each intersection vertex
                    for(int k=0; k<2; ++k){
                        TrianglePoint point;
                        point.coord = tri_intersect.true_intersect[k];
                        point.flat_coord[0] = dot(basis[0], tri_intersect.true_intersect[k]);
                        point.flat_coord[1] = dot(basis[1], tri_intersect.true_intersect[k]);
                        double vec_a[2], vec_b[2];
                        for(int l=0; l<2; ++l){
                            vec_a[l] = triangle_points[1].flat_coord[l] - triangle_points[0].flat_coord[l];
                            vec_b[l] = triangle_points[2].flat_coord[l] - triangle_points[0].flat_coord[l];
                        }
                        double twice_area = vec_a[0] * vec_b[1] - vec_a[1] * vec_b[0];
                        point.bary[0] = orient2d_double(triangle_points[1].flat_coord.entries, triangle_points[2].flat_coord.entries, point.flat_coord.entries);
                        point.bary[1] = orient2d_double(triangle_points[2].flat_coord.entries, triangle_points[0].flat_coord.entries, point.flat_coord.entries);
                        point.bary[2] = orient2d_double(triangle_points[0].flat_coord.entries, triangle_points[1].flat_coord.entries, point.flat_coord.entries);
                        point.bary[0] /= twice_area;
                        point.bary[1] /= twice_area;
                        point.bary[2] /= twice_area;
                        point.id = triangle_points.size();
                        point.on_edge[0] = false;
                        point.on_edge[1] = false;
                        point.on_edge[2] = false;
                        point.side = TrianglePoint::EDGE;
                        if(tri_intersect.edge_id[k].first == which){
                            on_edge.push_back(triangle_points.size());
                        }
                        triangle_points.push_back(point);
                    }
                    // Add edge for triangulation
                    edges.push_back(triangle_points.size()-2);
                    edges.push_back(triangle_points.size()-1);
                    // Add labeled_edge for processing later
                    LabeledEdge labeled_edge;
                    labeled_edge.points[0] = triangle_points.size()-2;
                    labeled_edge.points[1] = triangle_points.size()-1;
                    labeled_edge.points[2] = -1;
                    if(labeled_edge.points[0] > labeled_edge.points[1]){
                        std::swap(labeled_edge.points[0], labeled_edge.points[1]);
                    }
                    labeled_edge.side = TrianglePoint::EDGE;
                    // Get vertices of intersected triangle
                    vec3d other_tri_vert[3];
                    int other_tri_index = tri_intersect_info[j].tris[1-which]*3;
                    for(int j=0; j<3; ++j){
                        int vert_index = shape[which].faces[other_tri_index+j]*3;
                        for(int k=0; k<3; ++k){
                            other_tri_vert[j][k] = shape[which].vertices[vert_index+k];
                        }
                    }
                    // Label the edge with the normal of the other triangle
                    labeled_edge.norm = normalize(cross(other_tri_vert[1] - other_tri_vert[0], other_tri_vert[2] - other_tri_vert[0]));
                    labeled_edges.push_back(labeled_edge);
                }
                // Calculate the angle from the center of the tri to each edge tri
                std::vector<EdgeAngleSort> edge_angle;
                edge_angle.reserve(on_edge.size());
                vec2d mid_point;
                mid_point = (triangle_points[0].flat_coord + triangle_points[1].flat_coord + triangle_points[2].flat_coord) * (1.0 / 3.0);
                for(int j : on_edge){
                    EdgeAngleSort eas;
                    eas.second = j;
                    vec2d offset = triangle_points[j].flat_coord - mid_point;
                    eas.first = atan2(offset[1], offset[0]);
                    edge_angle.push_back(eas);
                }
                // Add edges sorted by angle
                std::sort(edge_angle.begin(), edge_angle.end());
                for(int j=0, len=edge_angle.size(); j<len; ++j){
                    edges.push_back(edge_angle[j].second);
                    edges.push_back(edge_angle[(j+1)%len].second);
                }
                // Get positions of each triangle corner on edge loop
                int edge_pos[3];
                for(int j=0, len=edge_angle.size(); j<len; ++j){
                    for(int k=0; k<3; ++k){
                        if(edge_angle[j].second == k){
                            edge_pos[k] = j;
                        }
                    }
                }
                // Mark which edge each non-corner edge vertex is on
                for(int k=0; k<3; ++k){
                    for(int j=edge_pos[k]+1;; ++j){
                        if(j >= (int)edge_angle.size()){
                            j = 0;
                        }
                        if(j == edge_pos[(k+1)%3]){
                            break;
                        }
                        triangle_points[edge_angle[j].second].on_edge[k] = true;
                    }
                }
                // Remove duplicate points
                std::vector<int> triangle_point_remap(triangle_points.size(), 0);
                std::sort(triangle_points.begin(), triangle_points.end());
                if(!triangle_points.empty()){
                    triangle_point_remap[triangle_points[0].id] = 0;
                    int index=0;
                    for(int i=1, len=triangle_points.size(); i<len; ++i){
                        if(distance_squared(triangle_points[i].flat_coord, triangle_points[i-1].flat_coord) > 0.0000001){
                            ++index;
                            triangle_point_remap[triangle_points[i].id] = index;
                            triangle_points[index] = triangle_points[i];
                        } else {
                            triangle_point_remap[triangle_points[i].id] = index;
                        }
                    }
                    triangle_points.resize(index+1);
                }
                // Remap edges to new points
                for(int & edge : edges){
                    edge = triangle_point_remap[edge];
                }
                for(auto & labeled_edge : labeled_edges){
                    labeled_edge.points[0] = triangle_point_remap[labeled_edge.points[0]];
                    labeled_edge.points[1] = triangle_point_remap[labeled_edge.points[1]];
                    if(labeled_edge.points[0] > labeled_edge.points[1]){
                        std::swap(labeled_edge.points[0], labeled_edge.points[1]);
                    }
                }
                // Get points to input to triangulation
                std::vector<float> points;
                for(auto & triangle_point : triangle_points){
                    for(int j=0; j<2; ++j){
                        points.push_back((float)triangle_point.flat_coord[j]);
                    }
                }
                // Triangulate
                std::vector<GLuint> faces;
                TriangulateWrapper(points, edges, faces);
                // Add new vertices
                int vert_start = output.new_bary->size()/3;
                for(auto & triangle_point : triangle_points){
                    for(int k=0; k<3; ++k){
                        output.new_vertices->push_back(triangle_point.coord[k]);
                        output.new_bary->push_back(triangle_point.bary[k]);
                        output.new_vert_tri_points->push_back(tri_vert_id[k]);
                    }
                }
                // Remove triangles that are all on same edge
                int face_index = 0;
                for(int j=0, len=faces.size(); j<len; j+=3){
                    bool includes_bad_vert = false;
                    for(int k=0; k<3; ++k){
                        if(triangle_points[faces[j+0]].on_edge[k] && 
                           triangle_points[faces[j+1]].on_edge[k] && 
                           triangle_points[faces[j+2]].on_edge[k])
                        {
                            includes_bad_vert = true;
                        }
                    }
                    if(!includes_bad_vert){
                        for(int k=0; k<3; ++k){
                            faces[face_index+k] = faces[j+k];
                        }
                        face_index += 3;
                    }
                }
                faces.resize(face_index);
                // Make sure faces all face the proper direction
                for(int j=0, len=faces.size(); j<len; j+=3){
                    vec3d vert[3];
                    for(int k=0; k<3; ++k){
                        int vert_index = faces[j+k];
                        vert[k] = triangle_points[vert_index].coord;
                    }
                    vec3d new_norm = normalize(cross(vert[1]-vert[0], vert[2]-vert[0]));
                    if(dot(new_norm, norm) < 0.0f){
                        std::swap(faces[j+1], faces[j+2]);
                    }
                }
                // Create unknown edge for each triangle side
                for(int i=0, len=faces.size(); i<len; i+=3){
                    for(int j=0; j<3; ++j){
                        LabeledEdge labeled_edge;
                        labeled_edge.tri = i/3;
                        labeled_edge.points[0] = faces[i+j];
                        labeled_edge.points[1] = faces[i+(j+1)%3];
                        labeled_edge.points[2] = faces[i+(j+2)%3];
                        if(labeled_edge.points[0] > labeled_edge.points[1]){
                            std::swap(labeled_edge.points[0], labeled_edge.points[1]);
                        }
                        labeled_edge.id = labeled_edges.size();
                        labeled_edge.side = TrianglePoint::UNKNOWN;
                        labeled_edges.push_back(labeled_edge);
                    }
                }
                std::vector<TriNeighbor> tri_neighbors;
                // Propagate edge info
                {
                    std::vector<LabeledEdge> sorted = labeled_edges;
                    std::sort(sorted.begin(), sorted.end());
                    int old_index = 0;
                    for(int i=1, len=sorted.size(); i<=len; ++i){
                        if(i==len || sorted[i].points[0] != sorted[old_index].points[0] || sorted[i].points[1] != sorted[old_index].points[1]){
                            bool edge = false;
                            vec3d norm;
                            for(int j=old_index; j<i; ++j){
                                if(sorted[j].side == TrianglePoint::EDGE){
                                    edge = true;
                                    norm = sorted[j].norm;
                                }
                            }
                            if(edge){
                                for(int j=old_index; j<i; ++j){
                                    if(sorted[j].side != TrianglePoint::EDGE && sorted[j].points[2] != -1){
                                        int index = sorted[j].id;
                                        labeled_edges[index].side = TrianglePoint::EDGE;
                                        labeled_edges[index].norm = norm;
                                    }
                                }
                            } else { // if edge is not an intersection edge, mark tri neighbors
                                for(int j=old_index; j<i; ++j){
                                    if(sorted[j].points[2] == -1){
                                        continue;
                                    }
                                    for(int k=old_index; k<i; ++k){
                                        if(j == k || sorted[k].points[2] == -1){
                                            continue;
                                        }
                                        tri_neighbors.push_back(TriNeighbor(sorted[j].tri, sorted[k].tri));
                                    }
                                }
                            }
                            old_index = i;
                        }
                    }
                }
                std::vector<TrianglePoint::Side> triangle_side(faces.size()/3, TrianglePoint::UNKNOWN);
                // Get outside/inside of edges neighboring edge
                for(auto & edge : labeled_edges){
                    if(edge.side == TrianglePoint::EDGE && edge.points[2] != -1){
                        vec3d opposite_vert = triangle_points[edge.points[2]].coord;
                        vec3d mid_point = (triangle_points[edge.points[0]].coord + triangle_points[edge.points[1]].coord)*0.5f;
                        vec3d dir = opposite_vert - mid_point;
                        if(dot(dir, edge.norm) > 0.0f){
                            triangle_side[edge.tri] = TrianglePoint::OUTSIDE;
                        } else {
                            triangle_side[edge.tri] = TrianglePoint::INSIDE;
                        }
                        continue;
                    }
                }
                // Propagate triangle side
                std::sort(tri_neighbors.begin(), tri_neighbors.end());
                std::vector<int> num_tri_neighbors(triangle_side.size(), 0);
                std::vector<int> tri_neighbor_index(triangle_side.size(), -1);
                for(int i=0, len=tri_neighbors.size(); i<len; ++i){
                    if(tri_neighbor_index[tri_neighbors[i].first] == -1){
                        tri_neighbor_index[tri_neighbors[i].first] = i;
                    }
                    ++num_tri_neighbors[tri_neighbors[i].first];
                }
                std::queue<int> tri_queue;
                for(int i=0, len=triangle_side.size(); i<len; ++i){
                    if(triangle_side[i] != TrianglePoint::UNKNOWN){
                        tri_queue.push(i);
                    }
                }
                while(!tri_queue.empty()){
                    int tri = tri_queue.front();
                    for(int i=0; i<num_tri_neighbors[tri]; ++i){
                        int neighbor = tri_neighbors[tri_neighbor_index[tri]+i].second;
                        if(triangle_side[neighbor] == TrianglePoint::UNKNOWN){
                            triangle_side[neighbor] = triangle_side[tri];
                            tri_queue.push(neighbor);
                        }
                    }
                    tri_queue.pop();
                }
                for(unsigned int face : faces){
                    output.new_faces->push_back(face+vert_start);
                }
                for(auto & j : triangle_side){
                    output.new_triangle_sides->push_back(j);
                }
                old_index = new_index;
            }
        }
    }


    struct MergeDist {
        double dist;
        int point[2];
    };

    bool operator<(const MergeDist &a, const MergeDist &b){
        if(a.point[0] == b.point[0]){
            return a.point[1] < b.point[1];
        } else {
            return a.point[0] < b.point[0];        
        }
    }

    void MergeTriIntersects(std::vector<TriIntersectInfo> &tri_intersect_info) {
        std::vector<vec3d> test_points;
        std::vector<MergeDist> test_point_distances;
        for(auto & tii : tri_intersect_info){
            test_points.push_back(tii.tri_intersect.true_intersect[0]);
            test_points.push_back(tii.tri_intersect.true_intersect[1]);
        }
        double merge_squared_threshold = 1.0e-10;
        //double merge_threshold = 1.0e-5;
        for(int i=0, len=test_points.size(); i<len; ++i){
            for(int j=i+1, len=test_points.size(); j<len; ++j){
                vec3d rel = test_points[i] - test_points[j];
                MergeDist md;
                md.dist = (dot(rel, rel));
                md.point[0] = i;
                md.point[1] = j;
                if(md.point[0] > md.point[1]){
                    std::swap(md.point[0], md.point[1]);
                }
                if(md.dist < merge_squared_threshold){
                    test_point_distances.push_back(md);
                }
            }
        }
        std::sort(test_point_distances.begin(), test_point_distances.end());
        for(auto & tpd : test_point_distances){
            test_points[tpd.point[1]] = test_points[tpd.point[0]];
        }
        for(int i=0, index=0, len=tri_intersect_info.size(); i<len; ++i, index+=2){
            TriIntersectInfo& tii = tri_intersect_info[i];
            tii.tri_intersect.true_intersect[0] = test_points[index+0];
            tii.tri_intersect.true_intersect[1] = test_points[index+1];
        }
    }
} // namespace ""

bool CheckShapeValid(const std::vector<int> &faces, const std::vector<double> &vertices) {
    std::vector<int> merged_faces = faces;
    MergeDoubleVerts(vertices, merged_faces, true);
    TriNeighborInfo tri_neighbor_info;
    GetTriNeighbors(merged_faces, vertices, &tri_neighbor_info, true);
    bool non_three_neighbor = false;
    for(int num_tri_neighbor : tri_neighbor_info.num_tri_neighbors){
        if(num_tri_neighbor != 3){
            non_three_neighbor = true;
        }
    }
    return !non_three_neighbor;
}

void AddCSGResult(const CSGResult &result, CSGModelInfo *csg_model, const Model& model, bool flip) {
    csg_model->faces.reserve(csg_model->faces.size()+result.indices.size());
    int old_faces = csg_model->verts.size()/3;
    if(!flip){
        for(int index : result.indices){
            csg_model->faces.push_back(index + old_faces);
        }
    } else {
        for(int i=0, len=result.indices.size(); i<len; i+=3){
            csg_model->faces.push_back(result.indices[i+0] + old_faces);
            csg_model->faces.push_back(result.indices[i+2] + old_faces);
            csg_model->faces.push_back(result.indices[i+1] + old_faces);
        }
    }
    csg_model->verts.reserve(csg_model->verts.size()+result.verts.size());
    csg_model->tex_coords.reserve(csg_model->verts.size()+result.verts.size()/3*2);
    csg_model->tex_coords2.reserve(csg_model->verts.size()+result.verts.size()/3*2);
    for(int i=0, len=result.bary.size(); i<len; i+=3){
        const double *bary = &result.bary[i];
        const double *result_vert = &result.verts[i];
        const int *vert_id = &result.vert_id[i];
        vec3d vert;
        for(int j=0; j<3; ++j){
            vert[j] = result_vert[j];
        }
        //MultiplyMat4Vec3D(eo->GetTransform(), &vert[0]);
        csg_model->verts.push_back(vert[0]);
        csg_model->verts.push_back(vert[1]);
        csg_model->verts.push_back(vert[2]);

        vec2 tex_coord;
        for(int j=0; j<3; ++j){
            vec2 tri_tex_coord;
            int vert_index = vert_id[j]*2;
            for(int k=0; k<2; ++k){
                tri_tex_coord[k] = model.tex_coords[vert_index+k];
            }
            tex_coord += tri_tex_coord * (float)bary[j];
        }
        csg_model->tex_coords.push_back(tex_coord[0]);
        csg_model->tex_coords.push_back(tex_coord[1]);

        for(int j=0; j<3; ++j){
            vec2 tri_tex_coord;
            int vert_index = vert_id[j]*2;
            for(int k=0; k<2; ++k){
                tri_tex_coord[k] = model.tex_coords2[vert_index+k];
            }
            tex_coord += tri_tex_coord * (float)bary[j];
        }
        csg_model->tex_coords2.push_back(tex_coord[0]);
        csg_model->tex_coords2.push_back(tex_coord[1]);
    };
}

int ModelFromCSGModelInfo(const CSGModelInfo &model_info){
    int model_id = Models::Instance()->AddModel();
    Model *model = &Models::Instance()->GetModel(model_id);
    model->vertices.resize(model_info.verts.size());
    for(int i=0, len=model_info.verts.size(); i<len; ++i){
        model->vertices[i] = (float)model_info.verts[i];
    }
    model->tex_coords.resize(model_info.verts.size()/3*2, 0.0f);
    model->tex_coords2.resize(model_info.verts.size()/3*2, 0.0f);
    for(int i=0, len=model_info.tex_coords.size(); i<len; ++i){
        model->tex_coords[i] = model_info.tex_coords[i];
        model->tex_coords2[i] = model_info.tex_coords2[i];
    }
    model->faces.resize(model_info.faces.size(), 0);
    for(int i=0, len=model_info.faces.size(); i<len; ++i){
        model->faces[i] = model_info.faces[i];
    }
    model->normals.resize(model_info.verts.size());
    model->tangents.resize(model_info.verts.size());
    model->bitangents.resize(model_info.verts.size());
    model->face_normals.resize(model_info.faces.size()/3);
    model->calcNormals();
    model->calcTangents();
    return model_id;
}

bool CollideObjects(BulletWorld& bw, const Model &model_a, const mat4 &transform_a, const Model &model_b, const mat4 &transform_b, CSGResultCombined *results) {
    // Get transformed and merged meshes
    TriIntersectionShape shape[2];
    FillTriIntersectionShape(model_a, transform_a, &shape[0]);
    FillTriIntersectionShape(model_b, transform_b, &shape[1]);
    // Check triangle neighbors 
    if(!CheckShapeValid(shape[1].faces, shape[1].vertices)){
        LOGE << "Model is invalid" << std::endl;
        return false;
    }

    // Use Bullet GIMPACT collision to get candidate triangle pairs
    MeshCollisionCallback cb;
    bw.GetPairCollisions(shape[1].col_obj, shape[0].col_obj, cb);
    // Process triangle intersections to eliminate false positives and get precise intersection segments
    std::vector<TriIntersectInfo> tri_intersect_info;
    for(const auto & tri_pair : cb.tri_pairs){
        // Get triangle vertices
        vec3d tri[2][3];
        int tri_index = tri_pair.first*3;
        for(int j=0; j<3; ++j){
            int vert_index = shape[1].faces[tri_index+j]*3;
            for(int k=0; k<3; ++k){
                tri[0][j][k] = shape[1].vertices[vert_index+k];
            }
        }
        tri_index = tri_pair.second*3;
        for(int j=0; j<3; ++j){
            int vert_index = shape[0].faces[tri_index+j]*3;
            for(int k=0; k<3; ++k){
                tri[1][j][k] = shape[0].vertices[vert_index+k];
            }
        }
        // Get intersection line
        TriIntersect tri_intersect;
        GetTriIntersection(tri, tri_intersect);
        // Add to list
        if(tri_intersect.valid){
            TriIntersectInfo info;
            info.tri_intersect = tri_intersect;
            info.tris[0] = tri_pair.first;
            info.tris[1] = tri_pair.second;
            tri_intersect_info.push_back(info);
        }
    }
    MergeTriIntersects(tri_intersect_info);
    std::vector<int> new_faces[2];
    std::vector<double> new_vertices[2];
    std::vector<int> faces_to_remove[2];
    std::vector<double> new_bary[2];
    std::vector<int> new_vert_tri_points[2];
    std::vector<TrianglePoint::Side> new_triangle_sides[2];
    for(int i=0; i<2; ++i){
        TriIntersectOutput output;
        output.faces_to_remove = &faces_to_remove[i];
        output.new_faces = &new_faces[i];
        output.new_triangle_sides = &new_triangle_sides[i];
        output.new_bary = &new_bary[i];
        output.new_vert_tri_points = &new_vert_tri_points[i];
        output.new_vertices = &new_vertices[i];
        GetNewTris(i, tri_intersect_info, shape, output);
    }
    // Get old faces minus removed ones
    std::vector<int> old_faces[2];
    std::vector<double> old_vertices[2];
    std::vector<double> old_bary[2];
    std::vector<int> old_vert_id[2];
    std::vector<TrianglePoint::Side> merge_triangle_sides[2];
    for(int l=0; l<2; ++l){
        old_faces[l].resize(shape[l].faces.size());
        for(int i=0, len=shape[l].faces.size(); i<len; ++i){
            old_faces[l][i] = shape[l].faces[i];
        }
        // Remove old intersecting faces
        for(int i=faces_to_remove[1-l].size()-1; i>=0; --i){
            int index = faces_to_remove[1-l][i]*3;
            int last_index = old_faces[l].size()-3;
            for(int j=0; j<3; ++j){
                old_faces[l][index+j] = old_faces[l][last_index+j];
            }
            old_faces[l].resize(old_faces[l].size()-3);
        } 

        old_vertices[l] = shape[l].vertices;
        for(int i=0, len=old_vertices[l].size(); i<len; i+=3){
            old_vert_id[l].push_back(i/3);
            old_vert_id[l].push_back(i/3);
            old_vert_id[l].push_back(i/3);
            old_bary[l].push_back(1.0);
            old_bary[l].push_back(0.0);
            old_bary[l].push_back(0.0);
        }

        // Add new faces and vertices
        merge_triangle_sides[l].resize(old_faces[l].size()/3, TrianglePoint::UNKNOWN);
        for(int i=0, len=new_faces[1-l].size(); i<len; ++i){
            old_faces[l].push_back(new_faces[1-l][i]+old_vertices[l].size()/3);
        }
        for(int i=0, len=new_triangle_sides[1-l].size(); i<len; ++i){
            merge_triangle_sides[l].push_back(new_triangle_sides[1-l][i]);
        }
        old_vertices[l].insert(old_vertices[l].end(), new_vertices[1-l].begin(), new_vertices[1-l].end());
        old_bary[l].insert(old_bary[l].end(), new_bary[1-l].begin(), new_bary[1-l].end());
        old_vert_id[l].insert(old_vert_id[l].end(), new_vert_tri_points[1-l].begin(), new_vert_tri_points[1-l].end());

        // Flood fill triangle sides
        // -- merge overlapping verts
        std::vector<int> merged_faces = old_faces[l];
        MergeDoubleVerts(old_vertices[l], merged_faces, false);

        TriNeighborInfo tri_neighbor_info;
        GetTriNeighbors(merged_faces, shape[l].vertices, &tri_neighbor_info, false);

        std::queue<int> tri_queue;
        for(int i=0, len=merge_triangle_sides[l].size(); i<len; ++i){
            if(merge_triangle_sides[l][i] != TrianglePoint::UNKNOWN){
                tri_queue.push(i);
            }
        }
        while(!tri_queue.empty()){
            int tri = tri_queue.front();
            for(int i=0; i<tri_neighbor_info.num_tri_neighbors[tri]; ++i){
                int neighbor = tri_neighbor_info.tri_neighbors[tri_neighbor_info.tri_neighbor_index[tri]+i].second;
                if(merge_triangle_sides[l][neighbor] == TrianglePoint::UNKNOWN){
                    merge_triangle_sides[l][neighbor] = merge_triangle_sides[l][tri];
                    tri_queue.push(neighbor);
                }
            }
            tri_queue.pop();
        }
    }
    if(!CheckShapeValid(shape[1].faces, shape[1].vertices)){
        LOGE << "Model is invalid" << std::endl;
        return false;
    }
    for(int which_model=0; which_model<2; ++which_model){
        for(int which_side=0; which_side<2; ++which_side){
            TrianglePoint::Side side;
            switch(which_side){
            case 0: side = TrianglePoint::INSIDE; break;
            case 1: side = TrianglePoint::OUTSIDE; break;
            default: __builtin_unreachable(); break;
            }

            const std::vector<TrianglePoint::Side> &sides = merge_triangle_sides[which_model];
            std::vector<double> vertices;
            std::vector<int> indices;
            std::vector<double> bary;
            std::vector<int> vert_id;
            for(int i=0, side_index=0, len=old_faces[which_model].size(); i<len; i+=3, ++side_index){
                if(sides[side_index] == side) {
                    for(int j=0; j<3; ++j){
                        int vert_index = old_faces[which_model][i+j]*3;
                        indices.push_back(vertices.size()/3);
                        for(int j=0; j<3; ++j){
                            vertices.push_back(old_vertices[which_model][vert_index+j]);
                            bary.push_back(old_bary[which_model][vert_index+j]);
                            vert_id.push_back(old_vert_id[which_model][vert_index+j]);
                        }
                    }
                }
            }
            results->result[which_model][which_side].verts = vertices;
            results->result[which_model][which_side].bary = bary;
            results->result[which_model][which_side].vert_id = vert_id;
            results->result[which_model][which_side].indices = indices;
        }
    }
    return true;
}

void MultiplyMat4Vec3D(const mat4& mat, double vec[]) {
    double new_vec[3];
    const float *mat_e = mat.entries;
    for(int i=0; i<3; ++i){
        new_vec[i] = mat_e[0+i] * vec[0] + mat_e[4+i] * vec[1] + mat_e[8+i] * vec[2] + mat_e[12+i];
    }
    for(int i=0; i<3; ++i){
        vec[i] = new_vec[i];
    }
}
