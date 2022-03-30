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
uniform mat4 mvp_mat;
uniform float time;

in vec2 vert_coord;

#ifdef TEXTURE
in vec2 tex_coord;
out vec2 var_tex_coord; 
#endif

#ifdef COLOREDVERTICES
in vec4 vert_color;
out vec4 color;
#endif

void main() {    
    #ifdef LOADING_LOGO
        gl_Position = mvp_mat * vec4((vert_coord-vec2(0.5,0.5))*(sin(time*2.0)*0.1+1.0)+vec2(0.5,0.5),0.0,1.0);
    #else
        gl_Position = mvp_mat * vec4(vert_coord,0.0,1.0);
    #endif
#ifdef TEXTURE
    #ifdef FLIPPED
        var_tex_coord = vec2(tex_coord[0], 1.0 - tex_coord[1]);
    #else
    	var_tex_coord = tex_coord;
    #endif 
#endif 

 #ifdef COLOREDVERTICES
	color = vert_color;
#endif

} 
