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
uniform samplerCube tex2;
#ifdef BAKED_SHADOWS
    uniform sampler2D tex4;
#else
    uniform sampler2DShadow tex4;
#endif
uniform vec3 cam_pos;
uniform vec3 ws_light;
uniform float extra_ao;
uniform float fade;
uniform mat4 shadowmat;

varying vec3 ws_vertex;
varying vec3 tangent_to_world1;
varying vec3 tangent_to_world2;
varying vec3 tangent_to_world3;
#ifndef BAKED_SHADOWS
    varying vec4 shadow_coords[4];
#endif

#include "pseudoinstance.glsl"
#include "lighting.glsl"
#include "texturepack.glsl"
#include "relativeskypos.glsl"

void main() {            
    if((rand(gl_FragCoord.xy)) < fade){
        discard;
    };
    // Get normal
    vec4 normalmap = texture2D(tex1,tc0);
    vec3 unpacked_normal = UnpackTanNormal(normalmap);
    vec3 ws_normal = tangent_to_world1 * unpacked_normal.x +
                     tangent_to_world2 * unpacked_normal.y +
                     tangent_to_world3 * unpacked_normal.z;

    ws_normal = normalize(ws_normal);

    // Get diffuse lighting
#ifdef BAKED_SHADOWS
    vec3 shadow_tex = texture2D(tex4,gl_TexCoord[2].xy).rgb;
#else
    vec3 shadow_tex = vec3(1.0);
    shadow_tex.r = GetCascadeShadow(tex4, shadow_coords, length(ws_vertex));
#endif
    float NdotL = GetDirectContrib(ws_light, ws_normal, shadow_tex.r);
    vec3 diffuse_color = GetDirectColor(NdotL);
    diffuse_color += LookupCubemapSimpleLod(ws_normal, tex2, 5.0) *
                     GetAmbientContrib(shadow_tex.g);
    
    // Get specular lighting
    float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,100.0);
    spec *= 5.0;
    vec3 spec_color = gl_LightSourceDEPRECATED[0].diffuse.xyz * vec3(spec);
    
    vec3 spec_map_vec = reflect(ws_vertex,ws_normal);
    spec_color += LookupCubemapSimpleLod(spec_map_vec, tex2, 0.0) * 0.5 *
                  GetAmbientContrib(shadow_tex.g);
    
    // Put it all together
    vec4 colormap = texture2D(tex0,gl_TexCoord[0].xy);
    vec3 color = diffuse_color * colormap.xyz + spec_color * GammaCorrectFloat(colormap.a);
    
    color *= BalanceAmbient(NdotL);
    
    color *= vec3(min(1.0,shadow_tex.g*2.0)*extra_ao + (1.0-extra_ao));
    AddHaze(color, ws_vertex, tex2);

    //color = unpacked_normal;

    gl_FragColor = vec4(color,1.0);
}
