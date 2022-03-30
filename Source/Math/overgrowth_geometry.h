//-----------------------------------------------------------------------------
//           Name: overgrowth_geometry.h
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
#pragma once

#include <Math/vec3.h>
#include <Math/vec4.h>

struct LineSegment {
    vec3 start;
    vec3 end;
};

class Box {
public:    
    static const int NUM_FACES = 6;
    static const int NUM_POINTS = 8;
    vec3 center;
    vec3 dims;

    int lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal);
    int GetNearestPointIndex(const vec3& point, float& dist) const;
    int GetHitFaceIndex(const vec3& normal, const vec3& poin) const;    // returns -1 if none hit
    bool IsInFace(const vec3& point, int which_face, float proportion_offset) const;
    vec3 GetPoint(int index) const;
    static vec3 GetPlaneNormal(int index);
    static vec3 GetPlaneTangent(int index);
    static vec3 GetPlaneBitangent(int index);
    vec3 GetPlanePoint(int index) const;
    bool operator==(const Box& other);

private:
    float lineCheckFace(const vec3& start, const vec3& end, vec3* point, vec3* normal, int which_face);
};

bool LineLineIntersect(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3* pa, vec3* pb);
float RayPlaneIntersection(const vec3& start, const vec3& dir, const vec3& p, const vec3& n);
bool RayLineClosestPoint(vec3 start, vec3 dir, vec3 p, vec3 n, vec3* closest_point);
float GetAngleBetween(const vec3& v1, const vec3& v2);
