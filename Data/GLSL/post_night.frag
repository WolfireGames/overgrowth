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
uniform sampler2D tex2;

uniform float time;

void main()
{    
    vec3 color;
    
    vec3 color_map = texture2DRect( tex0, gl_TexCoord[0].st ).rgb;
    vec3 noise = texture2D( tex2, gl_TexCoord[0].st/512.0+vec2(time*10.0,time*1.5) ).rgb;
    float depth = texture2DRect( tex1, gl_TexCoord[0].st ).r;
    
    float near = 0.1;
    float far = 1000.0;
    float distance = (near) / (far - depth * (far - near));

    float blur = 1.0-distance*0.5;
    vec3 accum;
    float total_weight = 0.0;
    int i,j;
    i = 0;
    j = 0;
    for (i=-2; i<2; i+=1){
        for ( j=-2; j<2; j+=1){
            vec2 uv_coord = gl_TexCoord[0].st + vec2(float(i)+noise.r*3.0,float(j)+noise.g*3.0)* blur ;
            float sample_depth = texture2DRect( tex1, uv_coord ).r;
            float sample_distance = (near) / (far - sample_depth * (far - near));
            float diff = abs(distance - sample_distance);
            float weight = max(0.11,1.0-diff*2000.0);
            accum += texture2DRect( tex0, uv_coord).rgb * weight;
            total_weight+=weight;
        }
    }
    accum /= float(total_weight);
        
    float saturation = 1.0;
    float exposure = 1.0;
    float noise_amount = 0.4;

    accum *= (1.0-noise_amount)+vec3(noise.r)*noise_amount*2.0;
    float avg = (accum.r + accum.g + accum.b)/3.0;
    vec3 accum_offset = accum - vec3(avg);
    accum = vec3(avg) + accum_offset * saturation;
    accum *= exposure;

    gl_FragColor = vec4(accum,1.0);
}
