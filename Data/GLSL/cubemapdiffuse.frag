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
#include "lighting150.glsl"

uniform samplerCube spec_cubemap;
uniform vec4 emission;
uniform vec3 cam_pos;
uniform vec3 ws_light;
uniform vec3 color_tint;

in vec3 ws_vertex;
in vec4 shadow_coords[4];
in vec3 normal;

#pragma bind_out_color
out vec4 out_color;

void main()
{    
    float NdotL = GetDirectContrib(ws_light, normal, 1.0);
    vec3 color = GetDirectColor(NdotL);

    color += textureLod(spec_cubemap, normal, 5.0).xyz * GetAmbientContrib(1.0);
    
    color *= BalanceAmbient(NdotL);
    color *= color_tint;
    color += emission.xyz;

    CALC_HAZE
    CALC_FINAL
}
