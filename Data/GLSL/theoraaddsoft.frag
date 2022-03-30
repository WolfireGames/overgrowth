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
#pragma blendmode_add
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shading_language_420pack : enable

uniform sampler2D tex0;
uniform sampler2DRect tex5;
uniform float size;

#include "lighting.glsl"

float LinearizeDepth(float z)
{
  float n = 0.1; // camera z near
  float f = 1000.0; // camera z far
  float depth = (2.0 * n) / (f + n - z * (f - n));
  return (f-n)*depth + n;
}

void main()
{        
    vec2 coord = vec2((1.0-gl_TexCoord[0].x),(1.0-gl_TexCoord[0].y));
    vec4 colormap = texture2D(tex0,coord);
    float avg_color = (colormap[0]+colormap[1]+colormap[2])/3.0;
    colormap.xyz = min(vec3(1.0),colormap.xyz + (colormap.xyz - vec3(avg_color))*0.5);

    colormap.xyz = max(vec3(0.0),colormap.xyz-vec3(0.05));

    float scale_down = 3.0;

    float env_depth = LinearizeDepth(texture2DRect(tex5,gl_FragCoord.xy).r);
    float particle_depth = LinearizeDepth(gl_FragCoord.z);
    float depth = env_depth - particle_depth;
    float depth_blend = depth / size * 0.5;
    depth_blend = (depth_blend - 0.5) * scale_down + 0.5;
    depth_blend = max(0.0,min(1.0,depth_blend));
    depth_blend *= max(0.0,min(1.0, (particle_depth-0.4)*scale_down));
    
    colormap.xyz *= vec3(depth_blend);
    gl_FragColor = colormap;
}
