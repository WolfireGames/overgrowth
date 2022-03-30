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

uniform vec3 cam_pos;
uniform mat4 mvp;

in vec3 vertex_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;
in vec3 tangent_attrib;

out vec3 ws_vertex;
out vec2 tex_coord;
out vec3 tangent_to_world1;
out vec3 tangent_to_world2;
out vec3 tangent_to_world3;

void main() {    
    tangent_to_world3 = normalize(normal_attrib * -1.0);
    tangent_to_world1 = normalize(tangent_attrib);
    tangent_to_world2 = normalize(cross(tangent_to_world1,tangent_to_world3));

    ws_vertex = vertex_attrib - cam_pos;
    gl_Position = mvp * vec4(vertex_attrib, 1.0);    
    tex_coord = tex_coord_attrib;
} 
