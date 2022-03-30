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
#pragma use_tangent

#include "lighting150.glsl"

uniform vec3 cam_pos;
uniform vec3 ws_light;
uniform float time;
uniform mat4 transforms[40];
uniform vec4 texcoords2[40];
uniform float height;
uniform float max_distance;
uniform float plant_shake;
uniform mat4 shadow_matrix[4];
uniform mat4 projection_view_mat;

in float index;
in vec3 vertex_attrib;
in vec2 tex_coords_attrib;
in vec3 tangent_attrib;
in vec3 bitangent_attrib;
in vec3 normal_attrib;

out vec2 frag_tex_coords;
out vec2 base_tex_coord;
out mat3 tangent_to_world;
out vec3 ws_vertex;
out vec4 shadow_coords[4];
out vec3 world_vert;

#define TERRAIN_LIGHT_OFFSET vec2(0.0005)+ws_light.xz*0.0005

void main()
{    
    mat4 obj2world = transforms[int(index)];
    vec4 transformed_vertex = obj2world*vec4(vertex_attrib, 1.0);
#ifdef PLANT
    vec3 vertex_offset = CalcVertexOffset(transformed_vertex, vertex_attrib.y*2.0, time, plant_shake);
    vertex_offset.y *= 0.2;
#endif

    mat3 obj2worldmat3 = mat3(normalize(obj2world[0].xyz), 
                              normalize(obj2world[1].xyz), 
                              normalize(obj2world[2].xyz));
    mat3 tan_to_obj = mat3(tangent_attrib, bitangent_attrib, normal_attrib);
    tangent_to_world = obj2worldmat3 * tan_to_obj;

    world_vert = transformed_vertex.xyz;
    ws_vertex = transformed_vertex.xyz - cam_pos;
     
    vec4 aux = texcoords2[int(index)];

    float embed = aux.z;
    float height_scale = aux.a;
    transformed_vertex.y -= max(embed,length(ws_vertex)/max_distance)*height*height_scale;
#ifdef PLANT
    transformed_vertex += obj2world * vec4(vertex_offset,0.0);
#endif
    gl_Position = projection_view_mat * transformed_vertex;

    frag_tex_coords = tex_coords_attrib;
    base_tex_coord = aux.xy+TERRAIN_LIGHT_OFFSET;
    
    shadow_coords[0] = shadow_matrix[0] * vec4(transformed_vertex);
    shadow_coords[1] = shadow_matrix[1] * vec4(transformed_vertex);
    shadow_coords[2] = shadow_matrix[2] * vec4(transformed_vertex);
    shadow_coords[3] = shadow_matrix[3] * vec4(transformed_vertex);
} 
