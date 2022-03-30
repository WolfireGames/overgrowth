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

#ifdef TEXTURE
uniform sampler2D tex0;
#endif

#ifndef COLOREDVERTICES
uniform vec4 color;
#else
in vec4 color;
#endif


#ifdef TEXTURE
in vec2 var_tex_coord; 
#endif

#pragma bind_out_color
out vec4 out_color;

void main() {    
#if defined TEXTURE
    #if defined(STAB)
        if(var_tex_coord.x < 0.0 || var_tex_coord.x > 1.0 || var_tex_coord.y < 0.0 || var_tex_coord.y > 1.0){
            discard;
        }
        vec4 tex_color = texture(tex0,var_tex_coord.xy);
        out_color = vec4(color[0] * tex_color[0], color[2], 0.0, min(1.0,color[3] * tex_color[0] * 10.0));
    #elif defined(LEVEL_SCREENSHOT)
        out_color = color * vec4(texture(tex0,var_tex_coord.xy)) * 
        pow(min(1.0, min(var_tex_coord.x, 1.0-var_tex_coord.x) * 10.0), 1.0/2.2) *
        pow(min(1.0, min(var_tex_coord.y, 1.0-var_tex_coord.y) * 10.0), 1.0/2.2) * 0.5;
        out_color.a = color.a;
    #elif defined(LOADING_LOGO)
        vec4 tex_sample = texture(tex0,var_tex_coord.xy);
        out_color = color * (vec4(1.0)-tex_sample);
        out_color.a = tex_sample.a;
    #else
        out_color = color * vec4(texture(tex0,var_tex_coord.xy));
    #endif
#else
    //out_color = vec4(1.0,0.0,0.0,1.0); //color;
    out_color = color;
#endif
}
