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
uniform sampler2DShadow tex0;
uniform samplerCube tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform vec3 ws_light;
uniform vec3 cam_pos;

varying vec4 ProjShadow;
varying vec3 normal;
varying vec3 ws_vertex;

#include "pseudoinstance.glsl"

void main()
{    
    mat4 obj2world = GetPseudoInstanceMat4();
    
    vec4 transformed_vertex = obj2world * gl_Vertex;
    ProjShadow = gl_TextureMatrix[0] * gl_ModelViewMatrix * transformed_vertex;

    gl_Position = gl_ModelViewProjectionMatrix * transformed_vertex;
    normal = gl_Normal;

    ws_vertex = transformed_vertex.xyz - cam_pos;

    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_TexCoord[1] = gl_MultiTexCoord3;
} 
