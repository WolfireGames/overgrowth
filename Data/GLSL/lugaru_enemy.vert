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

#pragma use_tangent
#include "object_vert.glsl"
#include "object_shared.glsl"

UNIFORM_REL_POS
uniform float time;
VARYING_REL_POS
VARYING_SHADOW
VARYING_TAN_TO_WORLD
uniform vec3 color_tint;

void main()
{    

	float tm = time * 3.0 * color_tint.g;
	float y = gl_Vertex.y * 35.0 + gl_Vertex.x * 45.0 + gl_Vertex.z * 32.0;
	float y2 = gl_Vertex.y * 0.5 + atan( gl_Vertex.x ,  gl_Vertex.y );
	float fact = gl_Vertex.y * 0.5 + 0.5; float invFact = 1.0 - fact;

	vec4 vert = gl_Vertex;

//	vert.x += sin(y + tm * 4.0) * 0.1 * fact;
//	vert.z += sin(y + tm * 4.0) * 0.1 * fact;
//	vert.x *= (sin(y2 + tm * 1.5) * 0.5 + 0.8) * fact + invFact;       //These 4 lines Control vertex deformation
//	vert.z *= (sin(y2 + tm * 1.0) * 0.5 + 0.8) * fact + invFact;
  
  	mat4 obj2world = GetPseudoInstanceMat4(); 
	vec4 transformed_vertex = obj2world * vert; 
	gl_Position = gl_ModelViewProjectionMatrix * transformed_vertex; 

    CALC_REL_POS
    CALC_TEX_COORDS
	CALC_TAN_TO_WORLD
} 
