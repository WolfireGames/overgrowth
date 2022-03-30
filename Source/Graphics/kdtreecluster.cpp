//-----------------------------------------------------------------------------
//           Name: kdtreecluster.cpp
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
#include "kdtreecluster.h"

#include <Graphics/pxdebugdraw.h>
#include <Math/vec3math.h>
#include <Logging/logdata.h>

#include <algorithm>
#include <cstdio>

void GetClosestClusters( KDNode &node, const std::vector<vec3> &cluster_center, std::vector<int> candidates )
{
    node.cluster_info.closest_clusters.clear();
    if(candidates.size() == 1){
        node.cluster_info.closest_clusters.push_back(candidates[0]);
        for(unsigned i=0; i<2; ++i){
            if(node.child[i]){
                GetClosestClusters(*node.child[i], 
                                   cluster_center, 
                                   node.cluster_info.closest_clusters);
            }  
        }
    }
    vec3 node_center = node.content_bounds[0]+node.content_bounds[1];
    int closest_candidate = -1;
    float closest_dist;
    float dist;
    for(unsigned i=0; i<candidates.size(); ++i){
        dist = distance_squared(node_center, cluster_center[candidates[i]]);
        if(closest_candidate == -1 || dist < closest_dist){
            closest_dist = dist;
            closest_candidate = i;
        }
    }
    node.cluster_info.closest_clusters.clear();
    node.cluster_info.closest_clusters.push_back(candidates[closest_candidate]);
    const vec3 &closest_pos = cluster_center[candidates[closest_candidate]];
    vec3 dir;
    //int closest_point;
    //float greatest_dot_amount;
    //float dot_amount;
    vec3 corner;
    vec3 closest_corner;
    for(int i=0; i<(int)candidates.size(); ++i){
        if(i == closest_candidate){
            continue;
        }
        const vec3 &other_pos = cluster_center[candidates[i]];
        if(other_pos[0] >= node.content_bounds[0][0] && 
           other_pos[1] >= node.content_bounds[0][1] &&
           other_pos[2] >= node.content_bounds[0][2] &&
           other_pos[0] <= node.content_bounds[1][0] &&
           other_pos[1] <= node.content_bounds[1][1] &&
           other_pos[2] <= node.content_bounds[1][2])
        {
            node.cluster_info.closest_clusters.push_back(candidates[i]);
            continue;
        }
        dir = other_pos - closest_pos;
        //closest_point = -1;
        for(unsigned j=0; j<8; ++j){
            corner = vec3(node.content_bounds[j/4][0],
                          node.content_bounds[j%2][1],
                          node.content_bounds[(j/2)%2][2]);
            /*dot_amount = dot(corner, dir);
            if(closest_point == -1 || dot_amount > greatest_dot_amount){
                greatest_dot_amount = dot_amount;
                closest_point = j;
                closest_corner = corner;
            }*/
            if(distance_squared(corner, other_pos) < 
               distance_squared(corner, closest_pos))
            {
                node.cluster_info.closest_clusters.push_back(candidates[i]);
                break;
            }
        }
        /*if(distance_squared(closest_corner, other_pos) < 
           distance_squared(closest_corner, closest_pos))
        {
            node.cluster_info.closest_clusters.push_back(candidates[i]);
        }*/
    }
    for(unsigned i=0; i<2; ++i){
        if(node.child[i]){
            GetClosestClusters(*node.child[i], 
                cluster_center, 
                node.cluster_info.closest_clusters);
        }  
    }
}

