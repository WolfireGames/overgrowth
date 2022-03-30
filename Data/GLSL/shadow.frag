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

#ifdef ALPHA
uniform sampler2D tex0;
in vec2 frag_tex;
#endif

#pragma bind_out_color
out vec4 out_color;

void main() {
#ifdef ALPHA
    if(texture(tex0, frag_tex).a < 0.1) {
        discard;
    }
#endif
#ifdef ONE_FOURTH_STIPPLE
    if(int(mod(gl_FragCoord.x,2.0))!=0||int(mod(gl_FragCoord.y,2.0))!=0){
        discard;
    }
#endif
#ifdef ONE_HALF_STIPPLE
    if(int(mod(gl_FragCoord.x+gl_FragCoord.y,2.0))==0){
        discard;
    }
#endif
#ifdef THREE_FOURTH_STIPPLE
    if(int(mod(gl_FragCoord.x,2.0))!=0&&int(mod(gl_FragCoord.y,2.0))==0){
        discard;
    }
#endif
#ifdef TRI_COLOR
    out_color.x = (gl_PrimitiveID%256)/255.0;
    out_color.y = ((gl_PrimitiveID/256)%256)/255.0;
    out_color.z = ((gl_PrimitiveID/256)/256)/255.0;
    out_color.a = 0.0;
#else
    out_color = vec4(0.0,0.0,0.0,1.0);
#endif
}
