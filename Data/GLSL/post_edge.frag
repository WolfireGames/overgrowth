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

uniform sampler2DRect tex0;
uniform sampler2DRect tex1;

void main()
{    
    vec3 color;
    
    vec3 color_map = texture2DRect( tex0, gl_TexCoord[0].st ).rgb;
    float depth = texture2DRect( tex1, gl_TexCoord[0].st ).r;
    
    vec3 accum = texture2DRect( tex0, gl_TexCoord[0].st ).rgb +
                 texture2DRect( tex0, gl_TexCoord[0].st + vec2(1,0) ).rgb * -0.25 +
                 texture2DRect( tex0, gl_TexCoord[0].st + vec2(-1,0) ).rgb * -0.25 +
                 texture2DRect( tex0, gl_TexCoord[0].st + vec2(0,1) ).rgb * -0.25 +
                 texture2DRect( tex0, gl_TexCoord[0].st + vec2(0,-1) ).rgb * -0.25;

    accum *= 100.0;

    gl_FragColor = vec4(accum,1.0);
}