//void DetailObjectSurface::Cluster()
//{
    //std::vector<vec3> points(detail_instances.size());
    //for(unsigned i=0; i<points.size(); ++i){
    //    points[i] = detail_instances[i].transform.GetTranslationPart();
    //}

    //vec3 min_bound,max_bound;
    //min_bound = points[0];
    //max_bound = points[0];
    //for(unsigned i=0; i<points.size(); ++i){
    //    for(unsigned j=0; j<3; ++j){
    //        if(points[i][j] < min_bound[j]){
    //            min_bound[j] = points[i][j];
    //        }
    //        if(points[i][j] > max_bound[j]){
    //            max_bound[j] = points[i][j];
    //        }
    //    }
    //}

    //KDNode kd_node(min_bound, max_bound, points);
    //std::vector<KDNode*> point_leaves(points.size());
    //for(unsigned i=0; i<points.size(); ++i){
    //    point_leaves[i] = kd_node.GetLeaf(points[i]);   
    //}

    //const int cluster_density = 1000;
    //unsigned num_clusters = (points.size()-1)/cluster_density+1;                // Num clusters = num points / density (rounded up)
    //std::vector<vec3> cluster_center(num_clusters);
    //std::vector<int> cluster_population(num_clusters, 0);
    //std::vector<int> cluster_assignment(points.size());
    //for(unsigned i=0; i<cluster_assignment.size(); ++i){                        // Start by assigning each point to a random cluster
    //    cluster_assignment[i] = rand()%num_clusters;
    //}
    //std::vector<int> all_candidates(num_clusters);                              // Create list of all candidate clusters
    //for(unsigned i=0; i<num_clusters; ++i){
    //    all_candidates[i] = i;
    //}

    //int num_iterations = 0;
    //bool something_changed = true;
    //while(something_changed){                                                   // Repeat k-means until nothing changes in an iteration
    //    for(unsigned i=0; i<num_clusters; ++i){
    //        cluster_center[i] = vec3(0.0f);
    //        cluster_population[i] = 0;
    //    }
    //    for(unsigned i=0; i<cluster_assignment.size(); ++i){                    // Get center position of each cluster by averaging
    //        cluster_center[cluster_assignment[i]] += points[i];                 // all member point positions
    //        ++cluster_population[cluster_assignment[i]];
    //    }
    //    for(unsigned i=0; i<num_clusters; ++i){
    //        if(cluster_population[i] != 0.0f){
    //            cluster_center[i] /= float(cluster_population[i]);
    //        }
    //    }
    //    GetClosestClusters(kd_node, cluster_center, all_candidates);            // Calculate kd-tree cluster center candidates
    //    something_changed = false;
    //    int closest_cluster;
    //    float closest_distance;
    //    float dist;
    //    for(unsigned i=0; i<points.size(); ++i){
    //        closest_cluster = -1;
    //        const std::vector<int> &closest_clusters = 
    //            point_leaves[i]->cluster_info.closest_clusters;
    //        for(unsigned j=0; j<closest_clusters.size(); ++j){
    //            dist = distance_squared(points[i], cluster_center[closest_clusters[j]]);
    //            if(closest_cluster == -1 || dist < closest_distance){
    //                closest_distance = dist;
    //                closest_cluster = closest_clusters[j];
    //            }
    //        }
    //        if(closest_cluster != cluster_assignment[i]){
    //            cluster_assignment[i] = closest_cluster;
    //            something_changed = true;
    //        }
    //    }
    //    ++num_iterations;
    //}

    //printf("Clustering completed after %d iterations\n", num_iterations);

    //std::vector<vec3> cluster_colors(num_clusters);
    //for(unsigned i=0; i<num_clusters; ++i){
    //    cluster_colors[i][0] = RangedRandomFloat(0.0f,1.0f);
    //    cluster_colors[i][1] = RangedRandomFloat(0.0f,1.0f);
    //    cluster_colors[i][2] = RangedRandomFloat(0.0f,1.0f);   
    //}
    //for(unsigned i=0; i<points.size(); ++i){
    //    DebugDraw::Instance()->AddPoint(
    //        points[i], 
    //        vec4(cluster_colors[cluster_assignment[i]],1.0f),
    //        _persistent,
    //        0);
    //}
//}

class Vec3XCompare {
public:
    bool operator()(const vec3 &a, const vec3 &b) {
        return a[0] < b[0];
    }
};

class Vec3YCompare {
public:
    bool operator()(const vec3 &a, const vec3 &b) {
        return a[1] < b[1];
    }
};


class Vec3ZCompare {
public:
    bool operator()(const vec3 &a, const vec3 &b) {
        return a[2] < b[2];
    }
};

const int _kd_split_threshold = 4;

