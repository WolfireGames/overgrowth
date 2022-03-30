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
#include 150
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shading_language_420pack : enable

uniform sampler2DRect tex0;
uniform sampler2DRect tex1;

void main()
{    
    vec3 color;
    
    vec3 color_map = texture2DRect( tex0, gl_TexCoord[0].st ).rgb;
    float depth = texture2DRect( tex1, gl_TexCoord[0].st ).r;
    
    float near = 0.1;
    float far = 1000.0;
    float distance = (near) / (far - depth * (far - near));
    //float distance = (near * far) / (far - depth * (far - near));
    //color = color_map;


    float blur = 2.0;//min(1.0,distance * 10.0);
    vec3 accum;
    
    int num_samples;
    int i,j;
    i = 0;
    j = 0;
    for (i=-10; i<10; i+=1){
        for ( j=-10; j<10; j+=1){
            if(length(vec2(float(i),float(j)))<10.0){
                accum += texture2DRect( tex0, gl_TexCoord[0].st + vec2(float(i),float(j)) * blur ).rgb;
                num_samples++;
            }
        }
    }
    accum /= float(num_samples);

    accum = color_map * accum; 
            
    float saturation = 1.0;
    float exposure = 4.0;

    float avg = (accum.r + accum.g + accum.b)/3.0;
    vec3 accum_offset = accum - vec3(avg);
    accum = vec3(avg) + accum_offset * saturation;

    accum *= exposure;
        
    //accum *= vec3(1.0-pow(length(gl_TexCoord[0].st - vec2(640,390))*0.0012,2.0));

    //accum = vec3(distance);
    gl_FragColor = vec4(accum,1.0);
}
