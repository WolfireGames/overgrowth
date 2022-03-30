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

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform vec2 billboardDim;

uniform mat4 mv;
uniform mat4 p;

uniform float scale;

out vec2 uv;

void main()
{
    vec2 halfsize = billboardDim/2.0 * scale;

#ifdef SCREENSPACE_SCALING
    vec4 projected = p * mv * gl_in[0].gl_Position;
    float w = projected.w;
    vec3 position = projected.xyz / w;
#else
    vec4 position = mv * gl_in[0].gl_Position;
#endif

#ifdef SCREENSPACE_SCALING
    gl_Position = vec4((position + vec3(halfsize.x,-halfsize.y,0))*w,w);
#else
    gl_Position = p * vec4(position + vec4(halfsize.x,-halfsize.y,0,0));
#endif

#ifdef FLIP_Y
    uv = vec2(1,1);
#else
    uv = vec2(1,0);
#endif
    EmitVertex();

#ifdef SCREENSPACE_SCALING
    gl_Position = vec4((position - vec3(halfsize.x,halfsize.y,0))*w,w);
#else
    gl_Position = p * vec4(position - vec4(halfsize.x,halfsize.y,0,0));
#endif

#ifdef FLIP_Y
    uv = vec2(0,1);
#else
    uv = vec2(0,0);
#endif
    EmitVertex();

#ifdef SCREENSPACE_SCALING
    gl_Position = vec4((position + vec3(halfsize.x,halfsize.y,0))*w,w);
#else
    gl_Position = p * vec4(position + vec4(halfsize.x,halfsize.y,0,0));
#endif

#ifdef FLIP_Y
    uv = vec2(1,0);
#else
    uv = vec2(1,1);
#endif
    EmitVertex();
    
#ifdef SCREENSPACE_SCALING
    gl_Position = vec4((position + vec3(-halfsize.x,halfsize.y,0))*w,w);
#else
    gl_Position = p * vec4(position + vec4(-halfsize.x,halfsize.y,0,0));
#endif

#ifdef FLIP_Y
    uv = vec2(0,0);
#else
    uv = vec2(0,1);
#endif
    EmitVertex();
}