KDNode::KDNode( const vec3 &_min_bounds, const vec3 &_max_bounds, std::vector<vec3> points, int depth )
{
    bounds[0] = _min_bounds;
    bounds[1] = _max_bounds;
    content_bounds[0] = bounds[0];
    content_bounds[1] = bounds[1];

    child[0] = NULL;
    child[1] = NULL;

    if((int)points.size() < _kd_split_threshold){
        return;
    }

    axis = depth%3;

    vec3 median_point;
    const int _rand_pick_threshold = 20;
    if((int)points.size()>_rand_pick_threshold){
        std::vector<vec3> median_points(_rand_pick_threshold);
        for(unsigned i=0; i<median_points.size(); ++i){
            median_points[i] = points[rand()%points.size()];
        }
        if(axis == 0){
            std::sort(median_points.begin(), median_points.end(), Vec3XCompare());
        } else if(axis == 1){
            std::sort(median_points.begin(), median_points.end(), Vec3YCompare());
        } else if(axis == 2){
            std::sort(median_points.begin(), median_points.end(), Vec3ZCompare());
        }

        int median = median_points.size()/2;
        plane_coord = median_points[median][axis];
        median_point = median_points[median];
    } else {
        if(axis == 0){
            std::sort(points.begin(), points.end(), Vec3XCompare());
        } else if(axis == 1){
            std::sort(points.begin(), points.end(), Vec3YCompare());
        } else if(axis == 2){
            std::sort(points.begin(), points.end(), Vec3ZCompare());
        }

        int median = points.size()/2;
        plane_coord = points[median][axis];
        median_point = points[median];
    }

    const bool _debug_draw_cut_plane = false;
    if(_debug_draw_cut_plane){
        vec4 color = vec4(vec3(1.0f),1.0f-depth*0.04f);
        vec3 plane_points[4];
        plane_points[0][axis] = plane_coord;
        plane_points[0][(axis+1)%3] = bounds[0][(axis+1)%3];
        plane_points[0][(axis+2)%3] = bounds[0][(axis+2)%3]; 
        plane_points[1][axis] = plane_coord;
        plane_points[1][(axis+1)%3] = bounds[1][(axis+1)%3];
        plane_points[1][(axis+2)%3] = bounds[0][(axis+2)%3]; 
        plane_points[2][axis] = plane_coord;
        plane_points[2][(axis+1)%3] = bounds[1][(axis+1)%3];
        plane_points[2][(axis+2)%3] = bounds[1][(axis+2)%3]; 
        plane_points[3][axis] = plane_coord;
        plane_points[3][(axis+1)%3] = bounds[0][(axis+1)%3];
        plane_points[3][(axis+2)%3] = bounds[1][(axis+2)%3]; 

        DebugDraw::Instance()->AddLine(plane_points[0], plane_points[1], color, color, _persistent);
        DebugDraw::Instance()->AddLine(plane_points[1], plane_points[2], color, color, _persistent);
        DebugDraw::Instance()->AddLine(plane_points[2], plane_points[3], color, color, _persistent);
        DebugDraw::Instance()->AddLine(plane_points[3], plane_points[0], color, color, _persistent);
    }

    vec3 child_min = _min_bounds;
    vec3 child_max = _max_bounds;
    child_max[axis] = plane_coord;
    std::vector<vec3> child_points;
    for(unsigned i=0; i<points.size(); ++i){
        if(points[i].entries[axis] < plane_coord){
            child_points.push_back(points[i]);
        }
    }
    child[0] = new KDNode(child_min, child_max, child_points, depth+1);

    child[0]->content_bounds[0] = median_point;
    child[0]->content_bounds[1] = median_point;
    for(unsigned i=0; i<child_points.size(); ++i){
        for(unsigned j=0; j<3; ++j){
            if(child_points[i][j] < child[0]->content_bounds[0][j]){
                child[0]->content_bounds[0][j] = child_points[i][j];
            }
            if(child_points[i][j] > child[0]->content_bounds[1][j]){
                child[0]->content_bounds[1][j] = child_points[i][j];
            }
        }
    }

    child_min = _min_bounds;
    child_max = _max_bounds;
    child_min[axis] = plane_coord;
    child_points.clear();
    for(unsigned i=0; i<points.size(); ++i){
        if(points[i].entries[axis] > plane_coord){
            child_points.push_back(points[i]);
        }
    }
    child[1] = new KDNode(child_min, child_max, child_points, depth+1);

    child[1]->content_bounds[0] = child_points[0];
    child[1]->content_bounds[1] = child_points[0];
    for(unsigned i=0; i<child_points.size(); ++i){
        for(unsigned j=0; j<3; ++j){
            if(child_points[i][j] < child[1]->content_bounds[0][j]){
                child[1]->content_bounds[0][j] = child_points[i][j];
            }
            if(child_points[i][j] > child[1]->content_bounds[1][j]){
                child[1]->content_bounds[1][j] = child_points[i][j];
            }
        }
    }

}

KDNode::~KDNode()
{
    delete child[0];
    delete child[1];
}

KDNode* KDNode::GetLeaf( const vec3 &point )
{
    if(point[0] < bounds[0][0] ||
        point[1] < bounds[0][1] ||
        point[2] < bounds[0][2] ||
        point[0] > bounds[1][0] || 
        point[1] > bounds[1][1] ||
        point[2] > bounds[1][2])
    {
        LOGE << "Get leaf failure" << std::endl;
        return this;
    }
    if(!child[0]){
        return this;
    }
    if(point[0] >= child[0]->bounds[0][0] && 
       point[1] >= child[0]->bounds[0][1] &&
       point[2] >= child[0]->bounds[0][2] &&
       point[0] <= child[0]->bounds[1][0] && 
       point[1] <= child[0]->bounds[1][1] &&
       point[2] <= child[0]->bounds[1][2])
    {
        return child[0]->GetLeaf(point);
    } else {
        return child[1]->GetLeaf(point);
    }
}
