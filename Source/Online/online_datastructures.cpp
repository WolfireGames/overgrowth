//-----------------------------------------------------------------------------
//           Name: online_datastructures.cpp
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
#include "online_datastructures.h"

binn* NetworkBone::Serialize() {
    binn* l = binn_object();

    binn_object_set_float(l, "s", scale);
    binn_object_set_vec3(l, "t0", translation0);
    binn_object_set_vec3(l, "t1", translation1);
    binn_object_set_quat(l, "r0", rotation0);
    binn_object_set_quat(l, "r1", rotation1);
    binn_object_set_float(l, "mcs", model_char_scale);

    return l;
}

void NetworkBone::Deserialize(binn* l) {
    binn_object_get_float(l, "s", &scale);
    binn_object_get_vec3(l, "t0", &translation0);
    binn_object_get_vec3(l, "t1", &translation1);
    binn_object_get_quat(l, "r0", &rotation0);
    binn_object_get_quat(l, "r1", &rotation1);
    binn_object_get_float(l, "mcs", &model_char_scale);
}


binn* RiggedObjectFrame::Serialize() {
    binn* l = binn_object();

    binn_object_set_float(l, "ts", host_walltime);
    binn_object_set_uint8(l, "bc", bone_count);
    
    binn* bl_bones = binn_list();

    for(int i = 0; i < bone_count && i < bones.size(); i++) {
        binn* bo_bone = bones[i].Serialize();
        binn_list_add_object(bl_bones,bo_bone);
        binn_free(bo_bone);
    }

    binn_object_set_list(l, "bones", bl_bones);

    binn_free(bl_bones);

    return l;
}

void RiggedObjectFrame::Deserialize(binn* l) {
    binn_object_get_float(l, "ts", &host_walltime);
    binn_object_get_uint8(l, "bc", &bone_count);

    void* bl_bones;
    binn_object_get_list(l, "bones", &bl_bones);

    for(int i = 0; i < bone_count && i < bones.size(); i++) {
        void* bo_bone;
        binn_list_get_object(bl_bones, i + 1, &bo_bone);
        bones[i].Deserialize((binn*)bo_bone);
    }
}
