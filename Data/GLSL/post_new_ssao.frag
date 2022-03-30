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
#extension GL_ARB_gpu_shader5 : enable
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shading_language_420pack : enable

uniform mat4 proj_mat;
uniform mat4 view_mat;
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

vec4 ScreenCoordFromDepth(vec2 tex_uv, vec2 offset, out float distance) {
	float depth = texture2DRect( tex1, tex_uv + offset ).r;
	distance = DistFromDepth( depth );
	return vec4((tex_uv[0] + offset[0]) / 1280.0 * 2.0 - 1.0, (tex_uv[1] + offset[1]) / 720.0 * 2.0 - 1.0, depth * 2.0- 1.0, 1.0);
}

void CheckSphere(in vec4 world_pos, in vec3 norm, in vec3 noise, in float ray_length, inout float total_divisor, inout float total_light) {
	for(int i=0; i<32; ++i){
		vec3 vec = reflect(sphere_points[i], noise);
		float weight = dot(vec, norm);
		if(weight > 0.0){
			vec3 world_pos_test = world_pos.xyz + vec * ray_length;
			vec4 screen_coord_test = (proj_mat * view_mat * vec4(world_pos_test, world_pos.w));
			screen_coord_test /= screen_coord_test.w;
			screen_coord_test[0] = (screen_coord_test[0] + 1.0) / 2.0 * 1280.0;
			screen_coord_test[1] = (screen_coord_test[1] + 1.0) / 2.0 * 720.0;
			screen_coord_test[2] = (screen_coord_test[2] + 1.0) / 2.0;
			if(screen_coord_test[0] < 0.0 || screen_coord_test[0] > 1280.0 || screen_coord_test[1] < 0.0 || screen_coord_test[1] > 720.0){
			} else {
				float test_depth = texture2DRect( tex1, screen_coord_test.xy ).r;
				total_divisor += weight;
				float true_dist = DistFromDepth(test_depth);
				float test_dist = DistFromDepth(screen_coord_test[2]);
				float offset = true_dist - test_dist;
				if(offset > 0.0 || offset < -ray_length){
					total_light += weight;
				}
			}
		}
    }
}

void main() {    
    vec2 tex_point = gl_TexCoord[0].st;
    vec3 color_map = texture2DRect( tex0, tex_point ).rgb;
    vec3 noise = normalize(texture2D( tex3, gl_TexCoord[0].st/256.0 ).rgb - vec3(0.5,0.5,0.5));
    
    float distance_1 = DistFromDepth(texture2DRect( tex1, tex_point ).r);
    float distance_2 = DistFromDepth(texture2DRect( tex1, tex_point+vec2(0.01,0) ).r);
    float distance_3 = DistFromDepth(texture2DRect( tex1, tex_point+vec2(0,0.01) ).r);

    vec3 a = vec3(0,0,distance_1);
    vec3 b = vec3(1,0,distance_2);
    vec3 c = vec3(0,1,distance_3);

    vec3 norm = normalize(cross(normalize(b-a), normalize(c-a)));

    norm *= 0.5;
    norm += 0.5;

    float dist;
    float dist1;
    float dist2;
    float depth = texture2DRect( tex1, tex_point ).r;
    vec4 screen_coord = ScreenCoordFromDepth(tex_point, vec2(0,0), dist);
	vec4 screen_coord1 = ScreenCoordFromDepth(tex_point, vec2(1.0,0), dist1);
	vec4 screen_coord2 = ScreenCoordFromDepth(tex_point, vec2(0,1.0), dist2);
	
	mat4 temp_view = view_mat;	
	//temp_view[3][0] *= 2.0;
	//temp_view[3][1] *= 2.0;
	//temp_view[3][2] *= 2.0;

    vec4 world_pos = inverse(proj_mat * temp_view) * screen_coord;
    vec4 world_pos1 = inverse(proj_mat * temp_view) * screen_coord1;
    vec4 world_pos2 = inverse(proj_mat * temp_view) * screen_coord2;

    world_pos *= dist;
    world_pos1 *= dist1;
    world_pos2 *= dist2;

	norm = normalize(cross(world_pos1.xyz-world_pos.xyz, world_pos2.xyz-world_pos.xyz));
	//norm = (view_mat * vec4(norm, 0.0)).xyz;


	float total_divisor = 0.000001;
	float total_light = 0.0;
	float ray_length = 0.05;
	float sphere_mult = pow(dist, 0.6);
	for(int i=0; i<5; ++i){
		ray_length = 0.2 * pow(2,i) * sphere_mult;
		CheckSphere(world_pos, norm, noise, ray_length, total_divisor, total_light);
		//vec3 noise_2 = normalize(texture2D( tex3, (gl_TexCoord[0].st + vec2(16,12))/256.0 ).rgb - vec3(0.5,0.5,0.5));
		//CheckSphere(world_pos, norm, noise_2, ray_length, total_divisor, total_light);
	}


    float ao_mult = total_light/total_divisor;
    ao_mult = 1.0 - (1.0 - ao_mult) * 2.0;
    if(dist > 500.0){
    	ao_mult = min(1.0, mix(ao_mult, 1.0, (dist-500.0)/50.0));
    }
    vec3 ao_mult_effect = mix(vec3(0.3,0.25,0.2),vec3(1.0), ao_mult);
    //ao_mult = 1.0;
    gl_FragColor.xyz = color_map * ao_mult_effect;
    //gl_FragColor.xyz = color_map;
}
