//-----------------------------------------------------------------------------
//           Name: binn_util.cpp
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
#include "binn_util.h"

void binn_object_set_vec3(binn* l, const char* key, const vec3& v){ 
    binn* l_vec3 = binn_list();

    binn_list_add_float(l_vec3, v.x());
    binn_list_add_float(l_vec3, v.y());
    binn_list_add_float(l_vec3, v.z());

    binn_object_set_list(l, key, l_vec3);

    binn_free(l_vec3);
}

void binn_object_set_quat(binn* l, const char* key, const quaternion& v) {
    binn* l_quat = binn_list();

    binn_list_add_float(l_quat, v[0]);
    binn_list_add_float(l_quat, v[1]);
    binn_list_add_float(l_quat, v[2]);
    binn_list_add_float(l_quat, v[3]);

    binn_object_set_list(l, key, l_quat);

    binn_free(l_quat);
}

void binn_object_set_mat4(binn* l, const char* key, const mat4& v) {
    binn* l_mat4 = binn_list();

    for(int i = 0; i < 4; i++) {
        binn_list_add_float(l_mat4, v[i*4 + 0]);
        binn_list_add_float(l_mat4, v[i*4 + 1]);
        binn_list_add_float(l_mat4, v[i*4 + 2]);
        binn_list_add_float(l_mat4, v[i*4 + 3]);
    }

    binn_object_set_list(l, key, l_mat4);

    binn_free(l_mat4);
}

void binn_object_set_std_string(binn* l, const char* key, const string& v) {
    binn_object_set_str(l, key, v.c_str());
}

void binn_object_set_vector_int(binn* l, const char* key, const vector<int>& arr) {
     binn* l_vec_int = binn_list();

     for(int i = 0; i < arr.size(); i++) {
         binn_list_add_int32(l_vec_int, arr[i]);
     }

     binn_object_set_list(l, key, l_vec_int);
     binn_free(l_vec_int);
}

bool binn_object_get_vec3(binn* l, const char* key, vec3* v) {
    void* l_vec3;
    if(binn_object_get_list(l, key, &l_vec3) == false) return false;

    if(binn_list_get_float(l_vec3, 1, &v->entries[0]) == false) return false;
    if(binn_list_get_float(l_vec3, 2, &v->entries[1]) == false) return false;
    if(binn_list_get_float(l_vec3, 3, &v->entries[2]) == false) return false;

    return true;
}

bool binn_object_get_quat(binn* l, const char* key, quaternion* v) {
    void* l_quat;

    if(binn_object_get_list(l, key, &l_quat) == false) return false;

    if(binn_list_get_float(l_quat, 1, &v->entries[0]) == false) return false;
    if(binn_list_get_float(l_quat, 2, &v->entries[1]) == false) return false;
    if(binn_list_get_float(l_quat, 3, &v->entries[2]) == false) return false;
    if(binn_list_get_float(l_quat, 4, &v->entries[3]) == false) return false;

    return true;
}

bool binn_object_get_mat4(binn* l, const char* key, mat4* v) {
    void* l_mat4;

    if(binn_object_get_list(l, key, &l_mat4) == false) return false;

    for(int i = 0; i < 4; i++) {
        if(binn_list_get_float(l_mat4, i*4 + 1, &v->entries[i*4 + 0]) == false) return false;
        if(binn_list_get_float(l_mat4, i*4 + 2, &v->entries[i*4 + 1]) == false) return false;
        if(binn_list_get_float(l_mat4, i*4 + 3, &v->entries[i*4 + 2]) == false) return false;
        if(binn_list_get_float(l_mat4, i*4 + 4, &v->entries[i*4 + 3]) == false) return false;
    }

    return true;
}

bool binn_object_get_std_string(binn* l, const char* key, string* v) {
    char* c_str;
    if(binn_object_get_str(l, key, &c_str) == false) return false;

    *v = string(c_str);
    
    return true;
}

bool binn_object_get_vector_int(binn* l, const char* key, vector<int>* arr) {
    void* l_vec_int;
    binn_object_get_list(l, key, &l_vec_int);

    arr->clear();
    binn_iter iter;
    binn value;
    binn_list_foreach(l_vec_int, value) {
        int32_t v;
        binn_get_int32(&value, &v);
        arr->push_back(v); 
    }

    return true;
}

void binn_list_add_std_string(binn* l, const string& v) {
    binn_list_add_str(l, const_cast<char*>(v.c_str()));
}

bool binn_list_get_std_string(binn* l, const int pos, string* v) {
    char* c_str;
    if(binn_list_get_str(l, pos, &c_str) == false) return false;

    *v = string(c_str);
    return true;
}
