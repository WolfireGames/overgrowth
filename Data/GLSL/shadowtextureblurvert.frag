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
uniform float tex_size;

void main()
{    
    float offset_size = 1.0/tex_size;
    float total_alpha = 0.0;
    vec4 color = vec4(0.0);
    vec4 contrib;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy);
    total_alpha += contrib.a*0.383;
    color += contrib*contrib.a*0.383;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy+vec2(0.0,offset_size));
    total_alpha += contrib.a*0.242;
    color += contrib*contrib.a*0.242;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy+vec2(0.0,-offset_size));
    total_alpha += contrib.a*0.242;
    color += contrib*contrib.a*0.242;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy+vec2(0.0,offset_size*2.0));
    total_alpha += contrib.a*0.061;
    color += contrib*contrib.a*0.061;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy+vec2(0.0,-offset_size*2.0));
    total_alpha += contrib.a*0.061;
    color += contrib*contrib.a*0.061;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy+vec2(0.0,offset_size*3.0));
    total_alpha += contrib.a*0.006;
    color += contrib*contrib.a*0.006;
    
    contrib = texture2D(tex0,gl_TexCoord[0].xy+vec2(0.0,-offset_size*3.0));
    total_alpha += contrib.a*0.006;
    color += contrib*contrib.a*0.006;
    
    color /= total_alpha;
    
    gl_FragColor = color;
}
