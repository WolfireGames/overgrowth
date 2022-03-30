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
uniform vec4 emission;
varying vec3 normal;

void main()
{    
    float NdotL;
    vec3 color;
    
    NdotL = max(dot(normal,gl_LightSourceDEPRECATED[0].position.xyz),0.0);
    vec4 color_tex = texture2D(tex0,gl_TexCoord[0].xy);
    
    color = gl_LightSourceDEPRECATED[0].diffuse.xyz * NdotL * gl_Color.xyz;// * color_tex.xyz;
    
    NdotL = max(dot(normal,normalize(gl_LightSourceDEPRECATED[1].position.xyz)),0.0);
    
    color += gl_LightSourceDEPRECATED[1].diffuse.xyz * NdotL * gl_Color.xyz;// * color_tex.xyz;

    color += emission.xyz;
            
    gl_FragColor = vec4(color,1.0);
}
