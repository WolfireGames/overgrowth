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
#pragma blendmode_add

uniform sampler2D tex0;
uniform samplerCube tex3;
uniform float shadowed;

#include "lighting.glsl"

void main() {    
    vec2 coord = vec2((1.0-gl_TexCoord[0].x),(1.0-gl_TexCoord[0].y));
    vec4 colormap = texture2D(tex0,coord);
    float avg_color = (colormap[0]+colormap[1]+colormap[2])/3.0;
    colormap.xyz = colormap.xyz + (colormap.xyz - vec3(avg_color))*0.5;

    gl_FragColor = colormap;
}
