//-----------------------------------------------------------------------------
//           Name: mocap.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

const float M_PI = 3.14159265f;

mat4 GetTransform(vec3 offset, vec3 pos, vec3 rot) {
    mat4 translate;
    translate.SetTranslationPart(offset+pos);
    mat4 xrot;
    mat4 yrot;
    mat4 zrot;
    xrot.SetRotationX(rot.x * M_PI / 180.0f);
    yrot.SetRotationY(rot.y * M_PI / 180.0f);
    zrot.SetRotationZ(rot.z * M_PI / 180.0f);
    mat4 return_mat = translate * zrot * yrot * xrot;
    return return_mat;
}

mat4 GetCorrection(vec3 dir1, vec3 dir2){
    mat4 ident;
    if(dot(dir1, dir2)<0.0f){
        dir1 *= -1.0f;
    }


    vec3 axis = normalize(cross(dir1,dir2));
    vec3 bi_axis = normalize(cross(dir1,axis));
    float current_angle = -atan2(dot(bi_axis, dir2),
                                  dot(dir1, dir2));

    quaternion quat(vec4(axis.x, axis.y, axis.z, current_angle));
    mat4 mat = Mat4FromQuaternion(quat);
    return mat;
}

mat4 GetRot(mat4 old_rot) {
    mat4 trans;
    trans.SetRotationX(30.0f * M_PI / 180.0f);
    return trans * old_rot;// * trans;
}