//-----------------------------------------------------------------------------
//           Name: csg.h
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

#include <vector>

struct CSGResult {
    std::vector<int> indices;
    std::vector<int> vert_id;
    std::vector<double> bary;
    std::vector<double> verts;
};

struct CSGResultCombined {
    CSGResult result[2][2];
};

struct CSGModelInfo {
    std::vector<int> faces;
    std::vector<double> verts;
    std::vector<float> tex_coords;
    std::vector<float> tex_coords2;
};

class BulletWorld;
class Model;
class mat4;
bool CollideObjects(BulletWorld& bw, const Model &model_a, const mat4 &transform_a, const Model &model_b, const mat4 &transform_b, CSGResultCombined *results);
void AddCSGResult(const CSGResult &result, CSGModelInfo *csg_model, const Model& model, bool flip);
bool CheckShapeValid(const std::vector<int> &faces, const std::vector<double> &vertices);
void MultiplyMat4Vec3D(const mat4& mat, double vec[]);
int ModelFromCSGModelInfo(const CSGModelInfo &model_info);
