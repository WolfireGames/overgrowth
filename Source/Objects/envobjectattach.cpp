//-----------------------------------------------------------------------------
//           Name: envobjectattach.cpp
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
#include <Objects/envobjectattach.h>
#include <Internal/memwrite.h>

void Serialize(const std::vector<AttachedEnvObject> &aeo, std::vector<char> &data) {
    data.clear();
    int num_attach = (int)aeo.size();
    memwrite(&num_attach, sizeof(num_attach), 1, data);
    for (int i = 0; i < num_attach; ++i) {
        const AttachedEnvObject &attached_env_object = aeo[i];
        memwrite(&attached_env_object.direct_ptr, sizeof(attached_env_object.direct_ptr), 1, data);
        memwrite(&attached_env_object.legacy_obj_id, sizeof(attached_env_object.legacy_obj_id), 1, data);
        for (const auto &bone_connect : attached_env_object.bone_connects) {
            memwrite(&bone_connect.bone_id, sizeof(bone_connect.bone_id), 1, data);
            memwrite(&bone_connect.num_connections, sizeof(bone_connect.num_connections), 1, data);
            memwrite(&bone_connect.rel_mat, sizeof(bone_connect.rel_mat), 1, data);
        }
    }
}

void Deserialize(const std::vector<char> &data, std::vector<AttachedEnvObject> &aeo) {
    int index = 0;
    int num_attach;
    memread(&num_attach, sizeof(num_attach), 1, data, index);
    aeo.resize(num_attach);
    for (int i = 0; i < num_attach; ++i) {
        AttachedEnvObject &attached_env_object = aeo[i];
        attached_env_object.bone_connection_dirty = false;
        memread(&attached_env_object.direct_ptr, sizeof(attached_env_object.direct_ptr), 1, data, index);
        memread(&attached_env_object.legacy_obj_id, sizeof(attached_env_object.legacy_obj_id), 1, data, index);
        for (const auto &bone_connect : attached_env_object.bone_connects) {
            memread((void *)&bone_connect.bone_id, sizeof(bone_connect.bone_id), 1, data, index);
            memread((void *)&bone_connect.num_connections, sizeof(bone_connect.num_connections), 1, data, index);
            memread((void *)&bone_connect.rel_mat, sizeof(bone_connect.rel_mat), 1, data, index);
        }
    }
}
