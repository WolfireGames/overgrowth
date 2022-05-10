//-----------------------------------------------------------------------------
//           Name: simplify_types.h
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
#include <list>

using std::list;
using std::vector;

namespace WOLFIRE_SIMPLIFY {
struct SimplifyModel {
    vector<float> vertices;
    vector<float> tex_coords;
    vector<int> vert_indices;
    vector<int> tex_indices;
    vector<int> old_vert_id;
    vector<int> old_tex_id;
};

struct MorphModel {
    SimplifyModel model;
    vector<float> vert_target;
    vector<float> tex_target;
};

struct SimplifyModelInput {
    vector<float> vertices;
    vector<float> tex_coords;
    vector<int> old_vert_id;
    vector<int> old_tex_id;
};

struct ParentRecord {
    int id;
    float weight;
};

typedef list<ParentRecord> ParentRecordList;
typedef vector<ParentRecordList> ParentRecordListVec;
}  // namespace WOLFIRE_SIMPLIFY
