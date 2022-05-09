//-----------------------------------------------------------------------------
//           Name: overgrowth_geometry.cpp
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Simple geometry primitives and calculations
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
#include "overgrowth_geometry.h"

#include <Math/vec4math.h>
#include <Math/enginemath.h>

#include <Graphics/camera.h>
#include <Utility/assert.h>

#include <cmath>
#include <cfloat>
#include <cassert>

vec3 Box::GetPlaneNormal(int index) {
    static const vec3 normals[] = {
        vec3(0, 0, 1),
        vec3(0, 0, -1),
        vec3(-1, 0, 0),
        vec3(1, 0, 0),
        vec3(0, -1, 0),
        vec3(0, 1, 0)};
    LOG_ASSERT(index < Box::NUM_FACES);
    return normals[index];
}

vec3 Box::GetPlanePoint(int index) const {
    LOG_ASSERT(index < Box::NUM_FACES);
    return center + GetPlaneNormal(index) * dims * 0.5f;
}

vec3 Box::GetPlaneTangent(int index) {
    static const vec3 tangents[] = {
        vec3(-1, 0, 0),
        vec3(1, 0, 0),
        vec3(0, 0, -1),
        vec3(0, 0, 1),
        vec3(-1, 0, 0),
        vec3(1, 0, 0)};
    LOG_ASSERT(index < Box::NUM_FACES);
    return tangents[index];
}

vec3 Box::GetPlaneBitangent(int index) {
    static const vec3 bitangents[] = {
        vec3(0, -1, 0),
        vec3(0, -1, 0),
        vec3(0, -1, 0),
        vec3(0, -1, 0),
        vec3(0, 0, -1),
        vec3(0, 0, -1)};
    LOG_ASSERT(index < Box::NUM_FACES);
    return bitangents[index];
}

vec3 Box::GetPoint(int index) const {
    static const vec3 box_points[] = {
        vec3(-0.5f, -0.5f, 0.5f),
        vec3(0.5f, -0.5f, 0.5f),
        vec3(0.5f, 0.5f, 0.5f),
        vec3(-0.5f, 0.5f, 0.5f),
        vec3(-0.5f, -0.5f, -0.5f),
        vec3(0.5f, -0.5f, -0.5f),
        vec3(0.5f, 0.5f, -0.5f),
        vec3(-0.5f, 0.5f, -0.5f)};

    LOG_ASSERT(index < NUM_POINTS);
    return center + (dims * box_points[index]);
}

// returns 1 if there is an intersection, -1 otherwise
int Box::lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal) {
    int hit = -1;
    float nearest_dist = FLT_MAX;
    float curr_dist = 0;
    for (int i = 0; i < NUM_FACES; i++) {
        vec3 tmp_point, tmp_normal;
        curr_dist = lineCheckFace(start, end, &tmp_point, &tmp_normal, i);
        if (curr_dist >= 0.0f && curr_dist < nearest_dist) {
            hit = 1;
            nearest_dist = curr_dist;
            *point = tmp_point;
            *normal = tmp_normal;
        }
    }
    return hit;
}

int Box::GetNearestPointIndex(const vec3& point, float& dist) const {
    // printf("x: %g\ny: %g\nz: %g\n\n", point[0], point[1], point[2]);
    bool unset = true;
    float curr_dist, least_dist = FLT_MAX;
    int index = 0;
    for (int i = 0; i < NUM_POINTS; i++) {
        curr_dist = length(GetPoint(i) - point);
        if (unset || curr_dist < least_dist) {
            unset = false;
            least_dist = curr_dist;
            index = i;
        }
    }

    dist = least_dist;
    // printf("x2: %g\ny2: %g\nz2: %g\n\n", points[index].coords[0], points[index].coords[1], points[index].coords[2]);
    return index;
}

const float _hit_face_tolerance = 0.01f;
int Box::GetHitFaceIndex(const vec3& normal, const vec3& point) const {
    for (int i = 0; i < NUM_FACES; i++) {
        const vec3 plane_normal = GetPlaneNormal(i);
        if (plane_normal[0] + _hit_face_tolerance >= normal[0] && plane_normal[0] - _hit_face_tolerance <= normal[0] &&
            plane_normal[1] + _hit_face_tolerance >= normal[1] && plane_normal[1] - _hit_face_tolerance <= normal[1] &&
            plane_normal[2] + _hit_face_tolerance >= normal[2] && plane_normal[2] - _hit_face_tolerance <= normal[2]) {
            return i;
        }
    }
    return -1;
}

