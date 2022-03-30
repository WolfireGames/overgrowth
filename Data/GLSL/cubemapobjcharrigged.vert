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
in vec2 tex_coord_attrib;
in vec2 morph_tex_offset_attrib;
in vec2 fur_tex_coord_attrib;
in vec4 transform_mat_column_a;
in vec4 transform_mat_column_b;
in vec4 transform_mat_column_c;

#ifndef DEPTH_ONLY
uniform vec3 cam_pos;
uniform mat4 shadow_matrix[4];
#endif
uniform mat4 mvp;

out vec2 fur_tex_coord;
#ifndef DEPTH_ONLY
out vec3 concat_bone1;
out vec3 concat_bone2;
out vec2 tex_coord;
out vec2 morphed_tex_coord;
out vec3 world_vert;
#endif

void main()
{    
    mat4 concat_bone;
    concat_bone[0] = vec4(transform_mat_column_a[0], transform_mat_column_b[0], transform_mat_column_c[0], 0.0);
    concat_bone[1] = vec4(transform_mat_column_a[1], transform_mat_column_b[1], transform_mat_column_c[1], 0.0);
    concat_bone[2] = vec4(transform_mat_column_a[2], transform_mat_column_b[2], transform_mat_column_c[2], 0.0);
    concat_bone[3] = vec4(transform_mat_column_a[3], transform_mat_column_b[3], transform_mat_column_c[3], 1.0);
    // Set up varyings to pass bone matrix to fragment shader
    vec3 transformed_vertex = (concat_bone * vec4(vertex_attrib, 1.0)).xyz;

    gl_Position = mvp * vec4(transformed_vertex, 1.0);

#ifndef DEPTH_ONLY
    world_vert = transformed_vertex;
    concat_bone1 = concat_bone[0].xyz;
    concat_bone2 = concat_bone[1].xyz;

    tex_coord = tex_coord_attrib;
    morphed_tex_coord = tex_coord_attrib + morph_tex_offset_attrib;
#endif
    fur_tex_coord = fur_tex_coord_attrib;
} 
