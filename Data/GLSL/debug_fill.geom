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

layout(triangles) in;
layout (triangle_strip, max_vertices=3) out;

in vec3 world_position[3];

out vec3 normal_frag;
out vec3 position_frag;

void main()
{
    vec3 n = cross(world_position[1].xyz-world_position[0].xyz, world_position[2].xyz-world_position[0].xyz);
    normal_frag = normalize(n);

    position_frag = world_position[0];
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    position_frag = world_position[1];
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    position_frag = world_position[2];
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

}



