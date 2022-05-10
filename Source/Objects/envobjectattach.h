//-----------------------------------------------------------------------------
//           Name: envobjectattach.h
//      Developer: Wolfire Games LLC
//    Description:
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

#include <Math/mat4.h>

#include <vector>

class Object;

const unsigned kMaxBoneConnects = 4;

struct BoneConnect {
    int bone_id;
    int num_connections;
    mat4 rel_mat;
};

struct AttachedEnvObject {
    bool bone_connection_dirty;  // Used to refresh rel_mats if obj is moved in editor
    Object* direct_ptr;
    int legacy_obj_id;  // used for opening levels made before A208.2
    BoneConnect bone_connects[kMaxBoneConnects];
};

void Serialize(const std::vector<AttachedEnvObject>& aeo, std::vector<char>& data);
void Deserialize(const std::vector<char>& data, std::vector<AttachedEnvObject>& aeo);
