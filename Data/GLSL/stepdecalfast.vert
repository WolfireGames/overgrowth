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
uniform sampler2D tex4;
uniform vec3 cam_pos;
uniform mat3 test;
uniform vec3 ws_light;

varying vec3 ws_vertex;
varying vec3 tangent_to_world1;
varying vec3 tangent_to_world2;
varying vec3 tangent_to_world3;

#include "transposemat3.glsl"
#include "pseudoinstance.glsl"

void main()
{    
    mat3 obj2worldmat3 = GetPseudoInstanceMat3();
    mat3 tan_to_obj = mat3(gl_MultiTexCoord1.xyz, gl_MultiTexCoord2.xyz, gl_Normal);
    mat3 tangent_to_world = obj2worldmat3 * tan_to_obj;
    tangent_to_world1 = normalize(tangent_to_world[0]);
    tangent_to_world2 = normalize(tangent_to_world[1]);
    tangent_to_world3 = normalize(tangent_to_world[2]);

    mat4 obj2world = GetPseudoInstanceMat4();
    
    vec4 transformed_vertex = obj2world * gl_Vertex;

    ws_vertex = transformed_vertex.xyz - cam_pos;
    
    gl_Position = gl_ModelViewProjectionMatrix * transformed_vertex;
    
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
} 
