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

#ifdef ARB_sample_shading_available
#extension GL_ARB_sample_shading: enable
#endif
#extension GL_ARB_shading_language_420pack : enable

#include "object_shared150.glsl"
#include "object_frag150.glsl"
#include "ambient_tet_mesh.glsl"

UNIFORM_COMMON_TEXTURES
UNIFORM_BLOOD_TEXTURE
UNIFORM_PROJECTED_SHADOW_TEXTURE

UNIFORM_LIGHT_DIR
UNIFORM_STIPPLE_FADE
UNIFORM_STIPPLE_BLUR
UNIFORM_SIMPLE_SHADOW_CATCH
UNIFORM_COLOR_TINT
uniform mat3 model_rotation_mat;

uniform usamplerBuffer ambient_grid_data;
uniform usamplerBuffer ambient_color_buffer;
uniform int num_light_probes;
uniform int num_tetrahedra;

uniform vec3 grid_bounds_min;
uniform vec3 grid_bounds_max;
uniform int subdivisions_x;
uniform int subdivisions_y;
uniform int subdivisions_z;

#ifndef DEPTH_ONLY
in vec3 ws_vertex;
in vec3 world_vert;
in vec2 frag_tex_coords;
in vec4 shadow_coords[4];
#endif

#pragma bind_out_color
out vec4 out_color;

#define tc0 frag_tex_coords

void main()
{            
#ifdef HALFTONE_STIPPLE
    CALC_HALFTONE_STIPPLE
#endif
    CALC_MOTION_BLUR
#ifdef DEPTH_ONLY
    out_color = vec4(1.0);
#else
    uint guess = 0u;
    int grid_coord[3];
    bool in_grid = true;
    for(int i=0; i<3; ++i){            
        if(world_vert[i] > grid_bounds_max[i] || world_vert[i] < grid_bounds_min[i]){
            in_grid = false;
            break;
        }
    }
    bool use_amb_cube = false;
    vec3 ambient_cube_color[6];
    if(in_grid){
        grid_coord[0] = int((world_vert[0] - grid_bounds_min[0]) / (grid_bounds_max[0] - grid_bounds_min[0]) * float(subdivisions_x));
        grid_coord[1] = int((world_vert[1] - grid_bounds_min[1]) / (grid_bounds_max[1] - grid_bounds_min[1]) * float(subdivisions_y));
        grid_coord[2] = int((world_vert[2] - grid_bounds_min[2]) / (grid_bounds_max[2] - grid_bounds_min[2]) * float(subdivisions_z));
        int cell_id = ((grid_coord[0] * subdivisions_y) + grid_coord[1])*subdivisions_z + grid_coord[2];
        uvec4 data = texelFetch(ambient_grid_data, cell_id/4);
        guess = data[cell_id%4];
        use_amb_cube = GetAmbientCube(world_vert, num_tetrahedra, ambient_color_buffer, ambient_cube_color, guess);
    } else {
        for(int i=0; i<6; ++i){
            ambient_cube_color[i] = vec3(0.0);
        }
    }
/*
    if(!use_amb_cube){
        diffuse_color += LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0) *
                         GetAmbientContrib(shadow_tex.g);
    } else {
        diffuse_color += SampleAmbientCube(ambient_cube_color, ws_normal) *
                         GetAmbientContrib(shadow_tex.g);
    }*/

    CALC_STIPPLE_FADE
    CALC_BLOOD_AMOUNT
    CALC_OBJ_NORMAL
    CALC_DYNAMIC_SHADOWED

    CALC_DIRECT_DIFFUSE_COLOR
    if(use_amb_cube){
        diffuse_color += SampleAmbientCube(ambient_cube_color, ws_normal) *
                         GetAmbientContrib(shadow_tex.g);
    } else {
        diffuse_color += LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0) *
                         GetAmbientContrib(shadow_tex.g);
    }
    
    float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,mix(100.0,50.0,(1.0-wetblood)*blood_amount)); 
    spec *= 5.0; 
    vec3 spec_color = primary_light_color.xyz * vec3(spec) * mix(1.0,0.3,blood_amount); 
    vec3 spec_map_vec = reflect(ws_vertex,ws_normal); 
    if(use_amb_cube){
        spec_color += SampleAmbientCube(ambient_cube_color, spec_map_vec) * 0.5 *
                      GetAmbientContrib(shadow_tex.g) * max(0.0,(1.0 - blood_amount * 2.0));
    } else {
        spec_color += LookupCubemapSimpleLod(spec_map_vec, tex2, 0.0) * 0.5 *
                      GetAmbientContrib(shadow_tex.g) * max(0.0,(1.0 - blood_amount * 2.0));
    }

    CALC_COLOR_MAP
    CALC_BLOOD_ON_COLOR_MAP
    CALC_COMBINED_COLOR_WITH_NORMALMAP_TINT;
    CALC_COLOR_ADJUST
    CALC_HAZE
    CALC_FINAL
#endif
}
