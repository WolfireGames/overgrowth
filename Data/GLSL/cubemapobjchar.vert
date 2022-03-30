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

in vec3 vertex_attrib;
in vec2 tex_coords_attrib;

uniform mat4 projection_view_mat;
uniform mat4 model_mat;
uniform vec3 cam_pos;
uniform mat4 shadow_matrix[4];

out vec3 ws_vertex;
out vec2 frag_tex_coords;
out vec4 shadow_coords[4];

void main() {    
    vec4 transformed_vertex = model_mat * vec4(vertex_attrib, 1.0);
    gl_Position = projection_view_mat * transformed_vertex;

    ws_vertex = transformed_vertex.xyz - cam_pos;
	frag_tex_coords = tex_coords_attrib;
    
    shadow_coords[0] = shadow_matrix[0] * transformed_vertex;
    shadow_coords[1] = shadow_matrix[1] * transformed_vertex;
    shadow_coords[2] = shadow_matrix[2] * transformed_vertex;
    shadow_coords[3] = shadow_matrix[3] * transformed_vertex;
} 
