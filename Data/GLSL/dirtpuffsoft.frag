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

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform samplerCube tex3;
uniform sampler2D tex5;
uniform float size;
uniform float shadowed;
uniform vec3 ws_light;
uniform vec3 cam_pos;
uniform vec2 viewport_dims;
uniform vec4 color_tint;

in vec3 ws_vertex;
in vec2 tex_coord;
in vec3 tangent_to_world1;
in vec3 tangent_to_world2;
in vec3 tangent_to_world3;

#pragma bind_out_color
out vec4 out_color;

#include "lighting150.glsl"

float LinearizeDepth(float z)
{
  float n = 0.1; // camera z near
  float f = 1000.0; // camera z far
  float depth = (2.0 * n) / (f + n - z * (f - n));
  return (f-n)*depth + n;
}

void main()
{    
    vec3 up = vec3(0.0,1.0,0.0);
    
    vec4 colormap = texture(tex0, tex_coord);
    vec4 normalmap = texture(tex1, tex_coord);

    float env_depth = LinearizeDepth(texture(tex5,gl_FragCoord.xy / viewport_dims).r);
    float particle_depth = LinearizeDepth(gl_FragCoord.z);
    float depth = env_depth - particle_depth;
    float depth_blend = depth / size * 1.0;
    depth_blend = max(0.0,min(1.0,depth_blend));
    depth_blend *= max(0.0,min(1.0, particle_depth*0.5-0.1));
    float alpha = min(1.0,pow(colormap.a*color_tint.a*depth_blend,2.0)*2.0);

    vec3 ws_normal = vec3(tangent_to_world3 * normalmap.b +
                          tangent_to_world1 * (normalmap.r*2.0-1.0) +
                          tangent_to_world2 * (normalmap.g*2.0-1.0));
    
    float NdotL = GetDirectContrib(ws_light, ws_normal, 1.0);
    NdotL = max(NdotL, max(0.0,(1.0-alpha*0.5)));
    NdotL *= (1.0-shadowed);
    vec3 diffuse_color = GetDirectColor(NdotL);
    diffuse_color += LookupCubemapSimple(ws_normal, tex3) *
                     GetAmbientContrib(1.0);
    vec3 color = diffuse_color * colormap.xyz * color_tint.xyz;
    
    color *= BalanceAmbient(NdotL);
    AddHaze(color, ws_vertex, tex3);
    
    out_color = vec4(color,alpha);
}
