//-----------------------------------------------------------------------------
//           Name: binn_util.h
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

#include <Math/vec3.h>
#include <Math/quaternions.h>

#include <binn.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

void binn_object_set_vec3(binn* l, const char* key, const vec3& v);
void binn_object_set_quat(binn* l, const char* key, const quaternion& v);
void binn_object_set_mat4(binn* l, const char* key, const mat4& v);
void binn_object_set_std_string(binn* l, const char* key, const string& v);
void binn_object_set_vector_int(binn* l, const char* key, const vector<int>& arr);

bool binn_object_get_vec3(binn* l, const char* key, vec3* v);
bool binn_object_get_quat(binn* l, const char* key, quaternion* v);
bool binn_object_get_mat4(binn* l, const char* key, mat4* v);
bool binn_object_get_std_string(binn* l, const char* key, string* v);
bool binn_object_get_vector_int(binn* l, const char* key, vector<int>* arr);

void binn_list_add_std_string(binn* l, const string& v);
bool binn_list_get_std_string(binn* l, const int key, string* v);
