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

#pragma transparent
#include "object_shared150.glsl"
#include "object_frag150.glsl"

#define base_normal_tex tex5

UNIFORM_COMMON_TEXTURES
uniform sampler2D base_normal_tex;
UNIFORM_LIGHT_DIR
UNIFORM_EXTRA_AO
UNIFORM_COLOR_TINT
uniform float wetness;

in vec4 shadow_coords[4];
in vec3 tangent;
in vec3 normal;
in vec3 ws_vertex;
in vec3 tex_coord;
in vec3 base_tex_coord;

#pragma bind_out_color
out vec4 out_color;

#define shadow_tex_coords gl_TexCoord[1].xy

void main()
{    
    vec4 colormap = texture(tex0, tex_coord.xy);
    if(tex_coord.x<0.0 || tex_coord.x>1.0 ||
       tex_coord.y<0.0 || tex_coord.y>1.0 ||
       tex_coord.z<-0.1 || tex_coord.z>0.1 ||
       colormap.a <= 0.01) 
    {
        discard;
    }
    // Calculate normal
    vec3 base_normal_tex = texture(base_normal_tex, base_tex_coord.xy).rgb;
    vec3 base_normal = normal;
    vec3 base_tangent = tangent;
    vec3 base_bitangent = normalize(cross(base_tangent,base_normal));
    base_tangent = normalize(cross(base_normal,base_bitangent));

    vec4 normalmap = texture(tex1, tex_coord.xy);
    vec3 ws_normal = vec3(base_normal * normalmap.b +
                          base_tangent * (normalmap.r*2.0-1.0) +
                          base_bitangent * (normalmap.g*2.0-1.0));
    ws_normal = normalize(ws_normal);
    
    CALC_SHADOWED
    CALC_DIFFUSE_LIGHTING

    vec3 H = normalize(normalize(ws_vertex*-1.0) + normalize(ws_light));
    float spec = min(1.0, pow(max(0.0,dot(ws_normal,H)),850.0)*pow(20.0,wetness)*0.5 * shadow_tex.r * primary_light_color.a);
    vec3 spec_color = vec3(spec);
    
    vec3 spec_map_vec = reflect(ws_vertex,ws_normal);
    spec_map_vec = reflect(ws_vertex,ws_normal);
    //spec_color += texture(tex2,spec_map_vec).xyz * 0.1;

    colormap.xyz *= mix(0.2, 0.4, max(0.0, min(1.0, wetness * 1.4 - 0.4)));

    CALC_COMBINED_COLOR_WITH_TINT
    CALC_COLOR_ADJUST
    CALC_HAZE
    CALC_FINAL_ALPHA
}
