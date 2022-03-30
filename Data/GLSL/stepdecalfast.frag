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
#pragma blendmode_multiply

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform samplerCube tex2;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform vec3 cam_pos;
uniform mat3 test;
uniform vec3 ws_light;
uniform float extra_ao;

varying vec3 ws_vertex;
varying vec3 tangent_to_world1;
varying vec3 tangent_to_world2;
varying vec3 tangent_to_world3;

#include "lighting.glsl"
#include "relativeskypos.glsl"

void main()
{    
    vec4 colormap = texture2D(tex0,gl_TexCoord[0].st);
    if(gl_TexCoord[0].x<0.0 || gl_TexCoord[0].x>1.0 ||
        gl_TexCoord[0].y<0.0 || gl_TexCoord[0].y>1.0 ||
        colormap.a <= 0.05) {
        discard;
    } else {
        // Calculate normal
        vec4 normalmap = texture2D(tex1,gl_TexCoord[0].st);
        vec3 ws_normal = vec3(tangent_to_world3 * normalmap.b +
                              tangent_to_world1 * (normalmap.r*2.0-1.0) +
                              tangent_to_world2 * (normalmap.g*2.0-1.0));

        colormap.a *= 2.0 * gl_Color.a * (gl_LightSourceDEPRECATED[0].diffuse.a * 0.4 + 0.3);
        //ws_normal = tangent_to_world3;
        vec3 color = vec3(dot(ws_light, ws_normal)-dot(ws_light, tangent_to_world3)+1.0)*colormap.a;
        color *= colormap.xyz;

        gl_FragColor = vec4(color,colormap.a);
    }
}
