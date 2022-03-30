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
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shading_language_420pack : enable

uniform sampler2DRect tex;
uniform sampler2DRect tex2;

void main()
{	
	vec3 color_map = texture2DRect( tex, gl_TexCoord[0].st ).rgb;

	float brightness = (color_map.r + color_map.g + color_map.b)/3.0;
	vec3 color = vec3(brightness);

	brightness = 1.0 - brightness;
	color.g = abs(brightness-0.5);
	color.r = abs(brightness-0.9);
	color.b = abs(brightness-0.1);


	/*float depth = texture2DRect( tex2, gl_TexCoord[0].st ).r;
	
	float near = 0.1;
	float far = 1000.0;
	float distance = (near) / (far - depth * (far - near));*/

	gl_FragColor = vec4(color,1.0);
}
