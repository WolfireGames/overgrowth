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
uniform vec4 color;

in vec2 var_tex_coord; 

#pragma bind_out_color
out vec4 out_color;

void main() {    
	#ifdef VR_DISPLAY
    	out_color = color * vec4(textureLod(tex0,var_tex_coord.xy,0.0)) * vec4(textureLod(tex0,var_tex_coord.xy,0.0));
	#else    
		out_color = color * vec4(texture(tex0,var_tex_coord.xy));
	#endif
}
