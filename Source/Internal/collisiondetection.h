//-----------------------------------------------------------------------------
//           Name: collisiondetection.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This file stores collision detection functions
//                 (mostly scavenged from the internet)
//        License: Read below
//-----------------------------------------------------------------------------
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

class Object;
class Collision {
   public:
    Collision();

    bool hit;
    Object *hit_what;
    int hit_how;
    vec3 hit_normal;
    vec3 hit_where;
};

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

int LineFacet(const vec3 &p1, const vec3 &p2, const vec3 &pa, const vec3 &pb, const vec3 &pc, vec3 *p);
int LineFacetNoBackface(const vec3 &p1, const vec3 &p2, const vec3 &pa, const vec3 &pb, const vec3 &pc, vec3 *p, const vec3 &n);
int LineFacet(const vec3 &p1, const vec3 &p2, const vec3 &pa, const vec3 &pb, const vec3 &pc, vec3 *p, const vec3 &n);
int LineFacet(vec3 *p1, vec3 *p2, vec3 *pa, vec3 *pb, vec3 *pc, vec3 *p);
int LineFacet(vec3 *p1, vec3 *p2, vec3 *pa, vec3 *pb, vec3 *pc, vec3 *p, vec3 *n);

bool inTriangle(const vec3 &pointv, const vec3 &normal, const vec3 &p1v, const vec3 &p2v, const vec3 &p3v);
vec3 barycentric(const vec3 &pointv, const vec3 &normal, const vec3 &p1v, const vec3 &p2v, const vec3 &p3v);

bool sphere_line_intersection(const vec3 &p1, const vec3 &p2, const vec3 &p3, const float &r, vec3 *ret = 0);

bool lineBox(const vec3 &start, const vec3 &end, const vec3 &box_min, const vec3 &box_max, float *time);

bool triBoxOverlap(const vec3 &min, const vec3 &max, const vec3 &vert1, const vec3 &vert2, const vec3 &vert3);

int coplanar_tri_tri_OG(float N[3], float V0[3], float V1[3], float V2[3],
                        float U0[3], float U1[3], float U2[3]);

int tri_tri_intersect(float V0[3], float V1[3], float V2[3],
                      float U0[3], float U1[3], float U2[3]);

int NoDivTriTriIsect(float V0[3], float V1[3], float V2[3],
                     float U0[3], float U1[3], float U2[3]);

int tri_tri_intersect_with_isectline(float V0[3], float V1[3], float V2[3],
                                     float U0[3], float U1[3], float U2[3], int *coplanar,
                                     float *isectpt1, float *isectpt2);

bool PointInTriangle(vec3 &point, vec3 *tri_point[3]);

bool LineIntersection2D(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4);

bool LineSquareIntersection2D(vec3 &min, vec3 &max, vec3 &start, vec3 &end);

bool TriangleSquareIntersection2D(vec3 &min, vec3 &max, vec3 *tri_point[3]);

bool DistancePointLine(const vec3 &Point, const vec3 &LineStart, const vec3 &LineEnd, float *Distance, vec3 *Intersection);
