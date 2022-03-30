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

    float shadow = texture2D(tex0,gl_TexCoord[0].xy).r;
    float float_i = 0.0;
    for(int i=0; i<8; i++){
        float_i += 1.0;
        vec2 new_uv = gl_TexCoord[0].xy+vec2(0.0,-offset_size)*float_i*1.0;
        new_uv.y = max(0.01,new_uv.y);
        shadow = max(shadow,texture2D(tex0,new_uv).r);
    }

    gl_FragColor = vec4(vec3(shadow),1.0);
}
