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

uniform vec3 light_pos;
uniform sampler2D tex0; // diffuse color
uniform samplerCube tex3; // skybox
uniform sampler2D tex4; // normal map

in vec2 var_uv;

#pragma bind_out_color
out vec4 out_color;

const float texture_offset = 0.001;
const float border_fade_size = 0.1;

#include "lighting150.glsl"

void main()
{    
    // Get lighting
    vec4 normal_map = texture(tex4, var_uv + light_pos.xz * texture_offset);
    vec3 normal_vec = normalize((normal_map.xyz*vec3(2.0))-vec3(1.0));
    float NdotL = GetDirectContrib(light_pos, normal_vec, 1.0);
    vec3 color = GetDirectColor(NdotL) + LookupCubemapSimpleLod(normal_vec, tex3, 5.0) * GetAmbientContrib(1.0);

    // Combine diffuse lighting with color
    color *= texture(tex0,var_uv).xyz;    

    // Fade borders
    float alpha = 1.0;
    if(var_uv.x<border_fade_size) {
        alpha *= var_uv.x/border_fade_size;
    }
    if(var_uv.x>1.0-border_fade_size) {
        alpha *= (1.0-var_uv.x)/border_fade_size;
    }
    if(var_uv.y<border_fade_size) {
        alpha *= var_uv.y/border_fade_size;
    }
    if(var_uv.y>1.0-border_fade_size) {
        alpha *= (1.0-var_uv.y)/border_fade_size;
    }
        
    out_color = vec4(color,alpha);
}
