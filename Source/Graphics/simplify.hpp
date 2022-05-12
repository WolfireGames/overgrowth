//-----------------------------------------------------------------------------
//           Name: simplify.hpp
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

#include <Wrappers/glm.h>
#include <Graphics/model.h>
#include <Graphics/halfedge.h>
#include <Graphics/simplify_types.h>

#include <vector>
#include <list>

using std::vector;

namespace WOLFIRE_SIMPLIFY {
void CalculateQuadrics(vector<glm::mat4>* quadrics, const vector<float>& vertices, const vector<int>& vert_indices);
bool SimplifyMorphLOD(const SimplifyModelInput& model_input, MorphModel* lod, ParentRecordListVec* vert_parents, ParentRecordListVec* tex_parents, int lod_levels);
bool SimplifySimpleModel(const Model& input, Model* output, int edge_target, bool include_tex);
bool Process(const Model& model, SimplifyModel& processed_model, vector<HalfEdge>& half_edges, bool include_tex);
float CalculateError(float* pos, int edge[2], const vector<float>& vertices, const vector<glm::mat4>& quadrics, const int edge_tc[]);
}  // namespace WOLFIRE_SIMPLIFY
