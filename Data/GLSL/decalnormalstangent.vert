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
uniform vec3 light_pos;

uniform sampler2D tex;
uniform sampler2D tex2;
uniform samplerCube tex4;
uniform sampler2D tex5;

uniform mat4 obj2world;

varying vec3 normal;
varying vec3 tangent;
varying vec3 bitangent;

void main()
{	
	normal = normalize(gl_Normal);

	mat3 obj2world3 = mat3(obj2world[0].xyz, obj2world[1].xyz, obj2world[2].xyz);
	
	tangent = obj2world3*normalize(gl_MultiTexCoord1.xyz);
	bitangent = obj2world3*normalize(gl_MultiTexCoord2.xyz);
	
	gl_Position = ftransform();
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord3;
} 
