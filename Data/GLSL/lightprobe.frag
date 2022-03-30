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
#version 150
#extension GL_ARB_shading_language_420pack : enable
#include "ambient_tet_mesh.glsl"

const int kMaxInstances = 128;

uniform LightProbeInfo {
    vec4 center[kMaxInstances];
    mat4 view_mat;
    mat4 proj_mat;
    vec3 cam_pos;
    vec4 ambient_cube_color[6*kMaxInstances];
};

flat in int instance_id;
in vec3 vert;
//in vec3 world_vert;

#pragma bind_out_color
out vec4 out_color;

void main()
{    
#ifdef STIPPLE
    if(mod(gl_FragCoord.x + gl_FragCoord.y, 2.0) == 0.0){
        discard;
    }
#endif

    vec3 acc[6];
    acc[0] = ambient_cube_color[instance_id * 6 + 0].xyz;
    acc[1] = ambient_cube_color[instance_id * 6 + 1].xyz;
    acc[2] = ambient_cube_color[instance_id * 6 + 2].xyz;
    acc[3] = ambient_cube_color[instance_id * 6 + 3].xyz;
    acc[4] = ambient_cube_color[instance_id * 6 + 4].xyz;
    acc[5] = ambient_cube_color[instance_id * 6 + 5].xyz;

    vec3 total = SampleAmbientCube(acc, normalize(vert));
    out_color.xyz = total;
    out_color.a = 1.0;
}