bool Box::IsInFace(const vec3& point, int which_face, float dimensions_shrink) const {
    vec3 vec_from_center = point - GetPlanePoint(which_face);
    vec3 tangent = GetPlaneTangent(which_face);
    vec3 bitangent = GetPlaneBitangent(which_face);
    return (fabsf(dot(vec_from_center, tangent)) <= fabsf(dot(dims, tangent)) * 0.5f * dimensions_shrink &&
            fabsf(dot(vec_from_center, bitangent)) <= fabsf(dot(dims, bitangent)) * 0.5f * dimensions_shrink);
}

float Box::lineCheckFace(const vec3& start, const vec3& end, vec3* point, vec3* normal, int which_face) {
    vec3 plane_point = GetPlanePoint(which_face);
    vec3 plane_normal = GetPlaneNormal(which_face);
    vec3 dir = normalize(end - start);
    float hit_dist = RayPlaneIntersection(start, dir, plane_point, plane_normal);
    if (hit_dist > distance(end, start) || hit_dist < 0) {
        return -1.0f;  // Intersection point is outside of line segment
    }
    vec3 tangent = GetPlaneTangent(which_face);
    vec3 bitangent = GetPlaneBitangent(which_face);
    vec3 intersect_point = start + hit_dist * dir;
    vec3 center_to_intersect = intersect_point - plane_point;
    if (fabsf(dot(center_to_intersect, tangent)) > fabsf(dot(dims, tangent)) * 0.5f ||
        fabsf(dot(center_to_intersect, bitangent)) > fabsf(dot(dims, bitangent)) * 0.5f) {
        return -1.0f;  // No intersection: line intersects plane outside face boundaries
    }
    *point = intersect_point;
    *normal = plane_normal;
    return hit_dist;
}

bool Box::operator==(const Box& other) {
    for (int i = 0; i < 3; ++i) {
        if (center[i] != other.center[i]) {
            return false;
        }
        if (dims[i] != other.dims[i]) {
            return false;
        }
    }
    return true;
}

/*
   Modified from: http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline3d/
   Calculate the line segment PaPb that is the shortest route between
   two lines P1P2 and P3P4. Calculate also the values of mua and mub where
      Pa = P1 + mua (P2 - P1)
      Pb = P3 + mub (P4 - P3)
   Return false if no solution exists.
*/
bool LineLineIntersect(vec3 line1_p1, vec3 line1_p2, vec3 line2_p1, vec3 line2_p2, vec3* closest_on_line1, vec3* closest_on_line2) {
    vec3 p13, p43, p21;
    float d1343, d4321, d1321, d4343, d2121;
    float numer, denom;
    float mua, mub;

    p13 = line1_p1 - line2_p1;
    p43 = line2_p2 - line2_p1;
    p21 = line1_p2 - line1_p1;

    d1343 = dot(p13, p43);
    d4321 = dot(p43, p21);
    d1321 = dot(p13, p21);
    d4343 = dot(p43, p43);
    d2121 = dot(p21, p21);

    denom = d2121 * d4343 - d4321 * d4321;
    if (denom == 0) return false;
    numer = d1343 * d4321 - d1321 * d4343;

    mua = numer / denom;
    if (d4343 == 0) return false;
    mub = (d1343 + d4321 * mua) / d4343;

    if (closest_on_line1) {
        *closest_on_line1 = line1_p1 + (float)mua * p21;
    }
    if (closest_on_line2) {
        *closest_on_line2 = line2_p1 + (float)mub * p43;
    }
    return true;
}

// Return distance from ray start to intersection point with plane, or -1.0f if parallel
float RayPlaneIntersection(const vec3& ray_start, const vec3& ray_dir, const vec3& plane_point, const vec3& plane_normal) {
    if (dot(ray_dir, plane_normal) == 0.0f) {
        return -1.0f;  // ray is perpendicular to plane
    } else {
        float plane_d = dot(plane_point, plane_normal);
        float ray_d = dot(ray_start, plane_normal);
        return (plane_d - ray_d) / dot(ray_dir, plane_normal);
    }
}

bool RayLineClosestPoint(vec3 ray_start, vec3 ray_dir, vec3 line_point, vec3 line_dir, vec3* closest_point) {
    return LineLineIntersect(ray_start, ray_start + ray_dir, line_point, line_point + line_dir, NULL, closest_point);
}

// note: returns smaller of the two angles between (i.e. <= 180 degrees)
float GetAngleBetween(const vec3& v1, const vec3& v2) {
    float d = dot(normalize(v1), normalize(v2));
    if (d < -1) d = -1;
    if (d > 1) d = 1;
    return rad2degf * acosf(d);
}
