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

uniform vec3 sphere_points[32];
uniform sampler2D tex3;

uniform sampler2DRect tex0;
uniform sampler2DRect tex1;

const float near = 0.1;
const float far = 1000.0;

float DistFromDepth(float depth) {   
    return (near) / (far - depth * (far - near)) * far;
}

void TestOcclusion(in float distance, in vec2 test_point, inout float occlusion, inout float divisor){
    float depth = texture2DRect( tex1, test_point ).r;
    float temp_distance = DistFromDepth(depth);

    if(temp_distance < distance) {
        float occlusion_delta = 1.0 / (1.0 + pow(distance - temp_distance,2.0)*0.1);
        occlusion += occlusion_delta;
        divisor += occlusion_delta;
    } else {
        divisor += 1.0;
    }
}

void TestOcclusionCircle(in float radius, in float distance, in vec2 tex_point, inout float occlusion, inout float divisor, in vec3 noise){
    for(int i=0; i<32; ++i){
        vec3 sp = reflect(sphere_points[i], noise);
        //vec3 sp = sphere_points[i];
        TestOcclusion(distance + sp.z * 0.004 * radius, tex_point + sp.xy * radius, occlusion, divisor);
    }
}

void main() {    
    vec2 tex_point = gl_TexCoord[0].st;
    vec3 color_map = texture2DRect( tex0, tex_point ).rgb;
    float depth = texture2DRect( tex1, tex_point ).r;
    vec3 noise = normalize(texture2D( tex3, gl_TexCoord[0].st/256.0 ).rgb - vec3(0.5,0.5,0.5));
    
    float distance = DistFromDepth(depth);
    
    float occlusion = 0.0;
    float divisor = 0.0;
    TestOcclusionCircle(min(64.0,max(32.0,512.0/distance)), distance, tex_point, occlusion, divisor, noise); 
    TestOcclusionCircle(min(32.0,max(8.0,320./distance)), distance, tex_point, occlusion, divisor, noise); 
    occlusion /= divisor;
    
    occlusion = min(1.0,pow((1.0-occlusion) * 1.0,1.0));
    occlusion = mix(occlusion, 1.0, distance/far);
    color_map *= vec3(occlusion*1.4);

    //color_map.xyz = vec3(pow(1.0-occlusion,3.0));

    //color_map.xyz = vec3(abs(10.0-(distance/(1.0+length(vert.xy)))));

    gl_FragColor = vec4(color_map,1.0);
}
