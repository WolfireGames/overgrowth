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
#include "lighting150.glsl"

#if defined(DRY) || defined(EXPAND) || defined(IGNITE) || defined(EXTINGUISH)
    in vec2 vert_coord;

    out vec2 tex_coord;
#else
    in vec3 vertex_attrib;
    in vec2 tex_coord_attrib;
    in vec2 morph_tex_offset_attrib;
    in vec2 fur_tex_coord_attrib;
    in vec4 transform_mat_column_a;
    in vec4 transform_mat_column_b;
    in vec4 transform_mat_column_c;
    in vec3 vel_attrib;

    uniform vec3 cam_pos;
    uniform mat4 shadow_matrix[4];
    uniform mat4 mvp;

    out vec2 fur_tex_coord;
    #ifndef DEPTH_ONLY
    out vec3 concat_bone1;
    out vec3 concat_bone2;
    out vec2 tex_coord;
    out vec2 morphed_tex_coord;
    out vec3 world_vert;
    out vec3 orig_vert;
    out vec3 vel;
    #endif
#endif

void main() {    
    #if !defined(DRY) && !defined(EXPAND) && !defined(IGNITE) && !defined(EXTINGUISH)
        mat4 concat_bone;
        concat_bone[0] = vec4(transform_mat_column_a[0], transform_mat_column_b[0], transform_mat_column_c[0], 0.0);
        concat_bone[1] = vec4(transform_mat_column_a[1], transform_mat_column_b[1], transform_mat_column_c[1], 0.0);
        concat_bone[2] = vec4(transform_mat_column_a[2], transform_mat_column_b[2], transform_mat_column_c[2], 0.0);
        concat_bone[3] = vec4(transform_mat_column_a[3], transform_mat_column_b[3], transform_mat_column_c[3], 1.0);
        // Set up varyings to pass bone matrix to fragment shader
        vec3 transformed_vertex = (concat_bone * vec4(vertex_attrib, 1.0)).xyz;

        orig_vert = vertex_attrib;
        world_vert = transformed_vertex;
        concat_bone1 = concat_bone[0].xyz;
        concat_bone2 = concat_bone[1].xyz;

        tex_coord = tex_coord_attrib;
        morphed_tex_coord = tex_coord_attrib + morph_tex_offset_attrib;
        vel = vel_attrib;

        gl_Position = vec4(morphed_tex_coord.x * 2.0 - 1.0, morphed_tex_coord.y * 2.0 - 1.0, -1.0, 1.0);

        fur_tex_coord = fur_tex_coord_attrib;
    #else
        gl_Position = vec4(vert_coord* 2.0 - 1.0, -1.0, 1.0);
        tex_coord = vert_coord.xy;
    #endif
} 
