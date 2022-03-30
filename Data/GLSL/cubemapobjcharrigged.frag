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

#include "object_shared150.glsl"
#include "object_frag150.glsl"
#include "ambient_tet_mesh.glsl"

//#define USE_AMBIENT_CUBE

#ifndef DEPTH_ONLY
UNIFORM_COMMON_TEXTURES
UNIFORM_BLOOD_TEXTURE
UNIFORM_PROJECTED_SHADOW_TEXTURE
UNIFORM_TINT_TEXTURE
#endif
UNIFORM_FUR_TEXTURE

UNIFORM_LIGHT_DIR
UNIFORM_SIMPLE_SHADOW_CATCH
UNIFORM_TINT_PALETTE
uniform float time;

uniform usamplerBuffer ambient_grid_data;
uniform usamplerBuffer ambient_color_buffer;
uniform int num_light_probes;
uniform int num_tetrahedra;

uniform vec3 grid_bounds_min;
uniform vec3 grid_bounds_max;
uniform int subdivisions_x;
uniform int subdivisions_y;
uniform int subdivisions_z;

uniform vec3 ambient_cube_color[6];

uniform vec3 cam_pos;
uniform mat4 shadow_matrix[4];

in vec2 fur_tex_coord;
#ifndef DEPTH_ONLY
in vec3 concat_bone1;
in vec3 concat_bone2;
in vec2 tex_coord;
in vec2 morphed_tex_coord;
in vec3 world_vert;
#endif

#pragma bind_out_color
out vec4 out_color;

void main()
{    
    float alpha = texture(fur_tex, fur_tex_coord).a;

	vec3 ws_vertex;
	vec4 shadow_coords[4];

#ifndef DEPTH_ONLY
    ws_vertex = world_vert - cam_pos;
    shadow_coords[0] = shadow_matrix[0] * vec4(world_vert, 1.0);
    shadow_coords[1] = shadow_matrix[1] * vec4(world_vert, 1.0);
    shadow_coords[2] = shadow_matrix[2] * vec4(world_vert, 1.0);
    shadow_coords[3] = shadow_matrix[3] * vec4(world_vert, 1.0);
#endif

#ifndef ALPHA_TO_COVERAGE
    if(alpha < 0.6){
        discard;
    }
#endif

#ifdef DEPTH_ONLY
    out_color = vec4(0.0, 0.0, 0.0, alpha);
#else
    // Reconstruct third bone axis
    vec3 concat_bone3 = cross(concat_bone1, concat_bone2);

    // Get world space normal
    vec4 normalmap = texture(normal_tex, tex_coord);
    vec3 unrigged_normal = UnpackObjNormal(normalmap);
    vec3 ws_normal = normalize(concat_bone1 * unrigged_normal.x +
                               concat_bone2 * unrigged_normal.y +
                               concat_bone3 * unrigged_normal.z);

    CALC_DYNAMIC_SHADOWED
    shadow_tex.g = 1.0;
    float blood_amount, wetblood;
    ReadBloodTex(blood_tex, tex_coord, blood_amount, wetblood);
    CALC_DIRECT_DIFFUSE_COLOR

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

    if(!use_amb_cube){
        diffuse_color += LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0) *
                         GetAmbientContrib(shadow_tex.g);
    } else {
        diffuse_color += SampleAmbientCube(ambient_cube_color, ws_normal) *
                         GetAmbientContrib(shadow_tex.g);
    }

    vec3 spec_color = vec3(0.0);
    if(!use_amb_cube){
        CALC_BLOODY_CHARACTER_SPEC
    } else {
        float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,
            mix(200.0,50.0,(1.0-wetblood)*blood_amount));
        spec *= 5.0; 
        spec_color = primary_light_color.xyz * vec3(spec) * 0.3;
        vec3 spec_map_vec = reflect(ws_vertex, ws_normal);
        spec_color += SampleAmbientCube(ambient_cube_color, spec_map_vec) * 0.2 *
            GetAmbientContrib(shadow_tex.g) * max(0.0,(1.0 - blood_amount * 2.0));
    }

    vec4 colormap = texture(color_tex, morphed_tex_coord);
    vec4 tintmap = texture(tint_map, morphed_tex_coord);
    vec3 tint_mult = mix(vec3(0.0), tint_palette[0], tintmap.r) +
                     mix(vec3(0.0), tint_palette[1], tintmap.g) +
                     mix(vec3(0.0), tint_palette[2], tintmap.b) +
                     mix(vec3(0.0), tint_palette[3], tintmap.a) +
                     mix(vec3(0.0), tint_palette[4], 1.0-(tintmap.r+tintmap.g+tintmap.b+tintmap.a));
    colormap.xyz *= tint_mult;
    CALC_BLOOD_ON_COLOR_MAP
    CALC_COMBINED_COLOR
    color *= BalanceAmbient(NdotL);
    color.xyz *= 2.0;
    CALC_RIM_HIGHLIGHT
    //CALC_HAZE    
    if(!use_amb_cube){
        vec3 fog_color = textureLod(spec_cubemap,ws_vertex,5.0).xyz;
        color = mix(color, fog_color, GetHazeAmount(ws_vertex));
    } else {
        vec3 fog_color = SampleAmbientCube(ambient_cube_color, ws_vertex);
        color = mix(color, fog_color, GetHazeAmount(ws_vertex));        
    }

    CALC_FINAL_UNIVERSAL(alpha);

    //out_color.xyz = LookupCubemapSimpleLod(reflect(ws_vertex, ws_normal), spec_cubemap, 2.0);

    //gl_FragColor.xyz = vec3(sin(time)*20+20,0.0,0.0);
    //gl_FragColor.xyz = total;

    //out_color.x = (gl_PrimitiveID%256)/255.0;
    //out_color.y = ((gl_PrimitiveID/256)%256)/255.0;
    //out_color.z = ((gl_PrimitiveID/256)/256)/255.0;
#endif
}
