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

/*#if defined(WATER)
#define NO_DECALS
#endif
*/
#define FIRE_DECAL_ENABLED
//#define RAINY

uniform float time;
uniform vec3 cam_pos;

//#define VOLCANO
//#define MISTY
//#define WATER_HORIZON

#if defined(SWAMP)
    const float kBeachLevel = 17.1;
#endif

#if defined(CAVE_ARENA)
    const float kBeachLevel = 101.31;
#endif

#if defined(SWAMP2)
    #if defined(MORE_REFLECT)
        const float kBeachLevel = -2.2;
    #else
        const float kBeachLevel = 83;
    #endif
#endif

#if defined(WATER_HORIZON)
    #if defined(ALT)
        const float kBeachLevel = 22.0;
    #else
        const float kBeachLevel = -4;
    #endif
#endif

#if defined(SNOW_EVERYWHERE)
    const float kBeachLevel = -3.3;
#endif

#if defined(SKY_ARK)
    const float kBeachLevel = 75.4;
#endif

//#define VOLUME_SHADOWS

#include "object_frag150.glsl"
#include "object_shared150.glsl"
#include "ambient_tet_mesh.glsl"

#if defined(PARTICLE)
    uniform sampler2D tex0; // Colormap

    #if !defined(INSTANCED)
        uniform vec4 color_tint;
    #endif

    #if !defined(DEPTH_ONLY)
        uniform sampler2D tex1; // Normalmap
        uniform samplerCube tex2; // Diffuse cubemap
        uniform samplerCube tex3; // Diffuse cubemap
        uniform sampler2D tex5; // Screen depth texture TODO: make this work with msaa properly

        UNIFORM_SHADOW_TEXTURE

        UNIFORM_LIGHT_DIR

        #if !defined(INSTANCED)
            uniform float size;
        #endif

        uniform vec2 viewport_dims;
        uniform sampler3D tex16;

        #if defined(NO_REFLECTION_CAPTURE)
            #define ref_cap_cubemap tex0
        #else
            uniform sampler2DArray tex19;
            #define ref_cap_cubemap tex19
        #endif

        uniform mat4 reflection_capture_matrix[10];
        uniform mat4 reflection_capture_matrix_inverse[10];
        uniform int reflection_capture_num;

        uniform mat4 light_volume_matrix[10];
        uniform mat4 light_volume_matrix_inverse[10];
        uniform int light_volume_num;

        uniform float haze_mult;
    #endif

    #if defined(INSTANCED)
        const int kMaxInstances = 100;

        uniform InstanceInfo {
            vec4 instance_color[kMaxInstances];
            mat4 instance_transform[kMaxInstances];
        };
    #endif
#elif defined(GPU_PARTICLE_FIELD)
    UNIFORM_SHADOW_TEXTURE

    uniform samplerCube tex3;
    uniform vec3 ws_light;
#elif defined(SKY)
    uniform samplerCube tex0;
    uniform samplerCube tex1;
    uniform samplerCube tex2;

    UNIFORM_SHADOW_TEXTURE

    uniform vec3 tint;
    uniform float fog_amount;
    uniform float haze_mult;
    uniform vec3 ws_light;
#elif defined(DETAIL_OBJECT)
    #if defined(PLANT)
        #pragma transparent
    #endif

    #define base_color_tex tex6
    #define base_normal_tex tex7

    UNIFORM_COMMON_TEXTURES

    #if defined(PLANT)
        UNIFORM_TRANSLUCENCY_TEXTURE
    #endif

    uniform sampler2D base_color_tex;
    uniform sampler2D base_normal_tex;
    uniform float overbright;
    uniform float max_distance;
    uniform float tint_weight;
    uniform sampler2D tex18;

    UNIFORM_LIGHT_DIR

    uniform vec3 avg_color;
    uniform vec3 color_tint;

    uniform mat3 normal_matrix;
    uniform sampler3D tex16;

    uniform float haze_mult;

    #define tc0 frag_tex_coords
    #define tc1 base_tex_coord
#else
    UNIFORM_COMMON_TEXTURES

    #if defined(PLANT)
        UNIFORM_TRANSLUCENCY_TEXTURE
    #endif

    UNIFORM_LIGHT_DIR

    #if defined(DETAILMAP4)
        UNIFORM_DETAIL4_TEXTURES

        UNIFORM_AVG_COLOR4
    #endif

    #if defined(TERRAIN)
        uniform sampler2D tex14;
        #define warp_tex tex14
    #endif

    #if defined(CHARACTER)
        UNIFORM_BLOOD_TEXTURE
        UNIFORM_TINT_TEXTURE
        UNIFORM_FUR_TEXTURE
        UNIFORM_TINT_PALETTE
    #endif

    #if defined(ITEM)
        UNIFORM_BLOOD_TEXTURE
        UNIFORM_COLOR_TINT

        uniform mat3 model_rotation_mat;
    #endif

    uniform sampler3D tex16;
    uniform sampler2D tex17;
    uniform sampler2D tex18;

    #if defined(NO_REFLECTION_CAPTURE)
        #define ref_cap_cubemap tex0
    #else
        uniform sampler2DArray tex19;

        #define ref_cap_cubemap tex19
    #endif

    uniform mat4 reflection_capture_matrix[10];
    uniform mat4 reflection_capture_matrix_inverse[10];
    uniform int reflection_capture_num;

    uniform mat4 light_volume_matrix[10];
    uniform mat4 light_volume_matrix_inverse[10];
    uniform int light_volume_num;
    uniform mat4 prev_projection_view_mat;

    uniform float haze_mult;

    //#define EMISSIVE

    #if defined(TERRAIN)
    #elif defined(CHARACTER) || defined(ITEM)
    #else
        #define INSTANCED_MESH

        #if !defined(ATTRIB_ENVOBJ_INSTANCING)
            #if defined(UBO_BATCH_SIZE_8X)
                const int kMaxInstances = 256 * 8;
            #elif defined(UBO_BATCH_SIZE_4X)
                const int kMaxInstances = 256 * 4;
            #elif defined(UBO_BATCH_SIZE_2X)
                const int kMaxInstances = 256 * 2;
            #else
                const int kMaxInstances = 256 * 1;
            #endif

            struct Instance {
                vec3 model_scale;
                vec4 model_rotation_quat;
                vec4 color_tint;
                vec4 detail_scale;  // TODO: DETAILMAP4 only?
            };

            uniform InstanceInfo {
                Instance instances[kMaxInstances];
            };
        #endif
    #endif
#endif // PARTICLE

#if defined(CAN_USE_LIGHT_PROBES)
    uniform usamplerBuffer ambient_grid_data;
    uniform usamplerBuffer ambient_color_buffer;
    uniform int num_light_probes;
    uniform int num_tetrahedra;

    uniform vec3 grid_bounds_min;
    uniform vec3 grid_bounds_max;
    uniform int subdivisions_x;
    uniform int subdivisions_y;
    uniform int subdivisions_z;
#endif

uniform mat4 shadow_matrix[4];
uniform mat4 projection_view_mat;

#include "decals.glsl"

in vec3 world_vert;

#if defined(PARTICLE)
    in vec2 tex_coord;

    #if defined(NORMAL_MAP_TRANSLUCENT) || defined(WATER) || defined(SPLAT)
        in vec3 tangent_to_world1;
        in vec3 tangent_to_world2;
        in vec3 tangent_to_world3;
    #endif

    flat in int instance_id;
#elif defined(GPU_PARTICLE_FIELD)
    in vec2 tex_coord;
    in float alpha;
    flat in int instance_id;
#elif defined(DETAIL_OBJECT)
    in vec2 frag_tex_coords;
    in vec2 base_tex_coord;
    in mat3 tangent_to_world;
#elif defined(ITEM)
    #if !defined(DEPTH_ONLY)
        #if !defined(NO_VELOCITY_BUF)
            in vec3 vel;
        #endif

        #if defined(TANGENT)
            in vec3 frag_normal;
        #endif
    #endif

    in vec2 frag_tex_coords;
#elif defined(TERRAIN)
    #if defined(DETAILMAP4)
        in vec3 frag_tangent;
    #endif

    #if !defined(SIMPLE_SHADOW)
        //in float alpha;
    #endif

    in vec4 frag_tex_coords;
#elif defined(CHARACTER)
    in vec2 fur_tex_coord;

    #if defined(TANGENT)
        in mat3 tan_to_obj;
    #endif

    #if !defined(DEPTH_ONLY)
        in vec3 concat_bone1;
        in vec3 concat_bone2;
        in vec2 tex_coord;
        in vec2 morphed_tex_coord;
        in vec3 orig_vert;

        #if !defined(NO_VELOCITY_BUF)
            in vec3 vel;
        #endif
    #endif
#elif defined(SKY)
    // Section empty right now
#else // static object

    #if defined(ATTRIB_ENVOBJ_INSTANCING)
        flat in vec3 model_scale_frag;
        flat in vec4 model_rotation_quat_frag;
        flat in vec4 color_tint_frag;

        #if defined(DETAILMAP4)
            flat in vec4 detail_scale_frag;
        #endif
    #endif

    vec3 GetInstancedModelScale(int instance_id) {
        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            return model_scale_frag;
        #else
            return instances[instance_id].model_scale;
        #endif
    }

    vec4 GetInstancedModelRotationQuat(int instance_id) {
        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            return model_rotation_quat_frag;
        #else
            return instances[instance_id].model_rotation_quat;
        #endif
    }

    vec4 GetInstancedColorTint(int instance_id) {
        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            return color_tint_frag;
        #else
            return instances[instance_id].color_tint;
        #endif
    }

    #if defined(DETAILMAP4)
        vec4 GetInstancedDetailScale(int instance_id) {
            #if defined(ATTRIB_ENVOBJ_INSTANCING)
                return detail_scale_frag;
            #else
                return instances[instance_id].detail_scale;
            #endif
        }
    #endif

    #if defined(TANGENT)
        #if defined(USE_GEOM_SHADER)
            in mat3 tan_to_obj_fs;

            #define tan_to_obj tan_to_obj_fs
        #else
            in mat3 tan_to_obj;
        #endif
    #endif

    #if defined(USE_GEOM_SHADER)
        in vec2 frag_tex_coords_fs;

        #define frag_tex_coords frag_tex_coords_fs
    #else
        in vec2 frag_tex_coords;
    #endif

    #if !defined(NO_INSTANCE_ID)
        flat in int instance_id;
    #endif
#endif

#pragma bind_out_color
out vec4 out_color;

#if !defined(NO_VELOCITY_BUF)
    #pragma bind_out_vel
    out vec4 out_vel;
#endif  // NO_VELOCITY_BUF

#define shadow_tex_coords tc1
#define tc0 frag_tex_coords

//#if defined(PARTICLE)

#if defined(SKY) && defined(YCOCG_SRGB)
    vec3 YCOCGtoRGB(in vec4 YCoCg) {
        float Co = YCoCg.r - 0.5;
        float Cg = YCoCg.g - 0.5;
        float Y  = YCoCg.a;

        float t = Y - Cg * 0.5;
        float g = Cg + t;
        float b = t - Co * 0.5;
        float r = b + Co;

        r = max(0.0,min(1.0,r));
        g = max(0.0,min(1.0,g));
        b = max(0.0,min(1.0,b));

        return vec3(r,g,b);
    }
#endif

float LinearizeDepth(float z) {
    float n = 0.1; // camera z near
    float epsilon = 0.000001;
    float z_scaled = z * 2.0 - 1.0; // Scale from 0 - 1 to -1 - 1
    float B = (epsilon-2.0)*n;
    float A = (epsilon - 1.0);
    float result = B / (z_scaled + A);

    if(result < 0.0){
        result = 99999999.0;
    }

    return result;
}


float UnLinearizeDepth(float result) {
    float n = 0.1; // camera z near
    float epsilon = 0.000001;
    float B = (epsilon-2.0)*n;
    float A = (epsilon - 1.0);
    float z_scaled = B / result - A;
    float z = (z_scaled + 1.0) * 0.5;
    return z;
}
//#endif
//#endif

#if defined(TEST_CLOUDS_2)
    const float cloud_speed = 0.1;
#endif

#if defined(SKY)

    vec2 star_hash( vec2 p ) {
        p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
        return -1.0 + 2.0*fract(sin(p)*43758.5453123);
    }

    float star_noise( in vec2 p ) {
        const float K1 = 0.366025404; // (sqrt(3)-1)/2;
        const float K2 = 0.211324865; // (3-sqrt(3))/6;
        vec2 i = floor(p + (p.x+p.y)*K1);
        vec2 a = p - i + (i.x+i.y)*K2;
        vec2 o = (a.x>a.y) ? vec2(1.0,0.0) : vec2(0.0,1.0); //vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y), sign(a.y-a.x));
        vec2 b = a - o + K2;
        vec2 c = a - 1.0 + 2.0*K2;
        vec3 h = max(0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
        vec3 n = h*h*h*h*vec3( dot(a,star_hash(i+0.0)), dot(b,star_hash(i+o)), dot(c,star_hash(i+1.0)));
        return dot(n, vec3(70.0));
    }

    #if defined(TEST_CLOUDS_2)
        // From drift on shadertoy - https://www.shadertoy.com/view/4tdSWr
        const float cloudscale = 1.1;
        const float speed = cloud_speed * 0.1;
        const float clouddark = 0.5;
        const float cloudlight = 0.3;

        #if defined(MORECLOUDS)
            const float cloudcover = 0.8;
        #elif defined(OURUIN_EVENING)
            const float cloudcover = 1.0;
        #else
            const float cloudcover = 0.2;
        #endif

        const float cloudalpha = 8.0;
        const float skytint = 0.5;

        #if defined(OURUIN_EVENING)
            const vec3 skycolour1 = vec3(0.1, 0.1, 0.1);
            const vec3 skycolour2 = vec3(0.1, 0.1, 0.1);

        #else
            const vec3 cloudtint = vec3(1.1, 1.1, 0.9);
            const vec3 skycolour1 = vec3(0.2, 0.4, 0.6);
            const vec3 skycolour2 = vec3(0.4, 0.7, 1.0);
        #endif

        const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );

        vec2 clouds_hash( vec2 p ) {
            p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
            return -1.0 + 2.0*fract(sin(p)*43758.5453123);
        }

        float clouds_noise( in vec2 p ) {
            const float K1 = 0.366025404; // (sqrt(3)-1)/2;
            const float K2 = 0.211324865; // (3-sqrt(3))/6;
            vec2 i = floor(p + (p.x+p.y)*K1);
            vec2 a = p - i + (i.x+i.y)*K2;
            vec2 o = (a.x>a.y) ? vec2(1.0,0.0) : vec2(0.0,1.0); //vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y), sign(a.y-a.x));
            vec2 b = a - o + K2;
            vec2 c = a - 1.0 + 2.0*K2;
            vec3 h = max(0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
            vec3 n = h*h*h*h*vec3( dot(a,clouds_hash(i+0.0)), dot(b,clouds_hash(i+o)), dot(c,clouds_hash(i+1.0)));
            return dot(n, vec3(70.0));
        }

        float clouds_fbm(vec2 n) {
            float total = 0.0, amplitude = 0.1;

            for (int i = 0; i < 7; i++) {
                total += clouds_noise(n) * amplitude;
                n = m * n;
                amplitude *= 0.4;
            }

            return total;
        }
    #endif

#endif  // ^ defined(SKY)

#if !defined(DEPTH_ONLY)
    void CalculateLightContribParticle(inout vec3 diffuse_color, vec3 world_vert, uint light_val) {
        // number of lights in current cluster
        uint light_count = (light_val >> COUNT_BITS) & COUNT_MASK;

        // index into cluster_lights
        uint first_light_index = light_val & INDEX_MASK;

        // light list data is immediately after cluster lookup data
        uint num_clusters = grid_size.x * grid_size.y * grid_size.z;
        first_light_index = first_light_index + uint(light_cluster_data_offset);

        // debug option, uncomment to visualize clusters
        //out_color = vec3(min(light_count, 63u) / 63.0);
        //out_color = vec3(g.z / grid_size.z);

        for (uint i = 0u; i < light_count; i++) {
            uint light_index = texelFetch(cluster_buffer, int(first_light_index + i)).x;

            PointLightData l = FetchPointLight(light_index);

            vec3 to_light = l.pos - world_vert;
            // TODO: inverse square falloff
            // TODO: real light equation
            float dist = length(to_light);
            float falloff = max(0.0, (1.0 / dist / dist) * (1.0 - dist / l.radius));

            falloff = min(0.5, falloff);
            diffuse_color += falloff * l.color * 0.5;
        }
    }
#endif // ^ !defined(DEPTH_ONLY)

#if !defined(DETAIL_OBJECT) && !defined(DEPTH_ONLY) && !defined(SKY) && !defined(GPU_PARTICLE_FIELD)
    vec3 LookupSphereReflectionPos(vec3 world_vert, vec3 spec_map_vec, int which) {
        //vec3 sphere_pos = world_vert - reflection_capture_pos[which];
        //sphere_pos /= reflection_capture_scale[which];
        vec3 sphere_pos = (reflection_capture_matrix_inverse[which] * vec4(world_vert, 1.0)).xyz;

        if(length(sphere_pos) > 1.0){
            return spec_map_vec;
        }

        // Ray trace reflection in sphere
        float test = (2 * dot(sphere_pos, spec_map_vec)) * (2 * dot(sphere_pos, spec_map_vec)) - 4 * (dot(sphere_pos, sphere_pos)-1.0) * dot(spec_map_vec, spec_map_vec);
        test = 0.5 * pow(test, 0.5);
        test = test - dot(spec_map_vec, sphere_pos);
        test = test / dot(spec_map_vec, spec_map_vec);
        return sphere_pos + spec_map_vec * test;

        /*
        // Brute force approach
        float t = 0.0;

        for(int i=0; i<100; ++i){
            t += 0.02;
            vec3 test_point = (sphere_pos + spec_map_vec * t);

            if(dot(test_point, test_point) >= 1.0){
                return sphere_pos + spec_map_vec * t;
            }
        }
        return spec_map_vec;*/
    }
#endif

const float water_speed = 0.03;

#if !defined(SKY) && !defined(GPU_PARTICLE_FIELD)
    float GetWaterHeight(vec2 pos, vec3 tint){
        float scale = 0.1 * tint[0];
        float height = 0.0;
        float uv_scale = tint[1];
        float scaled_water_speed = water_speed * uv_scale;

        #if defined(SWAMP)
            scaled_water_speed *= 0.4;
        #endif

        pos *= uv_scale;
        height = texture(tex0, pos  * 0.3 + normalize(vec2(0.0, 1.0))*time*scaled_water_speed).x;
        height += texture(tex0, pos * 0.7 + normalize(vec2(1.0, 0.0))*time*3.0*scaled_water_speed).x;
        height += texture(tex0, pos * 1.1 + normalize(vec2(-1.0, 0.0))*time*5.0*scaled_water_speed).x;
        height += texture(tex0, pos * 0.6 + normalize(vec2(-1.0, 1.0))*time*7.0*scaled_water_speed).x;
        height *= scale;

        //height += texture(tex0, pos * 0.3 + normalize(vec2(1.0, 0.0))*time*water_speed * pow(0.3, 0.5)).x * scale / 0.3;
        //height += texture(tex0, pos * 0.1 + normalize(vec2(-1.0, 0.0))*time*water_speed * pow(0.1, 0.5)).x * scale / 0.1;
        //height += texture(tex0, pos * 0.05 + normalize(vec2(-1.0, 1.0))*time*water_speed * pow(0.05, 0.5)).x * scale / 0.05;

        /*
        height += sin(pos.x * 11.0 + time * water_speed) * scale;
        height += sin(pos.y * 3.0 + time * water_speed) * scale * 2.0;
        height += sin(dot(pos, normalize(vec2(1,1))) * 7.0 + time * water_speed) * scale;
        height += sin(dot(pos, normalize(vec2(1,-1))) * 13.0 + time * water_speed) * scale * 0.9;
        height += sin(dot(pos, normalize(vec2(-1,-1))) * 29.0 + time * water_speed) * scale * 0.5;
        height += sin(dot(pos, normalize(vec2(-1,-0.1))) * 43.0 + time * water_speed) * scale * 0.4;
        height += sin(dot(pos, normalize(vec2(1,-0.1))) * 51.0 + time * water_speed) * scale * 0.4;*/

        return height;
    }
#endif

void ClampCoord(inout vec2 coord, float lod){
    float threshold = 1.0 / (256.0 / pow(2.0, lod+1.0));
    coord[0] = min(coord[0], 1.0 - threshold);
    coord[0] = max(coord[0], threshold);
    coord[1] = min(coord[1], 1.0 - threshold);
    coord[1] = max(coord[1], threshold);
}

vec2 LookupFauxCubemap(vec3 vec, float lod) {
    vec2 coord;

    if(vec.x > abs(vec.y) && vec.x > abs(vec.z)){
        vec3 hit_point = vec3(1.0, vec.y / vec.x, vec.z / vec.x);
        coord = vec2(hit_point.z, hit_point.y) * -0.5 + vec2(0.5);
        ClampCoord(coord, lod);
    }

    if(vec.z > abs(vec.y) && vec.z > abs(vec.x)){
        vec3 hit_point = vec3(1.0, vec.y / vec.z, vec.x / vec.z);
        coord = vec2(hit_point.z*-1.0, hit_point.y) * -0.5 + vec2(0.5);
        ClampCoord(coord, lod);
        coord += vec2(4.0, 0.0);
    }

    if(vec.x < -abs(vec.y) && vec.x < -abs(vec.z)){
        vec3 hit_point = vec3(1.0, vec.y / vec.x, vec.z / vec.x);
        coord = vec2(hit_point.z*-1.0, hit_point.y) * 0.5 + vec2(0.5);
        ClampCoord(coord, lod);
        coord += vec2(1.0, 0.0);
    }

    if(vec.z < -abs(vec.y) && vec.z < -abs(vec.x)){
        vec3 hit_point = vec3(1.0, vec.y / vec.z, vec.x / vec.z);
        coord = vec2(hit_point.z, hit_point.y) * 0.5 + vec2(0.5);
        ClampCoord(coord, lod);
        coord += vec2(5.0, 0.0);
    }

    if(vec.y < -abs(vec.z) && vec.y < -abs(vec.x)){
        vec3 hit_point = vec3(1.0, vec.z / vec.y, vec.x / vec.y);
        coord = vec2(-hit_point.z, hit_point.y) * 0.5 + vec2(0.5);
        ClampCoord(coord, lod);
        coord += vec2(3.0, 0.0);
    }

    if(vec.y > abs(vec.z) && vec.y > abs(vec.x)){
        vec3 hit_point = vec3(1.0, vec.z / vec.y, vec.x / vec.y);
        coord = vec2(hit_point.z, hit_point.y) * 0.5 + vec2(0.5);
        ClampCoord(coord, lod);
        coord += vec2(2.0, 0.0);
    }

    coord.x /= 6.0;
    return coord;
}

#if !defined(DEPTH_ONLY)
    #if defined(CAN_USE_3D_TEX) && !defined(DETAIL_OBJECT) && !defined(SKY) && !defined(GPU_PARTICLE_FIELD)
        bool Query3DTexture(inout vec3 ambient_color, vec3 pos, vec3 normal) {
            bool use_3d_tex = false;
            vec3 ambient_cube_color[6];

            for(int i=0; i<6; ++i){
                ambient_cube_color[i] = vec3(0.0);
            }

            for(int i=0; i<light_volume_num; ++i){
                //vec3 temp = (world_vert - reflection_capture_pos[i]) / reflection_capture_scale[i];
                vec3 temp = (light_volume_matrix_inverse[i] * vec4(pos, 1.0)).xyz;
                vec3 scale_vec = (light_volume_matrix[i] * vec4(1.0, 1.0, 1.0, 0.0)).xyz;
                float scale = dot(scale_vec, scale_vec);
                float val = dot(temp, temp);

                if(temp[0] <= 1.0 && temp[0] >= -1.0 &&
                        temp[1] <= 1.0 && temp[1] >= -1.0 &&
                        temp[2] <= 1.0 && temp[2] >= -1.0) {
                    vec3 tex_3d = temp * 0.5 + vec3(0.5);
                    vec4 test = texture(tex16, vec3((tex_3d[0] + 0)/ 6.0, tex_3d[1], tex_3d[2]));

                    if(test.a >= 1.0){
                        for(int j=1; j<6; ++j){
                            ambient_cube_color[j] = texture(tex16, vec3((tex_3d[0] + j)/ 6.0, tex_3d[1], tex_3d[2])).xyz;
                        }

                        ambient_cube_color[0] = test.xyz;
                        ambient_color = SampleAmbientCube(ambient_cube_color, normal);
                        use_3d_tex = true;
                    }

                    //out_color.xyz = world_vert * 0.01;
                }
            }

            return use_3d_tex;
        }
    #endif

    #if !defined(SKY) && !defined(GPU_PARTICLE_FIELD)
        vec3 GetAmbientColor(vec3 world_vert, vec3 ws_normal) {
            vec3 ambient_color = vec3(0.0);

            #if defined(CAN_USE_3D_TEX) && !defined(DETAIL_OBJECT)
                bool use_3d_tex = Query3DTexture(ambient_color, world_vert, ws_normal);
            #else
                bool use_3d_tex = false;
            #endif

            if(!use_3d_tex){
                bool use_amb_cube = false;
                vec3 ambient_cube_color[6];

                for(int i=0; i<6; ++i){
                    ambient_cube_color[i] = vec3(0.0);
                }

                #if defined(CAN_USE_LIGHT_PROBES)
                    uint guess = 0u;
                    int grid_coord[3];
                    bool in_grid = true;

                    for(int i=0; i<3; ++i){
                        if(world_vert[i] > grid_bounds_max[i] || world_vert[i] < grid_bounds_min[i]){
                            in_grid = false;
                            break;
                        }
                    }

                    if(in_grid){
                        grid_coord[0] = int((world_vert[0] - grid_bounds_min[0]) / (grid_bounds_max[0] - grid_bounds_min[0]) * float(subdivisions_x));
                        grid_coord[1] = int((world_vert[1] - grid_bounds_min[1]) / (grid_bounds_max[1] - grid_bounds_min[1]) * float(subdivisions_y));
                        grid_coord[2] = int((world_vert[2] - grid_bounds_min[2]) / (grid_bounds_max[2] - grid_bounds_min[2]) * float(subdivisions_z));
                        int cell_id = ((grid_coord[0] * subdivisions_y) + grid_coord[1])*subdivisions_z + grid_coord[2];
                        uvec4 data = texelFetch(ambient_grid_data, cell_id/4);
                        guess = data[cell_id%4];
                        use_amb_cube = GetAmbientCube(world_vert, num_tetrahedra, ambient_color_buffer, ambient_cube_color, guess);
                    }
                #endif

                if(!use_amb_cube){
                    ambient_color = LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0);
                } else {
                    ambient_color = SampleAmbientCube(ambient_cube_color, ws_normal);
                }
            }

            return ambient_color;
        }
    #endif

    #if !defined(DETAIL_OBJECT) && !defined(DEPTH_ONLY) && !defined(SKY) && !defined(GPU_PARTICLE_FIELD)
        vec3 LookUpReflectionShapes(sampler2DArray reflections_tex, vec3 world_vert, vec3 reflect_dir, float lod) {
            #if defined(NO_REFLECTION_CAPTURE)
                return vec3(0.0);
            #else
                vec3 reflection_color = vec3(0.0);
                float total = 0.0;

                for(int i=0; i<reflection_capture_num; ++i){
                    //vec3 temp = (world_vert - reflection_capture_pos[i]) / reflection_capture_scale[i];
                    vec3 temp = (reflection_capture_matrix_inverse[i] * vec4(world_vert, 1.0)).xyz;
                    vec3 scale_vec = (reflection_capture_matrix[i] * vec4(1.0, 1.0, 1.0, 0.0)).xyz;
                    float scale = dot(scale_vec, scale_vec);
                    float val = dot(temp, temp);

                    if(val < 1.0){
                        vec3 lookup = LookupSphereReflectionPos(world_vert, reflect_dir, i);
                        vec2 coord = LookupFauxCubemap(lookup, lod);
                        float weight = pow((1.0 - val), 8.0);
                        weight *= 100000.0;
                        weight /= pow(scale, 2.0);
                        reflection_color.xyz += textureLod(reflections_tex, vec3(coord, i+1), lod).xyz * weight;
                        total += weight;
                    }
                }

                if(total < 0.0000001){
                    float weight = 0.00000001;
                    vec2 coord = LookupFauxCubemap(reflect_dir, lod);
                    reflection_color.xyz += textureLod(reflections_tex, vec3(coord, 0), lod).xyz * weight;
                    total += weight;
                }

                if(total > 0.0){
                    reflection_color.xyz /= total;
                }

                return reflection_color;
            #endif
        }
    #endif
#endif // ^ !defined(DEPTH_ONLY)

// From http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );

    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

//#define CAN_USE_3D_TEX
//#define TEXEL_DENSITY_VIZ
//#define ALBEDO_ONLY
//#define NO_DECALS
//#define NO_DETAILMAPS

bool sphere_collision(vec3 s, vec3 c, vec3 d, float r, out vec3 intersection, out vec3 normal){
    // Calculate ray start's offset from the sphere center
    vec3 p = s - c;

    float rSquared = r * r;
    float p_d = dot(p, d);

    // The sphere is behind or surrounding the start point.
    if(p_d > 0 || dot(p, p) < rSquared) {
        return false;
    }

    // Flatten p into the plane passing through c perpendicular to the ray.
    // This gives the closest approach of the ray to the center.
    vec3 a = p - p_d * d;

    float aSquared = dot(a, a);

    // Closest approach is outside the sphere.
    if(aSquared > rSquared) {
        return false;
    }

    // Calculate distance from plane where ray enters/exits the sphere.
    float h = sqrt(rSquared - aSquared);

    // Calculate intersection point relative to sphere center.
    vec3 i = a - h * d;

    intersection = c + i;
    normal = i/r;
    // We've taken a shortcut here to avoid a second square root.
    // Note numerical errors can make the normal have length slightly different from 1.
    // If you need higher precision, you may need to perform a conventional normalization.

    return true;
}

#if !defined(GPU_PARTICLE_FIELD)
    vec3 GetFogColorMult(){
        #if defined(RAINY) || defined(WET)
            #if defined(NO_SKY_HIGHLIGHT)
                return vec3(1.0);
            #else
                return vec3(1.0) + primary_light_color.xyz * pow((dot(normalize(world_vert-cam_pos), ws_light) + 1.0)*0.5, 4.0);
            #endif
        #elif defined(VOLCANO)
            return mix(vec3(1.0), vec3(0.2, 1.6, 3.6), max(0.0, normalize(world_vert-cam_pos).y));
        #elif defined(SWAMP) || (defined(WATERFALL_ARENA) && !defined(CAVE))
            // Rainbow should be from 40.89–42 degrees, with red on the outside
            // Double rainbow should be at angles 50-53, with blue on the outside
            vec3 dir = normalize(world_vert-cam_pos);
            float dot = dot(dir, ws_light);
            float angle = 180 - acos(dot)*180.0/3.1417;
            vec3 rainbow = vec3(0.0);

            if(angle > 39 && angle < 53 && dir.y > 0.0){
                rainbow[0] = max(0.0, 1.0-abs(angle-42));
                rainbow[1] = max(0.0, 1.0-abs(angle-41.5));
                rainbow[2] = max(0.0, 1.0-abs(angle-40.89));

                rainbow[0] += max(0.0, 1.0-abs(angle-50.8)*0.8)*0.25;
                rainbow[1] += max(0.0, 1.0-abs(angle-51.5)*0.8)*0.25;
                rainbow[2] += max(0.0, 1.0-abs(angle-52.2)*0.8)*0.25;

                rainbow *= min(1.0, distance(world_vert, cam_pos) * 0.02);

                #if defined(SWAMP)
                    rainbow *= 1.0/(cam_pos.y-kBeachLevel+1.0);
                #endif

                rainbow *=  max(0.0, dir.y) * 1.5;
            }

            #if defined(WATERFALL_ARENA)
                rainbow *= 0.5;
            #endif

            vec3 col = vec3(mix(dot + 2.0, 1.0, 0.5)) + rainbow;

            #if defined(WATERFALL_ARENA) && !defined(CAVE)
                col *= 0.15;
            #endif

            return col;
        #elif defined(WATER_HORIZON)
            return vec3(dot(normalize(world_vert-cam_pos), ws_light) + 2.0)*0.5;
        #elif defined(SWAMP)
            return vec3(dot(normalize(world_vert-cam_pos), ws_light) + 2.0);
        #elif defined(SNOW_EVERYWHERE2)
            vec3 normal = normalize(world_vert-cam_pos);
            float gradient = pow(0.5+dot(normal, ws_light)*0.5,3.0);
            vec3 color = mix(vec3(0.1,0.2,0.5), vec3(2.0,1.5,0.7)*1.3, gradient) * 0.25;
            float opac = min(1.0, max(0.0, 1.0 - normal.y));
            color = mix(vec3(0.1,0.1,0.4), color, opac);
            return color;
        #else
            return vec3(1.0);
        #endif
    }
#endif

#if defined(VOLUME_SHADOWS)
    float VolumeShadow(vec3 pos) {
        float amount = 0.0;
        int num_samples = 5;
        float random = rand(gl_FragCoord.xy);
        float total = 0.0;

        for(int i=0; i<num_samples; ++i){
            vec3 sample_vert = mix(pos, cam_pos, (i+random)/float(num_samples));
            vec3 ws_vertex = sample_vert - cam_pos;
            vec4 shadow_coords[4];
            shadow_coords[0] = shadow_matrix[0] * vec4(sample_vert, 1.0);
            shadow_coords[1] = shadow_matrix[1] * vec4(sample_vert, 1.0);
            shadow_coords[2] = shadow_matrix[2] * vec4(sample_vert, 1.0);
            shadow_coords[3] = shadow_matrix[3] * vec4(sample_vert, 1.0);
            float len = length(ws_vertex);
            float weight = 1.0;
            amount += GetCascadeShadow(shadow_sampler, shadow_coords, len) * weight;
            total += weight;
        }

        amount /= total;
        return amount;
    }
#endif

//#define SPHERE_TEST

void Caustics(float kBeachLevel, vec3 ws_normal, inout vec3 diffuse_color){
    if(world_vert.y < kBeachLevel + 2.0){ // caustics
        float mult = (kBeachLevel + 2.0 - world_vert.y) / 3.0;

        #if defined(WATERFALL_ARENA)
            mult = mult*0.3+0.3;
        #endif

        #if defined(BEACH)
            float fade_x = -71 + sin(time*0.5)*-1.5;

            if(world_vert.x < fade_x){
                mult *= max(0.0, -fade_x + 1 + world_vert.x);
            }
        #endif

        #if defined(SKY_ARK)
            vec3 pos = vec3(-97.96, 68.4, 101.8);
            mult *= max(0.0,min(1.0, 26.0 - distance(world_vert, pos)));
        #endif

        mult = mult * mult;
        mult = min(1.0, mult);
        vec3 temp = world_vert * 0.2;
        float fade = 0.4;// max(0.0, (0.5 - length(temp))*8.0)* max(0.0, fractal(temp.xz*7.0)+0.3);
        float speed = 0.2;
        float fire = abs(fractal(temp.xz*11.0+time*3.0*speed)+fractal(temp.xy*7.0-time*3.0*speed)+fractal(temp.yz*5.0-time*3.0*speed));
        float flame_amount = max(0.0, 0.5 - (fire*0.5 / pow(fade, 2.0))) * 2.0;
        flame_amount += pow(max(0.0, 0.7-fire), 2.0);

        #if defined(WATER_HORIZON)
            mult *= max(0.0, 0.2-ws_normal.y) * 2.0;
        #elif defined(BEACH)
            mult *= max(0.0, 0.2-ws_normal.y) * 3.0;
        #elif defined(SNOW_EVERYWHERE)
            mult *= min(1.0, max(0.0, 1.0-ws_normal.y)) * 1.0;
        #elif defined(SKY_ARK)
            mult *= min(1.0, max(0.0, 1.0-ws_normal.y)) * 0.5;
        #elif defined(WATERFALL_ARENA)
            mult *= min(1.0, max(0.0, 1.0-ws_normal.y)) * 0.5;
        #endif

        diffuse_color.xyz *= 1.0 + flame_amount * primary_light_color.xyz  * mult;
    }
}

#if defined(SSAO_TEST) && !defined(SKY) && !defined(PARTICLE) && !defined(GPU_PARTICLE_FIELD) && !defined(DEPTH_ONLY)
    float SSAO(vec3 ws_vertex) {
        vec4 proj_test_point = (projection_view_mat * vec4(world_vert, 1.0));
        proj_test_point /= proj_test_point.w;
        proj_test_point.xy += vec2(1.0);
        proj_test_point.xy *= 0.5;
        vec2 uv = proj_test_point.xy;
        float my_depth = LinearizeDepth(gl_FragCoord.z);
        float temp = 0.0;
        float z_threshold = 0.3 * length(ws_vertex);
        float total;
        int num_samples = 32;

        for(int i=0; i<num_samples; ++i){
            float angle = noise(gl_FragCoord.xy) + 6.28318530718 * i / float(num_samples);
            mat2 rot;
            rot[0][0] = cos(angle);
            rot[0][1] = sin(angle);
            rot[1][0] = -sin(angle);
            rot[1][1] = cos(angle);
            vec2 offset;
            float mult = 0.1;
            vec2 dims = (viewport.zw - viewport.xy);
            float aspect = 16.0/9.0;// dims[0]/dims[1];
            offset = rot * vec2(mult, 0.0);
            offset[1] *= aspect;
            float radius = pow(noise(gl_FragCoord.xy*1.3+vec2(i)), 1.0);
            vec2 sample_pos = uv + offset * radius;

            if(sample_pos[0] > 0.0 && sample_pos[0] < 1.0 && sample_pos[1] > 0.0 && sample_pos[1] < 1.0) {
                float depth = LinearizeDepth(textureLod(tex18, sample_pos, 0.0).r);
                float fade = (depth - (my_depth - z_threshold)) / z_threshold;

                if(fade > -0.99 && fade < 0.99){
                    temp += 1.0;//min(1.0, max(0.0, fade));
                }

                total += 1.0;
            }
        }

        float ssao_amplify = 2.0;
        float ssao_amount = (1.0 - temp / total + 0.2) * ssao_amplify - ssao_amplify * 0.5;
        return ssao_amount;

        if(false) {
            out_color.xyz = vec3(ssao_amount) * 0.1;
            out_color.a = 1.0;
            return 0.0;
        }
    }
#endif

float CloudShadow(vec3 pos){
    #if defined(TEST_CLOUDS_2) && !defined(DEPTH_ONLY) && !defined(CLOUDS_DO_NOT_CAST_SHADOWS)
        return max(0.0, fractal(pos.zx*0.05+vec2(0.0,time*cloud_speed))*2.0+1.0);
    #else
        return 1.0;
    #endif
}


vec3 quat_mul_vec3(vec4 q, vec3 v) {
    // Adapted from https://github.com/g-truc/glm/blob/master/glm/detail/type_quat.inl
    // Also from Fabien Giesen, according to - https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
    vec3 quat_vector = q.xyz;
    vec3 uv = cross(quat_vector, v);
    vec3 uuv = cross(quat_vector, uv);
    return v + ((uv * q.w) + uuv) * 2;
}


void main() {
    #if defined(INVISIBLE) && !defined(COLLISION)
        discard;
    #endif

    /*{
    vec3 world_dx = dFdx(world_vert);
    vec3 world_dy = dFdy(world_vert);
    vec3 ws_normal = normalize(cross(world_dx, world_dy));
    out_color.xyz = ws_normal;
    //out_color.xyz = vec3(gl_PrimitiveID%256/255.0);
    out_color.a = 1.0;
    return;
    }*/

    #if defined(SPHERE_TEST)
        #if !defined(DEPTH_ONLY)
            {
                vec3 normal, intersection;
                vec3 sphere_pos = vec3(0.0);
                float sphere_radius = 1.0;

                if(sphere_collision(cam_pos, sphere_pos, normalize(world_vert-cam_pos), sphere_radius, intersection, normal)){
                    if(distance(cam_pos, intersection) < distance(cam_pos, world_vert)){
                        out_color = vec4((normal*0.5+vec3(0.5)), 1.0);

                        return;
                    }
                } else if(distance(cam_pos, sphere_pos) < sphere_radius){
                    vec3 normal = normalize(cam_pos - sphere_pos);
                    out_color = vec4((normal*0.5+vec3(0.5)), 1.0);

                    return;
                }
            }
        #endif
    #endif

    // Switch for GPU_PARTICLE_FIELD, SKY, or in-scene objects
    // Non-GPU particle field is handled in the in-scene objects section

    #if defined(GPU_PARTICLE_FIELD)

        #if defined(GPU_PARTICLE_FIELD_OCCLUSION)
            #if !defined(DEPTH_ONLY)
            {
                #if !defined(NO_DECALS)
                    vec4 ndcPos;
                    ndcPos.xy = ((2.0 * gl_FragCoord.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
                    ndcPos.z = 2.0 * gl_FragCoord.z - 1; // this assumes gl_DepthRange is not changed
                    ndcPos.w = 1.0;

                    vec4 clipPos = ndcPos / gl_FragCoord.w;
                    vec4 eyePos = inv_proj_mat * clipPos;

                    float zVal = ZCLUSTERFUNC(eyePos.z);

                    zVal = max(0u, min(zVal, grid_size.z - 1u));

                    uvec3 g = uvec3(uvec2(gl_FragCoord.xy) / cluster_width, zVal);
                    uint decal_cluster_index = NUM_GRID_COMPONENTS * ((g.y * grid_size.x + g.x) * grid_size.z + g.z);
                    uint decal_val = texelFetch(cluster_buffer, int(decal_cluster_index)).x;

                    // number of decals in current cluster
                    uint decal_count = (decal_val >> COUNT_BITS) & COUNT_MASK;

                    // index into cluster_decals
                    uint first_decal_index = decal_val & INDEX_MASK;

                    // decal list data is immediately after cluster lookup data
                    uint num_clusters = grid_size.x * grid_size.y * grid_size.z;
                    first_decal_index = first_decal_index + 2u * num_clusters;

                    for (uint i = 0u; i < decal_count; ++i) {
                        // texelFetch takes int
                        uint decal_index = texelFetch(cluster_buffer, int(first_decal_index + i)).x;

                        DecalData decal;
                        vec4 decal_temp  = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 0u));
                        decal.scale      = decal_temp.xyz;
                        decal.rotation   = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 1u));
                        decal.position   = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 2u)).xyz;

                        decal.tint = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 3u));

                        // We omit scale component because we want to keep normal unit length
                        // We omit translation component because normal
                        mat3 rotation_mat = mat_from_quat(decal.rotation);

                        vec3 inv_scale = vec3(1.0f) / decal.scale;
                        mat3 inv_rotation_mat = transpose(rotation_mat);
                        vec3 temp = (inv_rotation_mat * (world_vert - decal.position)) * inv_scale;

                        if(abs(temp[0]) < 0.5 && abs(temp[1]) < 0.5 && abs(temp[2]) < 0.5) {
                            int type = int(decal.tint[3]);

                            if(type == decalUnderwater) {
                                discard;
                            }
                        }
                    }
                #endif
            }
            #endif
        #endif

        //#define ASH
        //#define RAIN
        #if defined(ASH) || defined(SANDSTORM) || (defined(SNOW) && defined(MED) || defined(EMBERS))
            int type = instance_id%83;
        #endif

        float disp_alpha = max(0.0, 0.5-distance(tex_coord, vec2(0.5)))*2.0;
        float fade_dist = 2.0;
        out_color = vec4(1.0,1.0,1.0,alpha);
        float dist = distance(world_vert, cam_pos);

        #if defined(ASH)
            if(type >= 1 && type < 3){
                out_color.xyz = vec3(0.0);
                out_color.a = min(1.0, out_color.a);
            } else if (type == 0){
                out_color.xyz = vec3(2.0,0.0,0.0);

                if(out_color.a > 1.0){
                    out_color.xyz *= out_color.a;
                    out_color.a = 1.0;
                }
            } else {
                out_color.xyz = vec3(0.0,0.0,0.0);
                #if !defined(ASH_DISABLE_VOLCANO_LIGHTING)
                    float height = world_vert.y + 22;
                    float volcano_lit = pow(max(0.0, (1.0-height*0.1)), 4.0);
                    out_color.xyz += vec3(0.5,0.1,0.0)*volcano_lit;
                #endif
                fade_dist = 10.0;
                #if defined(ASH_DARKER_INDOOR_SMOKE)
                    out_color.a += 0.25;
                    fade_dist = 2.0;
                #endif
                #if defined(ASH_EVEN_DARKER_INDOOR_SMOKE)
                    out_color.a += 0.125;
                    fade_dist = 10.0;
                #endif
            }
        #elif defined(EMBERS)
            if(type >= 1 && type < 3){
                out_color.xyz = vec3(0.1);
                out_color.a = min(1.0, out_color.a);
            } else if (type == 0){
                float height = world_vert.y - cam_pos.y - 0.0;
                out_color.xyz = mix(vec4(1.0, 0.5, 0.0, 1.0) * 50.0, vec4(2.0), clamp(height * 1.0, 0.0, 1.0)).xyz;
                out_color.a = 1.0 - (min(4.0, height) / 4.0);

                if(out_color.a > 1.0){
                    out_color.xyz *= out_color.a;
                    out_color.a = 1.0;
                }

            } else {
                out_color.xyz = vec3(0.0,0.0,0.0);
                out_color.a = 0.005;
            }
        #elif defined(SANDSTORM)
            if(type < 3){
                out_color.xyz = vec3(0.0);
                out_color.a = min(1.0, out_color.a);
            } else {
                out_color.xyz = vec3(0.2,0.15,0.1);
                fade_dist = 10.0;
            }
        #elif defined(RAIN)
            out_color.xyz *= (vec3(0.5) + primary_light_color.xyz) * 0.5 * mix(0.5,1.0,(instance_id%83)/83.0);
        #elif defined(FIREFLY)
            float lit = max(0.0, sin(time*(0.5+instance_id%89*0.01) + instance_id)*40.0-39);
            out_color.xyz = mix(vec3(0.0), vec3(2.0, 1.0, 0.0), lit);
        #else
            #if defined(SNOW) && defined(MED)
                if(type == 10) {
                    out_color.a = mix(out_color.a, 0.0, min(1.0, (dist-10)/6.0));
                } else {
            #else
                if(true) {
            #endif

                vec4 shadow_coords[4];
                shadow_coords[0] = shadow_matrix[0] * vec4(world_vert, 1.0);
                shadow_coords[1] = shadow_matrix[1] * vec4(world_vert, 1.0);
                shadow_coords[2] = shadow_matrix[2] * vec4(world_vert, 1.0);
                shadow_coords[3] = shadow_matrix[3] * vec4(world_vert, 1.0);
                float shadow = GetCascadeShadow(shadow_sampler, shadow_coords, distance(cam_pos,world_vert));
                shadow *= CloudShadow(world_vert);
                out_color.xyz *= mix(0.2,1.0,shadow);
            }

            #if defined(BUGS)
                out_color.xyz *= mix(0.05,0.2,(instance_id%83)/83.0);
            #endif
        #endif

        if(dist < fade_dist){
            disp_alpha *= dist/fade_dist;
        }

        #if defined(RAIN)
            vec3 up = vec3(0,1,0);
            vec3 dir = (world_vert.xyz - cam_pos) / dist;
            vec3 right = cross(dir, up);
            up = cross(dir, right);
            #if !defined(GPU_PARTICLE_FIELD_SIMPLE)
                out_color.xyz = LookupCubemapSimpleLod(normalize(world_vert.xyz - cam_pos)+right*(tex_coord[0]-0.5)*2.0+up*(tex_coord[1]-0.5)*2.0, tex3, 0.0).xyz;
            #endif
            out_color.xyz *= 1.0 + primary_light_color.x * 2.0 * max(0.0, (dot(dir, ws_light)*0.5+0.5))*max(0.0, dir.y+0.1);
            #if defined(GPU_PARTICLE_FIELD_SIMPLE)
                out_color.a *= 0.1;
            #endif
        #endif

        //out_color.xyz = vec3(tex_coord[0]-0.5);
        //out_color.a = 1.0;
        out_color.a *= disp_alpha;

        return;
    #elif defined(SKY)
        vec3 color;
        vec3 normal = normalize(world_vert - cam_pos);

        #if defined(YCOCG_SRGB)
            color = YCOCGtoRGB(texture(tex0,normal));
            color = pow(color,vec3(2.2));
        #else
            color = texture(tex0,normal).xyz;
        #endif

        float foggy = max(0.0, min(1.0, (fog_amount - 1.0) / 2.0));

        #if defined(WATER_HORIZON)
            foggy = 0.0;
        #endif

        float fogness = mix(-1.0, 1.0, foggy);

        if(normal.y < 0.0){
            fogness = mix(fogness, 1.0, -normal.y * fog_amount / 5.0);
        }

        #if !defined(MISTY) && !defined(MISTY2) && !defined(SKY_ARK) && !defined(WATERFALL_ARENA)
            float blur = max(0.0, min(1.0, (1.0-abs(normal.y)+fogness)));
            color = mix(color, textureLod(tex1,normal, mix(pow(blur, 2.0), 1.0, fogness*0.5+0.5) * 5.0).xyz, min(1.0, blur * 4.0));
        #endif

        #if !defined(OURUIN_EVENING)
            color.xyz *= tint;
        #endif
        //vec3 tint = vec3(1.0, 0.0, 0.0);
        //color *= tint;
        out_color = vec4(color,1.0);

        #if defined(ADD_STARS)
            float star = star_noise(normal.xz/(normal.y+2.0) * 200.0+ vec2(time*0.05,0.0));
            star = max(0.0, star - 0.7);
            star = star * 100.0;
            star *= max(0.0, 1.0 - abs(star_noise(normal.xz/(normal.y+2.0) * 20.0 + vec2(time,0.0)))*1.0);

            if(out_color.b < 0.1){
                out_color.xyz += vec3(star)*0.01;
            }
        #endif

        #if defined(ADD_MOON)
            if(dot(normal, ws_light) > 0.9999){
                out_color.xyz += vec3(1.0) * 2.0;
            } else if(dot(normal, ws_light) > 0.999){
                out_color.xyz += vec3(pow((dot(normal, ws_light) - 0.9989) * 1000, 40.0))*2.0;
            }
        #endif

        #if defined(VOLCANO)
            float volcano_amount = min(1.0, max(0.0, 1.0 - normal.y));
            out_color.xyz += (textureLod(tex2,normal,5.0).xyz - out_color.xyz) * volcano_amount;
        #endif

        #if defined(MISTY) || defined(MISTY2) || defined(SKY_ARK) || defined(CAVE) || defined(RAINY) || defined(WET)
            float haze_amount = GetHazeAmount(normal * 1000.0, haze_mult);
            vec3 fog_color;
            fog_color = textureLod(tex2, normal, 3.0).xyz * GetFogColorMult();
            out_color.xyz = mix(out_color.xyz, fog_color, haze_amount);
        #endif

        #if defined(SKY_ARK)
            if(normal.y < -0.02){
                float val = 1.0 - max(0.0, min(1.0, (normal.y+0.04) / 0.02));
                out_color.xyz = mix(out_color.xyz, textureLod(tex2, normal, 0.0).xyz * vec3(1.2,1.1,1.0), val);
            }
        #endif

        #if defined(WATER_HORIZON)
            float haze_amount = 1.0;
            vec3 fog_color;
            float opac = min(1.0, pow(max(0.0,1.0-normal.y), 8.0));
            fog_color = textureLod(tex2, normal, 3.0).xyz * GetFogColorMult();

            #if defined(ALT2)
                fog_color = vec3(1.0);
            #endif

            out_color.xyz = mix(out_color.xyz, fog_color, opac);
        #endif

        #if defined(SNOW_EVERYWHERE2)
            out_color.xyz = GetFogColorMult();
        #endif

        #if defined(VOLUME_SHADOWS)
            out_color.xyz *= mix(VolumeShadow(cam_pos+normal*150.0), 1.0, 0.3);
        #endif

        #if defined(TEST_CLOUDS_2)
            vec3 temp = normalize(normal);
            vec3 normalized = temp;//vec3(temp[2], temp[1], -temp[0]);

            #if defined(CLOUDS_BELOW_HORIZON)
                vec3 plane_intersect = normalized / -(normalized.y-0.2);
            #else
                vec3 plane_intersect = normalized / (normalized.y+0.2);
            #endif

            vec2 old_uv = plane_intersect.xz;
            vec2 uv = old_uv;
            float temp_time = time * speed;

            #if defined(CLOUDS_VORTEX)
                float swirl_radius = 128.0 / 32.0;
                float swirl_angle = 32.0 / 32.0;
                float swirl_distance = length(uv);
                float swirl_time_scale = 2.0;

                if(swirl_distance < swirl_radius) {
                    float swirl_ratio = (swirl_radius - swirl_distance) / swirl_radius;
                    temp_time *= swirl_ratio * swirl_time_scale;
                    float swirl_theta = pow(swirl_ratio, 16.0) * swirl_angle * 8.0;
                    float swirl_sin_theta = sin(swirl_theta);
                    float swirl_cos_theta = cos(swirl_theta);
                    uv = old_uv = vec2(dot(uv, vec2(swirl_cos_theta, -swirl_sin_theta)), dot(uv, vec2(swirl_sin_theta, swirl_cos_theta)));
                }
            #endif

            float q = clouds_fbm(uv * cloudscale * 0.5);

            //ridged noise shape
            float r = 0.0;
            uv *= cloudscale;
            uv -= q - temp_time;
            float weight = 0.8;

            for (int i=0; i<8; i++){
                r += abs(weight*noise( uv ));
                uv = m*uv + temp_time;
                weight *= 0.7;
            }

            //noise shape
            float f = 0.0;
            uv = old_uv;
            uv *= cloudscale;
            uv -= q - temp_time;
            weight = 0.7;

            for (int i=0; i<8; i++){
                f += weight*noise( uv );
                uv = m*uv + temp_time;
                weight *= 0.6;
            }

            f *= r + f;

            //noise colour
            float c = 0.0;
            temp_time = time * speed * 2.0;

            #if defined(CLOUDS_VORTEX)
                if(swirl_distance < swirl_radius) {
                    float swirl_ratio = (swirl_radius - swirl_distance) / swirl_radius;
                    temp_time *= swirl_ratio * swirl_time_scale;
                }
            #endif

            uv = old_uv;
            uv *= cloudscale*2.0;
            uv -= q - temp_time;
            weight = 0.4;

            for (int i=0; i<7; i++){
                c += weight*noise( uv );
                uv = m*uv + temp_time;
                weight *= 0.6;
            }

            //noise ridge colour
            float c1 = 0.0;
            temp_time = time * speed * 3.0;

            #if defined(CLOUDS_VORTEX)
                if(swirl_distance < swirl_radius) {
                    float swirl_ratio = (swirl_radius - swirl_distance) / swirl_radius;
                    temp_time *= swirl_ratio * swirl_time_scale;
                }
            #endif

            uv = old_uv;
            uv *= cloudscale*3.0;
            uv -= q - temp_time;
            weight = 0.4;

            for (int i=0; i<7; i++){
                c1 += abs(weight*noise( uv ));
                uv = m*uv + temp_time;
                weight *= 0.6;
            }

            c += c1;

            #if defined(CLOUDS_DO_NOT_TINT_SKY)
                vec3 skycolour = color;
            #else
                vec3 skycolour = mix(skycolour2, skycolour1, normalized.y);
            #endif

            #if defined(OURUIN_EVENING)
                // Apply the tint to the skybox texture as well.
                skycolour *= tint;
                // The sun direction is used to create a light and a dark side.
                float sun_direction = dot(ws_light, normalized);
                // Make the clouds darker on the other side of the sky.
                float sun_colour  = max(0.0, -sun_direction);
                // Remove colour from the clouds when reaching the dark side.
                vec3 cloud_coloured = mix(tint, vec3(0.0), sun_colour);
                vec3 cloudcolour = cloud_coloured * clamp((clouddark + cloudlight*c), 0.0, 1.0);
            #else
                vec3 cloudcolour = cloudtint * clamp((clouddark + cloudlight*c), 0.0, 1.0);
            #endif

            f = cloudcover + cloudalpha*f*r;

            #if defined(CLOUDS_ALPHA)
                vec3 result = mix(skycolour, mix(skycolour, cloudcolour, 0.5), clamp(f + c, 0.0, 1.0));
            #else
                vec3 result = mix(skycolour, clamp(skytint * skycolour + cloudcolour, 0.0, 1.0), clamp(f + c, 0.0, 1.0));
            #endif

            #if defined(CLOUDS_BELOW_HORIZON)
                float mix_amount = max(-normalized.y, 0.0);
            #else
                float mix_amount = max(normalized.y, 0.0);
            #endif

            #if defined(OURUIN_EVENING)
                float direct_sun_light = max(0.0, (sun_direction + 1.0) / 2.0) * 1.0;
                // Apply an exponential tween to the sun direction so the brightness ramps up.
                direct_sun_light = direct_sun_light == 0.0 ? 0.0 : pow(2.0, 10.0 * direct_sun_light - 10.0);
                // Add a lot of sky brightness when 
                direct_sun_light *= 10.0;
                // Make sure the added brightness does not go negative.
                direct_sun_light = max(0.5, direct_sun_light);
                out_color = mix(out_color, vec4( result * direct_sun_light, 1.0 ), mix_amount);  // Clouds
            #else
                out_color = mix(out_color, vec4( result * 0.5, 1.0 ), mix_amount);  // Clouds
            #endif
            // out_color = vec4(vec3((old_uv.x > 0.0) == (old_uv.y > 0.0)), 1.0);  // Quadrants black/white
            // out_color = vec4(vec3((int(floor(old_uv.x)) % 2 == 0) == (int(floor(old_uv.y)) % 2 == 0)), 1.0);  // Checkerboard
        #endif

        #if defined(WATERFALL_ARENA) && !defined(CAVE)
            out_color.xyz = textureLod(tex2,normal,3.0).xyz * GetFogColorMult();
        #endif

    #else  // ^ defined(SKY)
        // Any other in-scene object type from now on:

        #if defined(TERRAIN) && !defined(TERRAIN_NO_EDGE_FADE)
            float terrain_edge_fade = 0.0;
            float start = 450;
            float fade_dist = 50;

            if(abs(world_vert.x) > start || abs(world_vert.z) > start) {
                #if defined(DEPTH_ONLY)
                    discard;
                #endif

                terrain_edge_fade = min(1.0, (max(abs(world_vert.x), abs(world_vert.z)) - start) / fade_dist);
            }
        #endif

        #if defined(WIREFRAME)
            out_color = vec4(0.0,0.0,0.0,1.0);
            gl_FragDepth = gl_FragCoord.z - 0.001 * gl_FragCoord.w;

            return;
        #endif

        #if defined(HALFTONE_STIPPLE)
            if(int(gl_FragCoord.x) % 3 != 0 || int(gl_FragCoord.y) % 3 != 0){
                discard;
            }
        #endif

        vec3 ws_vertex = world_vert - cam_pos;

        vec4 ndcPos;
        ndcPos.xy = ((2.0 * gl_FragCoord.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
        ndcPos.z = 2.0 * gl_FragCoord.z - 1; // this assumes gl_DepthRange is not changed
        ndcPos.w = 1.0;

        vec4 clipPos = ndcPos / gl_FragCoord.w;
        vec4 eyePos = inv_proj_mat * clipPos;

        float zVal = ZCLUSTERFUNC(eyePos.z);

        zVal = max(0u, min(zVal, grid_size.z - 1u));

        uvec3 g = uvec3(uvec2(gl_FragCoord.xy) / cluster_width, zVal);

        out_color = vec4(0.0);

        // decal/light cluster stuff
        #if !(defined(NO_DECALS) || defined(DEPTH_ONLY))
            uint decal_cluster_index = NUM_GRID_COMPONENTS * ((g.y * grid_size.x + g.x) * grid_size.z + g.z);
            uint decal_val = texelFetch(cluster_buffer, int(decal_cluster_index)).x;
            uint decal_count = (decal_val >> COUNT_BITS) & COUNT_MASK;

            /*out_color.xyz = vec3(decal_count * 0.1);
            out_color.a = 1.0;

            return;*/
        #endif  // NO_DECALS

        uint light_cluster_index = NUM_GRID_COMPONENTS * ((g.y * grid_size.x + g.x) * grid_size.z + g.z) + 1u;

        #if defined(DEPTH_ONLY)
            uint light_val = 0U;
        #else
            uint light_val = texelFetch(cluster_buffer, int(light_cluster_index)).x;
        #endif //DEPTH_ONLY

        #if defined(DETAIL_OBJECT)
            float dist_fade = 1.0 - length(ws_vertex)/max_distance;
            dist_fade = min(1.0, max(0.0, dist_fade));
            dist_fade = 1.0 - pow(1.0 - dist_fade, 5.0 - tint_weight * 4.0);// 2.0 - tint_weight);

            CALC_COLOR_MAP

            #if defined(PLANT)
                colormap.a = pow(colormap.a, max(0.1,min(1.0,3.0/length(ws_vertex))));

                #if !defined(TERRAIN)
                    colormap.a -= max(0.0, -1.0 + (length(ws_vertex)/max_distance * (1.0+rand(gl_FragCoord.xy)*0.5))*2.0);
                #endif

                #if !defined(ALPHA_TO_COVERAGE)
                    if(colormap.a < 0.5){
                        discard;
                    }
                #else
                    colormap.a = 0.5 + (colormap.a - 0.5) * 2.0;

                    if(colormap.a < 0.0){
                        discard;
                    }
                #endif

                #if defined(DEPTH_ONLY)
                    out_color = vec4(vec3(1.0), colormap.a);

                    return;
                #endif
            #endif // PLANT

            #if defined(DEPTH_ONLY)
                out_color = vec4(1.0);

                return;
            #else  // DEPTH_ONLY
                vec4 normalmap = texture(normal_tex,tc0);
                vec3 normal = UnpackTanNormal(normalmap);
                vec3 ws_normal = tangent_to_world * normal;

                vec3 base_normalmap = texture(base_normal_tex,tc1).xyz;

                #if defined(TERRAIN)
                    vec3 base_normal = normalize((base_normalmap*vec3(2.0))-vec3(1.0));
                #else  // TERRAIN
                    //I'm assuming this normal is supposed to be in world space --Max
                    vec3 base_normal = normalize(normal_matrix * UnpackObjNormalV3(base_normalmap.xyz));
                #endif  // TERRAIN

                ws_normal = mix(ws_normal,base_normal,min(1.0,1.0-(dist_fade-0.5)));

                float preserve_wetness = 1.0;
                float ambient_mult = 1.0;
                float env_ambient_mult = 1.0;
                float roughness = 1.0;

                vec3 decal_diffuse_color = vec3(0.0);

                #if !defined(NO_DECALS)
                    float spec_amount = 0.0;
                    vec3 flame_final_color = vec3(0.0, 0.0, 0.0);;
                    float flame_final_contrib = 0.0;
                    CalculateDecals(colormap, ws_normal, spec_amount, roughness, preserve_wetness, ambient_mult, env_ambient_mult, decal_diffuse_color, world_vert, time, decal_val, flame_final_color, flame_final_contrib);
                #endif

                #if defined(SSAO_TEST)
                    env_ambient_mult *= SSAO(ws_vertex);
                #endif

                #define shadow_tex_coords tc1

                #if !defined(NO_DETAIL_OBJECT_SHADOWS)
                    vec4 shadow_coords[4];
                    shadow_coords[0] = shadow_matrix[0] * vec4(world_vert, 1.0);
                    shadow_coords[1] = shadow_matrix[1] * vec4(world_vert, 1.0);
                    shadow_coords[2] = shadow_matrix[2] * vec4(world_vert, 1.0);
                    shadow_coords[3] = shadow_matrix[3] * vec4(world_vert, 1.0);
                    CALC_SHADOWED

                    #if defined(SIMPLE_SHADOW)
                        shadow_tex.r *= ambient_mult;
                    #endif

                    shadow_tex.r *= CloudShadow(world_vert);
                #else
                    vec3 shadow_tex = vec3(0.5f);
                #endif

                vec3 ambient_cube_color[6];

                for(int i=0; i<6; ++i){
                    ambient_cube_color[i] = vec3(0.0);
                }

                bool use_amb_cube = false;

                #if defined(CAN_USE_LIGHT_PROBES)
                    use_amb_cube = GetAmbientCube(world_vert, num_tetrahedra, ambient_color_buffer, ambient_cube_color, 0u);
                #endif

                CALC_DIRECT_DIFFUSE_COLOR

                diffuse_color += decal_diffuse_color;  // Is zero if decals aren't enabled

                if(!use_amb_cube){
                    diffuse_color += LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0) * GetAmbientContrib(1.0) * ambient_mult * env_ambient_mult;
                } else {
                    diffuse_color += SampleAmbientCube(ambient_cube_color, ws_normal) * GetAmbientContrib(1.0) * ambient_mult * env_ambient_mult;
                }

                vec3 spec_color = vec3(0.0);

                #if defined(DAMP_FOG) || defined(RAINY) || defined(WET) || defined(MISTY) || defined(VOLCANO) || defined(WATERFALL_ARENA) || defined(SKY_ARK) || defined(SHADOW_POINT_LIGHTS)
                    ambient_mult *= env_ambient_mult;  // Make ambient shadows apply to point lights
                #endif

                CalculateLightContrib(diffuse_color, spec_color, ws_vertex, world_vert, ws_normal, roughness, light_val, ambient_mult);

                // Put it all together
                vec3 base_color = texture(base_color_tex,tc1).rgb * color_tint;
                float overbright_adjusted = dist_fade * overbright;
                colormap.xyz = mix(base_color, mix(colormap.xyz, base_color * colormap.xyz / avg_color, tint_weight), dist_fade);
                colormap.xyz *= 1.0 + overbright_adjusted;

                #if defined(SWAMP) || defined(SWAMP2) || defined(CAVE_ARENA)
                    float opac = 0.0;

                    if(world_vert.y < kBeachLevel ){
                        #if defined(CAVE_ARENA)
                            float mult = 10.0;
                        #else
                            float mult = 5.0;
                        #endif

                        opac = max(0.0, (kBeachLevel - world_vert.y)*mult);
                        opac = min(1.0, opac);
                    }

                    colormap.xyz *= mix(1.0, 0.3, opac);

                    #if !defined(TERRAIN)
                        opac = max(opac, 0.7);
                    #endif

                    roughness *= mix(1.0, 0.0, opac);
                #endif

                vec3 color = diffuse_color * colormap.xyz;

                #if defined(ALBEDO_ONLY)
                    out_color = colormap;

                    return;
                #endif

                float haze_amount = GetHazeAmount(ws_vertex, haze_mult);

                #if !defined(MISTY) && !defined(RAINY) && !defined(WET) && !defined(MISTY2) && !defined(WATER_HORIZON) && !defined(SKY_ARK) && !defined(WATERFALL_ARENA)
                    vec3 fog_color = textureLod(spec_cubemap,ws_vertex ,5.0).xyz;
                #else
                    vec3 fog_color = textureLod(spec_cubemap,ws_vertex ,3.0).xyz;
                #endif

                #if defined(SNOW_EVERYWHERE2)
                    fog_color = vec3(1.0);
                #endif

                #if defined(VOLUME_SHADOWS)
                    fog_color.xyz *= mix(VolumeShadow(world_vert), 1.0, 0.3);
                #endif

                fog_color *= GetFogColorMult();
                color = mix(color, fog_color, haze_amount);

                #if defined(PLANT)
                    CALC_FINAL_ALPHA
                #else
                    CALC_FINAL
                #endif

                return;
            #endif // DEPTH_ONLY
        #elif defined(PARTICLE)
            #if defined(INSTANCED)
                vec4 color_tint = instance_color[instance_id];
                float size = length(instance_transform[instance_id] * vec4(0,0,1,0));
            #endif

            vec4 colormap = texture(tex0, tex_coord);

            #if defined(WATER)
                if(colormap.a * color_tint.a < 0.5){
                    discard;
                }
            #endif

            float random = rand(gl_FragCoord.xy);

            #if defined(DEPTH_ONLY)
                if(colormap.a *color_tint.a < random){
                    discard;
                }

                return;
            #else
                float spec_amount = 0.0;
                vec3 flame_final_color = vec3(0.0, 0.0, 0.0);;
                float flame_final_contrib = 0.0;
                vec3 ws_normal = vec3(1.0, 0.0, 0.0);
                float roughness = 0.0;
                float preserve_wetness = 1.0;
                float ambient_mult = 1.0;
                float env_ambient_mult = 1.0;
                vec3 decal_diffuse_color = vec3(0.0);

                #if !defined(NO_DECALS)
                    CalculateDecals(colormap, ws_normal, spec_amount, roughness, preserve_wetness, ambient_mult, env_ambient_mult, decal_diffuse_color, world_vert, time, decal_val, flame_final_color, flame_final_contrib);

                    if(ws_normal == vec3(-1.0)){
                        discard;
                    }
                #endif

                vec4 shadow_coords[4];

                int num_samples = 2;
                vec3 far = world_vert + normalize(world_vert-cam_pos) * size * 0.5;
                vec3 near = world_vert - normalize(world_vert-cam_pos) * size * 0.5;
                float shadowed = 0.0;

                for(int i=0; i<num_samples; ++i){
                    vec3 sample_vert = mix(far, near, (i+random)/float(num_samples));
                    ws_vertex = sample_vert - cam_pos;
                    shadow_coords[0] = shadow_matrix[0] * vec4(sample_vert, 1.0);
                    shadow_coords[1] = shadow_matrix[1] * vec4(sample_vert, 1.0);
                    shadow_coords[2] = shadow_matrix[2] * vec4(sample_vert, 1.0);
                    shadow_coords[3] = shadow_matrix[3] * vec4(sample_vert, 1.0);
                    float len = length(ws_vertex);
                    shadowed += GetCascadeShadow(shadow_sampler, shadow_coords, len);
                }

                shadowed /= float(num_samples);
                shadowed *= CloudShadow(world_vert);
                shadowed = 1.0 - shadowed;

                float env_depth = LinearizeDepth(textureLod(tex5,gl_FragCoord.xy / viewport_dims, 0.0).r);
                float particle_depth = LinearizeDepth(gl_FragCoord.z);
                float depth = env_depth - particle_depth;
                float depth_blend = depth / size * 0.5;
                depth_blend = max(0.0,min(1.0,depth_blend));
                depth_blend *= max(0.0,min(1.0, particle_depth*0.5-0.1));

                #if defined(NORMAL_MAP_TRANSLUCENT)
                    vec4 normalmap = texture(tex1, tex_coord);
                    ws_normal = vec3(tangent_to_world3 * normalmap.b +
                        tangent_to_world1 * (normalmap.r*2.0-1.0) +
                        tangent_to_world2 * (normalmap.g*2.0-1.0));
                    float surface_lighting = GetDirectContribSoft(ws_light, ws_normal, 1.0);
                    float subsurface_lighting = GetDirectContribSoft(ws_light, ws_light, 1.0) * 0.5;
                    float thickness = min(1.0,pow(colormap.a*color_tint.a*depth_blend,2.0)*2.0);
                    float NdotL = mix(subsurface_lighting, surface_lighting, thickness);
                    NdotL *= (1.0-shadowed);
                    vec3 diffuse_color = GetDirectColor(NdotL);
                    vec3 ambient_color = GetAmbientColor(world_vert, ws_normal);//LookupCubemapSimpleLod(ws_normal, tex3, 5.0);
                #elif defined(SPLAT)
                    vec4 normalmap = texture(tex1, tex_coord);
                    //normalmap.xyz = vec3(0.5, 0.5, 1.0);
                    ws_normal = vec3(tangent_to_world3 * normalmap.b +
                        tangent_to_world1 * (normalmap.r*2.0-1.0) +
                        tangent_to_world2 * (normalmap.g*2.0-1.0));
                    ws_normal = normalize(ws_normal);
                    vec3 diffuse_color = GetDirectColor(GetDirectContribSoft(ws_light, ws_normal, 1.0 - shadowed)*0.5);
                    vec3 ambient_color = GetAmbientColor(world_vert, ws_normal);//LookupCubemapSimpleLod(ws_normal, tex3, 5.0);
                #elif defined(WATER)
                    vec4 normalmap = texture(tex1, tex_coord);
                    ws_normal = vec3(tangent_to_world3 * normalmap.b +
                        tangent_to_world1 * (normalmap.r*2.0-1.0) +
                        tangent_to_world2 * (normalmap.g*2.0-1.0));

                    vec3 diffuse_color = GetDirectColor(GetDirectContribSoft(ws_light, ws_normal, 1.0 - shadowed)*0.5);
                    vec3 ambient_color = GetAmbientColor(world_vert, ws_normal);//LookupCubemapSimpleLod(ws_normal, tex3, 5.0);
                #else
                    float NdotL = GetDirectContribSimple((1.0-shadowed)*0.25);
                    vec3 diffuse_color = GetDirectColor(NdotL);
                    //vec3 ambient_color = LookupCubemapSimpleLod(cam_pos - world_vert, tex3, 5.0);
                    vec3 ambient_color = GetAmbientColor(world_vert, cam_pos - world_vert);
                #endif

                diffuse_color += ambient_color * GetAmbientContrib(1.0) * env_ambient_mult;
                diffuse_color += decal_diffuse_color;  // Is zero if decals aren't enabled

                CalculateLightContribParticle(diffuse_color, world_vert, light_val);

                vec3 color = diffuse_color * colormap.xyz *color_tint.xyz;

                #if defined(SPLAT)
                    vec3 blood_spec = vec3(GetSpecContrib(ws_light, normalize(ws_normal), ws_vertex, 1.0, 200.0)) * (1.0-shadowed);
                    blood_spec *= 10.0;
                    vec3 spec_map_vec = reflect(ws_vertex, mix(tangent_to_world3, ws_normal, 0.5));

                    #if defined(NO_REFLECTION_CAPTURE)
                        vec3 reflection_color = vec3(0.0);
                    #else
                        vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, normalize(spec_map_vec), 0.0);
                    #endif

                    color = mix(color, (blood_spec + reflection_color), 0.1);
                #elif defined(WATER)
                    vec3 blood_spec = vec3(GetSpecContrib(ws_light, normalize(ws_normal), ws_vertex, 1.0, 200.0)) * (1.0-shadowed);
                    blood_spec *= 10.0;
                    vec3 spec_map_vec = reflect(ws_vertex,ws_normal);

                    #if defined(NO_REFLECTION_CAPTURE)
                        vec3 reflection_color = vec3(0.0);
                    #else
                        vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, normalize(spec_map_vec), 0.0);
                    #endif

                    float glancing = max(0.0, min(1.0, 1.0 + dot(normalize(ws_vertex), ws_normal)));
                    float fresnel = pow(glancing, 6.0);
                    fresnel = mix(fresnel, 1.0, 0.05);
                    //color = pow(max(0.0, dot(ws_light, mix(normalize(ws_vertex), -ws_normal, 0.2))), 5.0) * (1.0-shadowed) * 2.0 * primary_light_color.a * primary_light_color.xyz;

                    #if !defined(NO_REFLECTION_CAPTURE)
                        color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, normalize(mix(normalize(ws_vertex), -ws_normal, 0.5)), 0.0);
                    #endif

                    color = mix(color, (blood_spec + reflection_color), fresnel);
                    color = mix(color, diffuse_color, 0.3);
                    colormap.a *= 0.5;
                #endif

                #if defined(GLOW)
                    color.xyz = colormap.xyz*color_tint.xyz*5.0;
                #endif

                float alpha = min(1.0, colormap.a*color_tint.a*depth_blend);

                #if defined(SPLAT)
                    if(alpha < 0.3){
                        discard;
                    }

                    alpha = min(1.0, (alpha - 0.3) * 6.0);
                #endif

                float haze_amount = GetHazeAmount(ws_vertex, haze_mult);
                vec3 fog_color = textureLod(spec_cubemap,ws_vertex ,5.0).xyz;

                #if defined(SNOW_EVERYWHERE2)
                    fog_color = vec3(1.0);
                #endif

                fog_color *= GetFogColorMult();
                color = mix(color, fog_color, haze_amount);

                out_color = vec4(color, alpha);

                #if defined(ALBEDO_ONLY)
                    out_color = vec4(colormap.xyz*color_tint.xyz, alpha);

                    return;
                #endif
            #endif

            //out_color.xyz = vec3(pow(colormap.a,2.0));
            //out_color = vec4(1.0);
        #else
            #if defined(NO_INSTANCE_ID)
                int instance_id = 0;
            #endif

            #if defined(CHARACTER)
                float alpha = texture(fur_tex, fur_tex_coord).a;
            #else
                #if defined(TERRAIN)
                    vec2 test_offset = (texture(warp_tex,frag_tex_coords.xy*200.0).xy-0.5)*0.001;
                    vec2 base_tex_coords = frag_tex_coords.xy + test_offset;
                    vec2 detail_coords = frag_tex_coords.zw;
                #else
                    vec2 base_tex_coords = frag_tex_coords;

                    #if !defined(ITEM)
                        vec4 instance_color_tint = GetInstancedColorTint(instance_id);
                    #endif

                    #if defined(DETAILMAP4)
                        vec2 detail_coords = base_tex_coords*GetInstancedDetailScale(instance_id).xy;
                    #endif
                #endif

                vec4 colormap = texture(tex0, base_tex_coords);
            #endif

            vec4 shadow_coords[4];

            #if !defined(DEPTH_ONLY)
                shadow_coords[0] = shadow_matrix[0] * vec4(world_vert, 1.0);
                shadow_coords[1] = shadow_matrix[1] * vec4(world_vert, 1.0);
                shadow_coords[2] = shadow_matrix[2] * vec4(world_vert, 1.0);
                shadow_coords[3] = shadow_matrix[3] * vec4(world_vert, 1.0);
            #endif

            #if defined(CHARACTER)
                #if !defined(ALPHA_TO_COVERAGE)
                    if(alpha < 0.6){
                        discard;
                    }
                #else
                    if(alpha < 0.0){
                        discard;
                    }
                #endif
            #else
                #if defined(ALPHA)
                    #if !defined(ALPHA_TO_COVERAGE)
                        if(colormap.a < 0.5){
                            discard;
                        }
                    #else
                        colormap.a = 0.5 + (colormap.a - 0.5) * 2.0;

                        if(colormap.a < 0.0){
                            discard;
                        }
                    #endif
                #endif
            #endif

            #if defined(DEPTH_ONLY)
                #if defined(CHARACTER)
                    out_color = vec4(0.0, 0.0, 0.0, alpha);
                #elif defined(ALPHA)
                    out_color = vec4(vec3(1.0), colormap.a);
                #else
                    out_color = vec4(vec3(1.0), 1.0);
                #endif

                return;
            #else
                #if defined(DETAILMAP4)
                    #if defined(TEXEL_DENSITY_VIZ)
                        int max_res[2];
                        max_res[0] = max(textureSize(tex0,0)[0], textureSize(tex1,0)[0]);
                        max_res[1] = max(textureSize(tex0,0)[1], textureSize(tex1,0)[1]);
                        out_color.xyz = vec3((int(tc0[0] * max_res[0] / 32.0)+int(tc0[1] * max_res[1] / 32.0))%2);
                        max_res[0] = max(textureSize(detail_color, 0)[0], textureSize(detail_normal, 0)[0]);
                        max_res[1] = max(textureSize(detail_color, 0)[1], textureSize(detail_normal, 0)[1]);
                        out_color.xyz += vec3((int(detail_coords[0] * max_res[0] / 32.0)+int(detail_coords[1] * max_res[1] / 32.0))%2);
                        out_color.xyz *= 0.5;
                        out_color.a = 1.0;

                        return;
                    #endif

                    vec4 weight_map = GetWeightMap(weight_tex, base_tex_coords);

                    #if defined(HEIGHT_BLEND)
                        float heights[4];

                        for(int i=0; i<4; ++i){
                            heights[i] = weight_map[i] + texture(detail_normal, vec3(detail_coords, i)).a;
                        }

                        for(int i=0; i<4; ++i){
                            weight_map[i] = pow(heights[i], 16.0);
                        }
                    #endif

                    float total = weight_map[0] + weight_map[1] + weight_map[2] + weight_map[3];
                    weight_map /= total;
                    CALC_DETAIL_FADE
                    // Get normal
                    float color_tint_alpha;
                    mat3 ws_from_ns;

                    #if defined(TERRAIN)
                        vec3 base_normalmap = texture(tex1,base_tex_coords).xyz;
                        vec3 base_normal = normalize((base_normalmap*vec3(2.0))-vec3(1.0));
                        vec3 base_bitangent = normalize(cross(frag_tangent,base_normal));
                        vec3 base_tangent = normalize(cross(base_normal,base_bitangent));
                    #else
                        vec4 base_normalmap = texture(tex1,base_tex_coords);
                        color_tint_alpha = base_normalmap.a;

                        #if defined(BASE_TANGENT)
                            vec3 base_normal = normalize(tan_to_obj * UnpackTanNormal(base_normalmap));
                        #else
                            vec3 base_normal = UnpackObjNormalV3(base_normalmap.xyz);
                        #endif

                        vec3 base_bitangent = normalize(cross(base_normal,tan_to_obj[0]));
                        vec3 base_tangent = normalize(cross(base_bitangent,base_normal));
                        base_bitangent *= 1.0 - step(dot(base_bitangent, tan_to_obj[1]),0.0) * 2.0;

                        {
                            if(GetInstancedModelScale(instance_id)[0] < 0.0){
                                base_tangent[0] *= -1.0;
                                base_bitangent[0] *= -1.0;
                                base_normal[0] *= -1.0;
                            }

                            if(GetInstancedModelScale(instance_id)[1] < 0.0){
                                base_tangent[1] *= -1.0;
                                base_bitangent[1] *= -1.0;
                                base_normal[1] *= -1.0;
                            }

                            if(GetInstancedModelScale(instance_id)[2] < 0.0){
                                base_tangent[2] *= -1.0;
                                base_bitangent[2] *= -1.0;
                                base_normal[2] *= -1.0;
                            }
                        }
                    #endif

                    ws_from_ns = mat3(base_tangent,
                        base_bitangent,
                        base_normal);

                    #if defined(AXIS_BLEND) && !defined(TERRAIN) && !defined(AXIS_UV)
                        float axis_blend_scale = 0.15;
                        {
                            vec3 rotated_base_normal = quat_mul_vec3(GetInstancedModelRotationQuat(instance_id), base_normal);
                            float temp = 2;
                            weight_map[0] = pow(abs(rotated_base_normal.x),temp);
                            weight_map[1] = pow(max(0.0, rotated_base_normal.y),temp);
                            weight_map[2] = pow(abs(rotated_base_normal.z),temp);
                            weight_map[3] = max(0.0, -rotated_base_normal.y);
                            float total = weight_map[0] + weight_map[1] + weight_map[2] + weight_map[3];
                            weight_map /= total;
                        }
                    #endif

                    vec3 ws_normal;
                    vec4 normalmap;

                    {
                        #if defined(TERRAIN)
                            normalmap = vec4(0.0);

                            if(detail_fade < 1.0){
                                for(int i=0; i<4; ++i){
                                    if(weight_map[i] > 0.0){
                                        normalmap += texture(detail_normal, vec3(detail_coords, i)) * weight_map[i] ;
                                    }
                                }
                            }
                        #elif defined(AXIS_UV)
                            normalmap = vec4(0.0);

                            if(detail_fade < 1.0){
                                vec2 temp_uv;
                                temp_uv = base_tex_coords;
                                temp_uv.x *= abs(dot(GetInstancedModelScale(instance_id), base_tangent));
                                temp_uv.y *= abs(dot(GetInstancedModelScale(instance_id), base_bitangent));

                                for(int i=0; i<4; ++i){
                                    if(weight_map[i] > 0.0){
                                        normalmap += texture(detail_normal, vec3(temp_uv, detail_normal_indices[i])) * weight_map[i];
                                    }
                                }
                            }
                        #elif defined(AXIS_BLEND)
                            normalmap = vec4(0.0);

                            if(detail_fade < 1.0){
                                for(int i=0; i<4; ++i){
                                    if(weight_map[i] > 0.0){
                                        vec2 axis_coord;

                                        if(i==0){
                                            axis_coord = world_vert.zy;
                                        } else if(i==2){
                                            axis_coord = world_vert.xy;
                                        } else {
                                            axis_coord = world_vert.xz;
                                        }

                                        axis_coord *= axis_blend_scale;
                                        normalmap += texture(detail_normal, vec3(axis_coord, detail_normal_indices[i])) * weight_map[i];
                                    }
                                }
                            }
                        #else
                            normalmap = vec4(0.0);

                            if(detail_fade < 1.0){
                                // TODO: would it be possible to reduce this to two samples by using the tex coord z to interpolate?
                                for(int i=0; i<4; ++i){
                                    if(weight_map[i] > 0.0){
                                        normalmap += texture(detail_normal, vec3(base_tex_coords*GetInstancedDetailScale(instance_id)[i], detail_normal_indices[i])) * weight_map[i];
                                    }
                                }
                            }
                        #endif

                        normalmap.xyz = UnpackTanNormal(normalmap);

                        #if defined(BEACH) && defined(TERRAIN)
                            if(world_vert.y < 16.9){
                                detail_fade = mix(detail_fade, 1.0, min(0.8, (16.9 - world_vert.y)*3.0));
                            }
                        #endif

                        normalmap.xyz = mix(normalmap.xyz,vec3(0.0,0.0,1.0),detail_fade);

                        #if defined(NO_DETAILMAPS)
                            normalmap.xyz = vec3(0.0,0.0,1.0);
                        #endif

                        #if defined(TERRAIN)
                            ws_normal = ws_from_ns * normalmap.xyz;
                        #else
                            ws_normal = normalize(quat_mul_vec3(GetInstancedModelRotationQuat(instance_id), ws_from_ns * normalmap.xyz));
                        #endif
                    }

                    // Get color
                    vec4 base_color = texture(color_tex,base_tex_coords);

                    #if !defined(KEEP_SPEC)
                        base_color.a = 0.0;
                    #endif

                    vec4 tint;

                    {
                        vec4 average_color = avg_color0 * weight_map[0] +
                        avg_color1 * weight_map[1] +
                        avg_color2 * weight_map[2] +
                        avg_color3 * weight_map[3];
                        average_color = max(average_color, vec4(0.01));
                        tint = base_color / average_color;

                        #if defined(TERRAIN)
                            tint[3] = 1.0;
                            base_color[3] = average_color[3];
                        #endif
                    }

                    #if defined(TERRAIN)
                        colormap = vec4(0.0);

                        if(detail_fade < 1.0){
                            for(int i=0; i<4; ++i){
                                if(weight_map[i] > 0.0){
                                    colormap += texture(detail_color, vec3(detail_coords, detail_color_indices[i])) * weight_map[i] ;
                                }
                            }
                        }
                    #elif defined(AXIS_UV)
                        colormap = vec4(0.0);

                        if(detail_fade < 1.0){
                            vec2 temp_uv;
                            temp_uv = base_tex_coords;
                            temp_uv.x *= abs(dot(GetInstancedModelScale(instance_id), base_tangent));
                            temp_uv.y *= abs(dot(GetInstancedModelScale(instance_id), base_bitangent));

                            for(int i=0; i<4; ++i){
                                if(weight_map[i] > 0.0){
                                    colormap += texture(detail_color, vec3(temp_uv, detail_color_indices[i])) * weight_map[i];
                                }
                            }
                        }
                    #elif defined(AXIS_BLEND)
                        colormap = vec4(0.0);

                        if(detail_fade < 1.0){
                            for(int i=0; i<4; ++i){
                                vec2 axis_coord;

                                if(i==0){
                                    axis_coord = world_vert.zy;
                                } else if(i==2){
                                    axis_coord = world_vert.xy;
                                } else {
                                    axis_coord = world_vert.xz;
                                }

                                axis_coord *= axis_blend_scale;

                                if(weight_map[i] > 0.0){
                                    colormap += texture(detail_color, vec3(axis_coord, detail_color_indices[i])) * weight_map[i];
                                }
                            }
                        }
                    #else
                        colormap = vec4(0.0);

                        if(detail_fade < 1.0){
                            for(int i=0; i<4; ++i){
                                if(weight_map[i] > 0.0){
                                    colormap += texture(detail_color, vec3(base_tex_coords*GetInstancedDetailScale(instance_id)[i], detail_color_indices[i])) * weight_map[i];
                                }
                            }
                        }
                    #endif

                    colormap = mix(colormap * tint, base_color, detail_fade);

                    #if defined(NO_DETAILMAPS)
                        colormap = base_color;
                    #endif

                    #if !defined(TERRAIN)
                        colormap.xyz = mix(colormap.xyz,colormap.xyz*instance_color_tint.xyz,color_tint_alpha);
                    #endif

                    colormap.a = min(1.0, max(0.0,colormap.a));

                    /*//colormap.xyz = mix(colormap.xyz, vec3(1.0,0.0,0.0), weight_map[1]);
                    out_color.xyz = colormap.xyz;
                    out_color.a = 1.0;

                    return;
                    */

                    //colormap.a = mix(colormap.a, 1.0, weight_map[1]);
                    /*float a = avg_color0.a;
                    float b = texture(detail_color, vec3(detail_coords, detail_color_indices[0])).a;
                    float val = mix(a, b, sin(time)*0.5+0.5);
                    out_color.xyz = vec3(val);
                    out_color.a = 1.0;

                    return;*/
                #elif defined(ITEM)
                    #if defined(TEXEL_DENSITY_VIZ)
                        int max_res[2];
                        max_res[0] = max(textureSize(tex0,0)[0], textureSize(tex1,0)[0]);
                        max_res[1] = max(textureSize(tex0,0)[1], textureSize(tex1,0)[1]);
                        out_color.xyz = vec3((int(tc0[0] * max_res[0] / 32.0)+int(tc0[1] * max_res[1] / 32.0))%2);

                        return;
                    #endif

                    float blood_amount, wetblood;
                    vec4 blood_texel = textureLod(blood_tex, tc0, 0.0);
                    blood_amount = min(blood_texel.r*5.0, 1.0);
                    wetblood = max(0.0,blood_texel.g*1.4-0.4);

                    vec4 normalmap = texture(tex1,tc0);

                    #if !defined(TANGENT)
                        vec3 os_normal = UnpackObjNormal(normalmap);
                        vec3 ws_normal = model_rotation_mat * os_normal;
                    #else
                        vec3 unpacked_normal = UnpackTanNormal(normalmap);
                        //vec3 world_dx = dFdx(world_vert);
                        //vec3 world_dy = dFdy(world_vert);
                        //vec3 ws_normal = normalize(cross(world_dx, world_dy));
                        vec3 ws_normal = model_rotation_mat * frag_normal;
                        mat3 cotangent_frame = cotangent_frame(ws_normal, normalize(world_vert - cam_pos), tc0);
                        ws_normal = cotangent_frame * unpacked_normal;
                    #endif

                    ws_normal = normalize(ws_normal);
                    colormap.xyz *= mix(vec3(1.0),color_tint,normalmap.a);

                    /*
                    out_color.xyz = ws_normal;
                    out_color.a = 1.0;

                    return;
                    */

                    normalmap.a = mix(normalmap.a, 1.0, blood_amount * 0.5);
                    //CALC_BLOOD_ON_COLOR_MAP
                #elif defined(CHARACTER)
                    #if defined(TEXEL_DENSITY_VIZ)
                        int max_res[2];
                        max_res[0] = max(textureSize(color_tex,0)[0], textureSize(normal_tex,0)[0]);
                        max_res[1] = max(textureSize(color_tex,0)[1], textureSize(normal_tex,0)[1]);
                        out_color.xyz = vec3((int(morphed_tex_coord[0] * max_res[0] / 32.0)+int(morphed_tex_coord[1] * max_res[1] / 32.0))%2);

                        return;
                    #endif

                    // Reconstruct third bone axis
                    vec3 concat_bone3 = cross(concat_bone1, concat_bone2);

                    float blood_amount, wetblood;
                    vec4 blood_texel = textureLod(blood_tex, vec2(tex_coord[0], 1.0-tex_coord[1]), 0.0);
                    ReadBloodTex(blood_tex, vec2(tex_coord[0], 1.0-tex_coord[1]), blood_amount, wetblood);

                    /*
                    out_color.xyz = vec3(blood_texel);
                    out_color.a = 1.0;

                    return;
                    */

                    vec2 tex_offset = vec2(pow(blood_texel.g, 8.0)) * 0.001;

                    // Get world space normal
                    vec4 normalmap = texture(normal_tex, tex_coord + tex_offset);
                    #if defined(TANGENT)
                        vec3 unpacked_normal = UnpackTanNormal(normalmap);
                        vec3 unrigged_normal = tan_to_obj * unpacked_normal;
                    #else
                        vec3 unrigged_normal = UnpackObjNormal(normalmap);
                    #endif
                    vec3 ws_normal = normalize(concat_bone1 * unrigged_normal.x +
                        concat_bone2 * unrigged_normal.y +
                        concat_bone3 * unrigged_normal.z);

                    vec4 colormap = texture(color_tex, morphed_tex_coord + tex_offset);
                    vec4 tintmap = texture(tint_map, morphed_tex_coord + tex_offset);
                    float tint_total = max(1.0, tintmap[0] + tintmap[1] + tintmap[2] + tintmap[3]);
                    tintmap /= tint_total;
                    vec3 tint_mult = mix(vec3(0.0), tint_palette[0], tintmap.r) +
                    mix(vec3(0.0), tint_palette[1], tintmap.g) +
                    mix(vec3(0.0), tint_palette[2], tintmap.b) +
                    mix(vec3(0.0), tint_palette[3], tintmap.a) +
                    mix(vec3(0.0), tint_palette[4], 1.0-(tintmap.r+tintmap.g+tintmap.b+tintmap.a));
                    colormap.xyz *= tint_mult;

                    CALC_BLOOD_ON_COLOR_MAP
                #else
                    #if defined(TEXEL_DENSITY_VIZ)
                        int max_res[2];
                        max_res[0] = max(textureSize(tex0,0)[0], textureSize(tex1,0)[0]);
                        max_res[1] = max(textureSize(tex0,0)[1], textureSize(tex1,0)[1]);
                        out_color.xyz = vec3((int(tc0[0] * max_res[0] / 32.0)+int(tc0[1] * max_res[1] / 32.0))%2);
                        out_color.a = 1.0;

                        return;
                    #endif

                    #if defined(WATER)
                        vec3 base_ws_normal;
                        vec3 base_water_offset;
                        float water_depth = LinearizeDepth(gl_FragCoord.z);
                    #endif

                    #if defined(TANGENT)
                        vec3 ws_normal;
                        vec4 normalmap = texture(normal_tex,tc0);

                        {
                            vec3 unpacked_normal = UnpackTanNormal(normalmap);

                            #if defined(WATER)
                                vec3 tint = instance_color_tint.xyz;
                                float sample_height[3];
                                float eps = 0.015 / tint[1];
                                vec2 water_uv = world_vert.xz * 0.2;
                                vec4 proj_test_point = (projection_view_mat * vec4(world_vert, 1.0));
                                proj_test_point /= proj_test_point.w;
                                proj_test_point.xy += vec2(1.0);
                                proj_test_point.xy *= 0.5;

                                #if !defined(SIMPLE_WATER)
                                    float old_depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r);
                                #endif

                                sample_height[0] = GetWaterHeight(water_uv, tint);
                                sample_height[1] = GetWaterHeight(water_uv + vec2(eps, 0.0), tint);
                                sample_height[2] = GetWaterHeight(water_uv + vec2(0.0, eps), tint);
                                unpacked_normal.x = sample_height[1] - sample_height[0];
                                unpacked_normal.y = sample_height[2] - sample_height[0];
                                unpacked_normal.z = eps;

                                base_water_offset = normalize(unpacked_normal);

                                #if !defined(SIMPLE_WATER)
                                    if(gl_FrontFacing){
                                        water_depth += abs(base_water_offset.y)/(1.0+abs(normalize(ws_vertex).y));

                                        #if defined(SWAMP) || defined(RAINY) || defined(SWAMP2)
                                            water_depth += noise(world_vert.xz*20)*0.02;
                                        #endif

                                        if(water_depth > old_depth){
                                            discard;
                                        }
                                    }
                                #endif

                                #if defined(ALBEDO_ONLY)
                                    if(base_water_offset[0] < 0.0 || base_water_offset[1] < 0.0){
                                        discard;
                                    } else {
                                        out_color = vec4(vec3(0.4), 1.0);

                                        return;
                                    }
                                #endif

                                base_ws_normal = normalize(quat_mul_vec3(GetInstancedModelRotationQuat(instance_id), tan_to_obj * vec3(0,0,1)));
                            #endif

                            ws_normal = normalize(quat_mul_vec3(GetInstancedModelRotationQuat(instance_id), tan_to_obj * unpacked_normal));

                            #if defined(WATER)
                                //ws_normal = vec3(0,1,0);
                                //ws_normal = normalize(vec3(unpacked_normal.x, unpacked_normal.z, unpacked_normal.y));
                            #endif
                        }
                    #else
                        vec4 normalmap = texture(tex1,tc0);
                        vec3 os_normal = UnpackObjNormal(normalmap);
                        vec3 ws_normal = quat_mul_vec3(GetInstancedModelRotationQuat(instance_id), os_normal);
                    #endif

                    #if !defined(WATER)
                        colormap.xyz *= instance_color_tint.xyz;
                    #endif
                #endif

                #if !defined(PLANT)
                    #if defined(ALPHA)
                        float spec_amount = normalmap.a;
                    #else
                        float spec_amount = colormap.a;

                        #if !defined(CHARACTER) && !defined(ITEM) && !defined(METALNESS_PBR)
                            spec_amount = GammaCorrectFloat(spec_amount);
                        #endif
                    #endif
                #endif

                /*
                {
                vec3 world_dx = dFdx(world_vert);
                vec3 world_dy = dFdy(world_vert);
                ws_normal = normalize(cross(world_dx, world_dy));
                }*/

                #if defined(COLLISION)
                    {
                        vec3 world_dx = dFdx(world_vert);
                        vec3 world_dy = dFdy(world_vert);
                        vec3 ws_normal = normalize(cross(world_dx, world_dy));
                        vec3 custom_normal = texelFetch(light_decal_data_buffer, gl_PrimitiveID).xyz;
                        int val = int(custom_normal[1]);
                        ws_normal = mix(ws_normal, custom_normal, 0.8);
                        out_color.xyz = ws_normal.xyz * 0.5 + vec3(0.5);

                        if(val == 0 || custom_normal[1] == 1.0){
                        } else if(val <= 3){
                            out_color.xyz = vec3(0,1,0);

                            if(val == 2){
                                out_color.xyz += vec3(0.5,0.0,0.0);
                            }

                            if(val == 3){
                                out_color.xyz += vec3(0.0,0.0,0.5);
                            }
                        } else if(val <= 6){
                            out_color.xyz = vec3(1,0,0);

                            if(val == 5){
                                out_color.xyz += vec3(0.0,0.5,0.0);
                            }

                            if(val == 6){
                                out_color.xyz += vec3(0.0,0.0,0.5);
                            }
                        } else if(val==9){
                            out_color.xyz = vec3(0.5);
                        } else {
                            out_color.xyz = vec3(0,0,1);
                        }

                        //out_color.xyz *= color_tint;
                        float mult = 1.0;

                        if((int(world_vert.x)+int(world_vert.z))%2==0){
                            mult = 0.8;
                        }

                        if(int(world_vert.y)%2==0){
                            mult *= 0.8;
                        }

                        out_color.xyz *= mult;
                        out_color.a = 1.0;

                        return;
                    }
                #endif

                #if defined(CHARACTER)
                    spec_amount = GammaCorrectFloat(spec_amount);
                    float roughness = pow(1.0 - spec_amount, 20.0);
                #elif defined(ITEM)
                    float roughness = normalmap.a;
                #elif defined(METALNESS_PBR)
                    float roughness = (1.0 - normalmap.a);
                #elif defined(TERRAIN)
                    #if defined(SNOWY)
                        float old_spec = spec_amount;
                        float roughness = 0.99;
                        roughness = mix(roughness, 0.7, weight_map[1]);
                        roughness = mix(roughness, 0.2, weight_map[0]);
                        spec_amount = mix(spec_amount, 0.4, weight_map[1]);
                        spec_amount = mix(spec_amount, 0.5, weight_map[0]);
                        colormap.xyz *= mix(1.0, 0.25, weight_map[2]);
                        colormap.xyz *= mix(1.0, 0.5, weight_map[3]);
                        colormap.xyz *= mix(1.0, 0.8, weight_map[0]);
                    #elif defined(SNOW_EVERYWHERE2) || defined(SNOW_EVERYWHERE3)
                        float old_spec = spec_amount;
                        float roughness = 0.99;
                    #else
                        float roughness = 0.99;
                    #endif

                    /*
                    out_color.xyz = vec3(spec_amount);//colormap.a);
                    out_color.a = 1.0;

                    return;*/
                #elif defined(KEEP_SPEC)
                    float roughness = (1.0 - normalmap.a);
                #else
                    float roughness = mix(0.7, 1.0, pow((colormap.x + colormap.y + colormap.z) / 3.0, 0.01));
                #endif

                //out_color = vec4(vec3(roughness), 1.0);
                //return;

                //out_color = vec4(vec3(ws_normal), 1.0);
                //return;

                // wet character
                #if defined(CHARACTER)
                    float wet = 0.0;

                    if(blood_texel.g < 1.0){
                        wet = blood_texel.g;//pow(max(blood_texel.g-0.2, 0.0)/0.8, 0.5);

                        #if defined(RAINY) || defined(WET)
                            #if !defined(CHARACTER_DRY)
                                wet = 1.0;
                            #endif
                        #else
                            colormap.xyz *= mix(1.0, 0.5, wet);
                        #endif

                        roughness = mix(roughness, 0.3, wet);
                    }
                #endif

                #if defined(PLANT)
                    float spec_amount = 0.0;
                #endif

                float preserve_wetness = 1.0;
                float ambient_mult = 1.0;
                float env_ambient_mult = 1.0;

                vec3 flame_final_color = vec3(0.0, 0.0, 0.0);
                float flame_final_contrib = 0.0;

                #if defined(WATER)
                    float extra_froth = 0.0;
                #endif

                vec3 decal_diffuse_color = vec3(0.0);

                #if !defined(NO_DECALS)
                    #if defined(INSTANCED_MESH)
                        vec4 old_colormap = colormap;
                        float old_spec_amount = spec_amount;
                    #endif

                    #if defined(WATER)
                        roughness = 0.0;

                        #if defined(DIRECTED_WATER_DECALS)
                            colormap = vec4(0.0);
                        #endif
                    #endif

                    {
                        CalculateDecals(colormap, ws_normal, spec_amount, roughness, preserve_wetness, ambient_mult, env_ambient_mult, decal_diffuse_color, world_vert, time, decal_val, flame_final_color, flame_final_contrib);
                    }

                    #if defined(WATER)
                        #if defined(DIRECTED_WATER_DECALS)
                            extra_froth = colormap.r;
                            colormap.xyz = vec3(1.0);
                        #else
                            extra_froth = roughness;
                        #endif
                    #endif

                    #if defined(INSTANCED_MESH)
                        if(instance_color_tint[3] == -1.0){
                            colormap = old_colormap;
                            ambient_mult = 1.0;
                        }
                    #endif
                #endif

                #if (defined(SNOW_EVERYWHERE) || defined(SNOW_EVERYWHERE2) || defined(SNOW_EVERYWHERE3)) && !defined(WATER)
                    {
                        float snow_blend = 0.0;

                        #if !defined(CHARACTER) && !defined(ITEM)
                            #if defined(SNOW_EVERYWHERE)
                                float snow_low = -3;
                                float snow_high = 80;
                                float water = -3;
                                float snow_amount = pow(min(1.0, (world_vert.y - snow_low) / (snow_high - snow_low)), 0.2);
                                float snow_cover = ws_normal.y - mix(1.0, 0.2, snow_amount);

                                if(snow_cover > 0.0){
                                    snow_blend = min(1.0, snow_cover * 20.0);
                                }

                                if(world_vert.y < water){
                                    float wet = min(1.0, (water - world_vert.y)*0.7);
                                    spec_amount = mix(spec_amount, 0.1, wet);
                                    colormap.xyz = mix(colormap.xyz, vec3(0.0), wet*0.9);
                                    roughness = mix(roughness, 0.8, wet);
                                }
                            #else
                                #if defined(SNOW_EVERYWHERE2)
                                    float snow_cover = ws_normal.y - 0.2;
                                #elif defined(SNOW_EVERYWHERE3)
                                    float snow_cover = (ws_normal.y+max(0.0, abs(ws_normal.x)*0.65+0.25)) - 0.95;
                                #endif

                                #if defined(INSTANCED_MESH)
                                    if(instance_color_tint[3] == -1.0){
                                        snow_cover = 0.0;
                                    }
                                #endif

                                if(snow_cover > 0.0){
                                    snow_blend = min(1.0, snow_cover * 20.0);
                                }

                                #if defined(TERRAIN)
                                    snow_blend *= 1.0;
                                #endif

                                snow_blend *= roughness;
                            #endif
                        #else
                            snow_blend = (max(0.0, ws_normal.y - 0.3) * 1.0) * roughness;
                            snow_blend = mix(snow_blend, 0.0, blood_amount);
                        #endif

                        snow_blend *= preserve_wetness;

                        if(snow_blend > 0.0) {
                            colormap.xyz = mix(colormap.xyz, vec3(0.55), snow_blend);
                            roughness = mix(roughness, 0.6, snow_blend);
                            spec_amount = mix(spec_amount, 0.0, snow_blend);
                            ws_normal = mix(ws_normal, vec3(1.0), 0.3*snow_blend);
                            ws_normal = normalize(ws_normal);

                            #if !defined(TERRAIN) && !defined(ITEM) && !defined(CHARACTER)
                                instance_color_tint = mix(instance_color_tint, vec4(1.0), snow_blend);
                            #endif
                        }
                    }
                #endif

                #if defined(ALBEDO_ONLY)
                    out_color = vec4(colormap.xyz,1.0);

                    return;
                #endif

                vec3 shadow_tex = vec3(1.0);

                #if defined(SIMPLE_SHADOW)
                    {
                        vec3 X = dFdx(world_vert);
                        vec3 Y = dFdy(world_vert);
                        vec3 norm = normalize(cross(X, Y));
                        float slope_dot = dot(norm, ws_light);
                        slope_dot = min(slope_dot, 1);
                        shadow_tex.r = GetCascadeShadow(tex4, shadow_coords, length(ws_vertex), slope_dot);
                    }
                    shadow_tex.r *= ambient_mult;
                #else
                    shadow_tex.r = GetCascadeShadow(tex4, shadow_coords, length(ws_vertex));
                #endif

                shadow_tex.r *= CloudShadow(world_vert);

                #if defined(WATERFALL)
                    ws_normal = vec3(0,0.5,0);
                #endif

                CALC_DIRECT_DIFFUSE_COLOR

                diffuse_color += decal_diffuse_color;  // Is zero if decals aren't enabled

                bool use_amb_cube = false;
                bool use_3d_tex = false;
                vec3 ambient_cube_color[6];

                for(int i=0; i<6; ++i){
                    ambient_cube_color[i] = vec3(0.0);
                }

                vec3 ambient_color = vec3(0.0);

                #if defined(CAN_USE_3D_TEX)
                    for(int i=0; i<light_volume_num; ++i){
                        //vec3 temp = (world_vert - reflection_capture_pos[i]) / reflection_capture_scale[i];
                        vec3 temp = (light_volume_matrix_inverse[i] * vec4(world_vert, 1.0)).xyz;
                        vec3 scale_vec = (light_volume_matrix[i] * vec4(1.0, 1.0, 1.0, 0.0)).xyz;
                        float scale = dot(scale_vec, scale_vec);
                        float val = dot(temp, temp);

                        if(temp[0] <= 1.0 && temp[0] >= -1.0 &&
                                temp[1] <= 1.0 && temp[1] >= -1.0 &&
                                temp[2] <= 1.0 && temp[2] >= -1.0) {
                            vec3 tex_3d = temp * 0.5 + vec3(0.5);
                            vec4 test = texture(tex16, vec3((tex_3d[0] + 0)/ 6.0, tex_3d[1], tex_3d[2]));

                            if(test.a >= 1.0 && tex_3d[0] > 0.01 && tex_3d[0] < 0.99){
                                for(int j=1; j<6; ++j){
                                    ambient_cube_color[j] = texture(tex16, vec3((tex_3d[0] + j)/ 6.0, tex_3d[1], tex_3d[2])).xyz;
                                }

                                ambient_cube_color[0] = test.xyz;
                                ambient_color = SampleAmbientCube(ambient_cube_color, ws_normal);
                                use_3d_tex = true;
                            }
                        }
                    }
                #endif

                if(!use_3d_tex){
                    use_amb_cube = false;

                    #if defined(CAN_USE_LIGHT_PROBES)
                        uint guess = 0u;
                        int grid_coord[3];
                        bool in_grid = true;

                        for(int i=0; i<3; ++i){
                            if(world_vert[i] > grid_bounds_max[i] || world_vert[i] < grid_bounds_min[i]){
                                in_grid = false;
                                break;
                            }
                        }

                        if(in_grid){
                            grid_coord[0] = int((world_vert[0] - grid_bounds_min[0]) / (grid_bounds_max[0] - grid_bounds_min[0]) * float(subdivisions_x));
                            grid_coord[1] = int((world_vert[1] - grid_bounds_min[1]) / (grid_bounds_max[1] - grid_bounds_min[1]) * float(subdivisions_y));
                            grid_coord[2] = int((world_vert[2] - grid_bounds_min[2]) / (grid_bounds_max[2] - grid_bounds_min[2]) * float(subdivisions_z));
                            int cell_id = ((grid_coord[0] * subdivisions_y) + grid_coord[1])*subdivisions_z + grid_coord[2];
                            uvec4 data = texelFetch(ambient_grid_data, cell_id/4);
                            guess = data[cell_id%4];
                            use_amb_cube = GetAmbientCube(world_vert, num_tetrahedra, ambient_color_buffer, ambient_cube_color, guess);
                        }

                        ambient_color = SampleAmbientCube(ambient_cube_color, ws_normal);
                    #endif

                    if(!use_amb_cube){
                        ambient_color = LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0);
                    }
                }

                #if defined(VOLCANO)
                    {
                        float height = world_vert.y + 22;

                        vec3 center = vec3(-33, -32, 2.5);
                        vec3 offset = world_vert - center;
                        offset.y = 0.0;
                        float direct_light = max(0.0, dot(normalize(center - world_vert), ws_normal));
                        direct_light = mix(direct_light, 1.0, min(1.0, 1.0/pow(height, 0.7)));
                        //ambient_color = vec3(direct_light);

                        #if defined(LESS_GLOW)
                            ambient_color += direct_light * vec3(1,0.1,0) / max(1.0, pow(height, 1.8)) * 10.0;
                        #else
                            ambient_color += direct_light * vec3(1,0.1,0) / max(1.0, pow(height, 1.0)) * 10.0;
                        #endif
                    }
                #endif

                #if defined(TERRAIN_DEPTH_GRASS_TEST)
                    #if defined(TERRAIN)
                        float val = 0.0;
                        vec3 sample_point = world_vert;

                        for(int i=0; i<10; ++i){
                            float height = sample_point.y - world_vert.y;
                            val += (noise(sample_point.xz*40)+1.0)*0.005*height*10.0;
                            sample_point -= normalize(ws_vertex) * 0.02;
                        }

                        out_color.xyz = vec3(val);
                        out_color.a = 1.0;
                        gl_FragDepth = UnLinearizeDepth(LinearizeDepth(gl_FragCoord.z)-(noise(gl_FragCoord.xy*vec2(0.4,0.03))+2.0)*0.05/abs(normalize(ws_vertex).y));

                        //return;
                    #endif
                #endif

                // Screen space reflection test
                #if !defined(SIMPLE_WATER)
                    #define SCREEN_SPACE_REFLECTION
                #endif

                #if defined(SCREEN_SPACE_REFLECTION) && !defined(DEPTH_ONLY)
                    #if defined(WATER)
                        vec3 screen_space_reflect;

                        {
                            vec3 spec_map_vec = normalize(reflect(ws_vertex,ws_normal));
                            //out_color.xyz = vec3(0.0);
                            bool done = false;
                            bool good = false;
                            int count = 0;
                            vec4 proj_test_point;
                            float step_size = 0.01;
                            vec3 march = world_vert;
                            float screen_step_mult = abs(dot(spec_map_vec, normalize(ws_vertex)));
                            float random = rand(gl_FragCoord.xy);

                            vec3 test_point = march;
                            proj_test_point = (projection_view_mat * vec4(test_point, 1.0));
                            proj_test_point /= proj_test_point.w;
                            proj_test_point.xy += vec2(1.0);
                            proj_test_point.xy *= 0.5;
                            vec2 a = proj_test_point.xy;

                            test_point += spec_map_vec * 0.1;
                            proj_test_point = (projection_view_mat * vec4(test_point, 1.0));
                            proj_test_point /= proj_test_point.w;
                            proj_test_point.xy += vec2(1.0);
                            proj_test_point.xy *= 0.5;
                            vec2 b = proj_test_point.xy;

                            screen_step_mult = length(a - b) * 10.0;
                            step_size /= screen_step_mult;
                            float buf_extend = 0.05;

                            while(!done){
                                march += spec_map_vec * step_size;
                                test_point = march + (spec_map_vec * step_size * random * 1.5);
                                proj_test_point = (projection_view_mat * vec4(test_point, 1.0));
                                proj_test_point /= proj_test_point.w;
                                proj_test_point.xy += vec2(1.0);
                                proj_test_point.xy *= 0.5;
                                proj_test_point.z = (proj_test_point.z + 1.0) * 0.5;
                                proj_test_point.z = LinearizeDepth(proj_test_point.z);
                                float depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r);
                                ++count;

                                if(count > 20 || proj_test_point.x < -buf_extend || proj_test_point.y < 0.0 || proj_test_point.x > 1.0+buf_extend || proj_test_point.y > 1.0){
                                    done = true;
                                }

                                if((count > 20 && depth > proj_test_point.z) || (depth < proj_test_point.z && abs(depth - proj_test_point.z) < step_size * 2.0)){
                                    done = true;
                                    good = true;
                                    march -= spec_map_vec * step_size;
                                }

                                if(!done){
                                    step_size *= 1.5;
                                }
                            }

                            if(good){
                                done = false;
                                count = 0;

                                while(!done){
                                    march += spec_map_vec * step_size * 0.2;
                                    test_point = march + (spec_map_vec * step_size * random * 1.5 * 0.2);
                                    proj_test_point = (projection_view_mat * vec4(test_point, 1.0));
                                    proj_test_point /= proj_test_point.w;
                                    proj_test_point.xy += vec2(1.0);
                                    proj_test_point.xy *= 0.5;
                                    proj_test_point.z = (proj_test_point.z + 1.0) * 0.5;
                                    proj_test_point.z = LinearizeDepth(proj_test_point.z);
                                    float depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r);
                                    ++count;

                                    if(count > 10 || proj_test_point.x < -buf_extend || proj_test_point.y < 0.0 || proj_test_point.x > 1.0+buf_extend || proj_test_point.y > 1.0){
                                        done = true;
                                    }

                                    if((count > 10 && depth > proj_test_point.z) || (depth < proj_test_point.z && abs(depth - proj_test_point.z) < step_size * 2.0)){
                                        done = true;
                                    }
                                }
                            }

                            //out_color.r = depth * 0.01;
                            //out_color.g = proj_test_point.z * 0.01;
                            //out_color.b = 0.0;
                            //out_color.xyz = texture(tex17, proj_test_point.xy).xyz;
                            //out_color.xyz = vec3(proj_test_point.x);
                            float reflect_amount = min(1.0, pow(-abs(dot(ws_normal, normalize(ws_vertex))) + 1.0, 2.0));
                            reflect_amount = 1.0;
                            float screen_space_amount = 0.0;
                            //good = false;
                            vec3 reflect_color = vec3(0.0);

                            if(good){
                                /*proj_test_point = (prev_projection_view_mat * vec4(test_point, 1.0));
                                proj_test_point /= proj_test_point.w;
                                proj_test_point.xy += vec2(1.0);
                                proj_test_point.xy *= 0.5;
                                proj_test_point.z = (proj_test_point.z + 1.0) * 0.5;
                                proj_test_point.z = LinearizeDepth(proj_test_point.z);*/

                                reflect_color = textureLod(tex17, proj_test_point.xy, 0.0).xyz;
                                screen_space_amount = 1.0;
                                //screen_space_amount *= min(1.0, max(0.0, pow((0.5 - abs(proj_test_point.x-0.5))*2.0, 1.0))*8.0);
                                screen_space_amount *= min(1.0, max(0.0, pow((0.5 - abs(proj_test_point.y-0.5))*2.0, 1.0))*8.0);
                            }

                            #if !defined(NO_REFLECTION_CAPTURE)
                                reflect_color = mix(reflect_color,
                                    LookUpReflectionShapes(ref_cap_cubemap, world_vert, spec_map_vec, 0.0/*roughness * 3.0*/) * ambient_mult * env_ambient_mult,
                                    1.0 - screen_space_amount);
                            #endif

                            screen_space_reflect = reflect_color;
                            out_color.xyz = mix(out_color.xyz, reflect_color, reflect_amount);
                            //out_color.xyz = vec3(reflect_amount);
                            //out_color.xyz = vec3(abs(depth - proj_test_point.z) * 0.1);
                        }
                    #endif  // WATER
                #endif  // defined(SCREEN_SPACE_REFLECTION) && !defined(DEPTH_ONLY)

                #if defined(SSAO_TEST)
                    env_ambient_mult *= SSAO(ws_vertex);
                #endif

                diffuse_color += ambient_color * GetAmbientContrib(shadow_tex.g) * ambient_mult * env_ambient_mult;

                #if defined(BEACH)
                    float kBeachLevel = 16.7 + sin(time*0.5-2.0)*0.05;
                #endif

                #if defined(WATERFALL_ARENA)
                    float kBeachLevel = 95;
                #endif

                #if defined(PLANT)
                    vec3 spec_color = vec3(0.0);
                    vec3 translucent_lighting = GetDirectColor(shadow_tex.r) * primary_light_color.a;
                    translucent_lighting += ambient_color * GetAmbientContrib(shadow_tex.g) * ambient_mult * env_ambient_mult;
                    translucent_lighting *= GammaCorrectFloat(0.6);
                    vec3 translucent_map = texture(translucency_tex, frag_tex_coords).xyz;
                    /*
                    roughness = 0.0;
                    float spec_pow = mix(1200.0, 20.0, pow(roughness,2.0));
                    float reflection_roughness = roughness;
                    roughness = mix(0.00001, 0.9, roughness);
                    float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,spec_pow);
                    spec *= 100.0* mix(1.0, 0.01, roughness);
                    spec_color = primary_light_color.xyz * vec3(spec);
                    vec3 spec_map_vec = normalize(reflect(ws_vertex,ws_normal));

                    #if defined(NO_REFLECTION_CAPTURE)
                    vec3 reflection_color = vec3(0.0);
                    #else
                    vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, spec_map_vec, reflection_roughness * 3.0);
                    #endif
                    spec_color += reflection_color;

                    float spec_amount = 0.0;
                    float glancing = max(0.0, min(1.0, 1.0 + dot(normalize(ws_vertex), ws_normal)));
                    float base_reflectivity = spec_amount;
                    float fresnel = pow(glancing, 4.0) * (1.0 - roughness) * 0.05;
                    float spec_val = mix(base_reflectivity, 1.0, fresnel);
                    spec_amount = spec_val;*/

                    #if defined(SWAMP)
                        if(world_vert.y < kBeachLevel ){
                            float mult = 5.0;
                            float opac = max(0.0, (kBeachLevel - world_vert.y)*mult);
                            opac = min(1.0, opac);
                            //spec_amount = mix(spec_amount, 1.0, 0.1 * opac);
                            roughness *= mix(1.0, 0.2, opac);
                            colormap.xyz *= mix(1.0, 0.3, opac);
                            translucent_map.xyz *= mix(1.0, 0.3, opac);
                        }
                    #endif

                    CalculateLightContrib(diffuse_color, spec_color, ws_vertex, world_vert, ws_normal, roughness, light_val, ambient_mult * env_ambient_mult);
                    diffuse_color *= colormap.xyz;
                    diffuse_color += translucent_lighting * translucent_map;
                    diffuse_color *= instance_color_tint.xyz;

                    #if defined(WATER_HORIZON) || defined(SNOW_EVERYWHERE) || defined(MORE_REFLECT) || defined(BEACH) || defined(SKY_ARK) || defined(WATERFALL_ARENA)
                        Caustics(kBeachLevel, ws_normal, diffuse_color);
                    #endif

                    //vec3 color = mix(diffuse_color, spec_color, spec_amount);
                    vec3 color = diffuse_color;
                #elif defined(WATERFALL)
                    vec3 spec_color = vec3(0.0);

                    CalculateLightContrib(diffuse_color, spec_color, ws_vertex, world_vert, ws_normal, roughness, light_val, ambient_mult * env_ambient_mult);

                    #if defined(WATER_HORIZON) || defined(SNOW_EVERYWHERE) || defined(MORE_REFLECT) || defined(BEACH) || defined(SKY_ARK) || defined(WATERFALL_ARENA)
                        Caustics(kBeachLevel, ws_normal, diffuse_color);
                    #endif

                    vec3 color = diffuse_color;
                #else
                    vec3 spec_color = vec3(0.0);

                    #if defined(CHARACTER)
                        float reflection_roughness = roughness;
                        roughness = mix(0.00001, 0.9, roughness);

                        float spec_pow = 2/pow(max(0.3, roughness), 4.0) - 2.0;
                        float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,spec_pow);
                        spec *= (spec_pow + 8) / (8 * 3.141592);
                        spec_color = primary_light_color.xyz * vec3(spec);

                        vec3 spec_map_vec = reflect(ws_vertex,ws_normal);

                        #if defined(NO_REFLECTION_CAPTURE)
                            vec3 reflection_color = vec3(0.0);
                        #else
                            vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, normalize(spec_map_vec), reflection_roughness*3.0);
                        #endif

                        #if defined(VOLCANO)
                            spec_color += reflection_color;
                        #else
                            spec_color += reflection_color * ambient_mult * env_ambient_mult;
                        #endif

                        float glancing = max(0.0, min(1.0, 1.0 + dot(normalize(ws_vertex), ws_normal)));
                        float base_reflectivity = mix(spec_amount * 0.1, 0.03, wet);
                        float fresnel = pow(glancing, 6.0) * (1.0 - roughness * 0.5);
                        fresnel = mix(fresnel, pow(glancing, 5.0), wet);
                        fresnel *= (1.0 + ws_normal.y) * 0.5;

                        float spec_val = mix(base_reflectivity, 1.0, fresnel);
                        spec_amount = spec_val;

                        /*
                        if(!use_amb_cube && !use_3d_tex){
                            CALC_BLOODY_CHARACTER_SPEC
                        } else {
                            float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,
                                mix(200.0,50.0,(1.0-wetblood)*blood_amount));
                            spec *= 5.0;
                            spec_color = primary_light_color.xyz * vec3(spec) * 0.3;
                            vec3 spec_map_vec = reflect(ws_vertex, ws_normal);
                            spec_color += SampleAmbientCube(ambient_cube_color, spec_map_vec) * 0.2 * max(0.0,(1.0 - blood_amount * 2.0));
                        }
                        */

                    /*
                    #elif defined(ITEM)
                        float reflection_roughness = roughness;
                        roughness = mix(0.00001, 0.9, roughness);
                        float spec_pow = mix(1200.0, 20.0, pow(roughness,2.0));
                        float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,spec_pow);
                        spec *= 20.0 * mix(1.0, 0.01, roughness);
                        spec_color = primary_light_color.xyz * vec3(spec);
                        vec3 spec_map_vec = reflect(ws_vertex,ws_normal);

                        #if defined(NO_REFLECTION_CAPTURE)
                            vec3 reflection_color = vec3(0.0);
                        #else
                            vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, spec_map_vec, reflection_roughness * 3.0);
                        #endif

                        spec_color += reflection_color;
                        float glancing = max(0.0, min(1.0, 1.0 + dot(normalize(ws_vertex), ws_normal)));
                        float base_reflectivity = spec_amount;
                        float fresnel = pow(glancing, 6.0) * mix(0.7, 1.0, blood_amount);
                        float spec_val = mix(base_reflectivity, 1.0, fresnel);
                        spec_amount = spec_val;
                    */
                    #elif defined(METALNESS_PBR) || defined(ITEM) || defined(KEEP_SPEC) || defined(TERRAIN) || ((defined(RAINY) || defined(WET) || defined(SWAMP) || defined(BEACH) || defined(WATER_HORIZON) || defined(WATERFALL_ARENA) || defined(VOLCANO) || defined(SKY_ARK) || defined(SNOW_EVERYWHERE) || defined(SNOW_EVERYWHERE2) || defined(SNOW_EVERYWHERE3) || defined(DAMP_FOG) || defined(SWAMP2)) && !defined(WATER))
                        #if defined(RAINY) || defined(WET)
                            {
                                float sponge = 1.0 - preserve_wetness;

                                #if !defined(TERRAIN)
                                    roughness *= mix(0.3, 1.0, sponge);
                                #else
                                    roughness *= mix(0.3, 1.0, mix(min(1.0, length(ws_vertex)*0.1), 1.0, sponge));
                                #endif

                                #if defined(RAINY)
                                    ws_normal.y *= 1.0+noise(world_vert.xz*33.0+vec2(time*10.0,0.0))*0.25 * (1.0 - sponge);
                                    ws_normal.y *= 1.0+noise(world_vert.xz*17.0+vec2(time*-10.0,time * 4.0))*0.2 * (1.0 - sponge);
                                    ws_normal = normalize(ws_normal);
                                #endif
                            }
                        #endif

                        #if defined(BEACH)
                            if(world_vert.y < kBeachLevel ){
                                float mult = 5.0;
                                float opac = min(1.0, max(0.0, (kBeachLevel - world_vert.y)*mult));

                                if(opac > 5.0){
                                    opac = 0.0;
                                }

                                //spec_amount = mix(spec_amount, 1.0, 0.1 * opac);
                                roughness *= mix(1.0, 0.0, opac);
                                colormap.xyz *= mix(1.0, 0.5, opac);
                            }
                        #endif

                        #if defined(CAVE_ARENA)
                            spec_amount += 0.2 / (1.0 + length(ws_vertex) * 0.2);
                        #endif

                        #if defined(SWAMP) || defined(SWAMP2) || defined(CAVE_ARENA)
                            float opac = 0.0;

                            if(world_vert.y < kBeachLevel ){
                                #if defined(CAVE_ARENA)
                                    float mult = 10.0;
                                #else
                                    float mult = 5.0;
                                #endif

                                opac = max(0.0, (kBeachLevel - world_vert.y)*mult);
                                opac = min(1.0, opac);
                            }

                            colormap.xyz *= mix(1.0, 0.3, opac);

                            #if !defined(TERRAIN)
                                opac = max(opac, 0.7);
                            #endif

                            roughness *= mix(1.0, 0.0, opac);
                        #endif

                        #if defined(DAMP_FOG) || defined(WATERFALL_ARENA)
                            float opac = 0.6;

                            #if defined(TERRAIN)
                                opac *= 0.5;
                            #endif

                            roughness *= mix(1.0, 0.2, opac);
                            colormap.xyz *= mix(1.0, 0.3, opac);
                        #endif

                        #if defined(WATER_HORIZON)
                            if(world_vert.y < kBeachLevel ){
                                float mult = 1.0;
                                float opac = max(0.0, (kBeachLevel - world_vert.y)*mult);
                                opac = min(1.0, opac);
                                //spec_amount = mix(spec_amount, 1.0, 0.1 * opac);
                                roughness *= mix(1.0, 0.2, opac);
                                colormap.xyz *= mix(1.0, 0.3, opac);
                            }

                            const float kTintHeight = kBeachLevel+6;

                            if(world_vert.y < kTintHeight ){
                                float mult = 0.2;
                                float opac = max(0.0, (kTintHeight - world_vert.y)*mult);
                                opac -= pow(max(0.0, ws_normal.y),2.0);
                                opac = max(0.0, min(1.0, opac));
                                colormap.xyz *= mix(vec3(1.0), vec3(122/255.0, 89/255.0, 30/255.0), opac*0.8);
                            }
                        #endif

                        #if defined(SKY_ARK)
                            if(world_vert.y < kBeachLevel ){
                                float mult = 0.5;
                                float opac = max(0.0, (kBeachLevel - world_vert.y)*mult);
                                opac = min(1.0, opac);
                                //spec_amount = mix(spec_amount, 1.0, 0.1 * opac);
                                roughness *= mix(1.0, 0.2, opac);
                                colormap.xyz *= mix(1.0, 0.3, opac);
                            }

                            const float kTintHeight = 170.0;

                            if(world_vert.y < kTintHeight ){
                                float mult = 0.02;
                                float opac = 1.0;
                                opac -= (ws_normal.y*0.9+ws_normal.z*0.4);
                                opac *= 0.5+((noise((world_vert.xz+vec2(world_vert.y*0.05,0.0))*5.0)+1.0)*0.5);
                                opac *= max(0.0, (kTintHeight - world_vert.y)*mult);
                                opac *= 0.7;
                                opac = min(1.0, opac);
                                colormap.xyz *= mix(vec3(1.0), vec3(122/255.0, 89/255.0, 30/255.0), opac*0.8);
                            }
                        #endif

                        #if defined(ITEM)
                            spec_amount = GammaCorrectFloat(spec_amount);

                            colormap.xyz = mix(colormap.xyz, blood_tint * 0.4, blood_amount);
                            spec_amount = mix(spec_amount, 0.0, blood_amount);
                            roughness = mix(roughness, 0.0, blood_amount * 0.2);
                        #endif

                        #if defined(VOLCANO)
                            roughness = max(0.0, 1.0 - colormap.x * 2.0);
                        #endif

                        float reflection_roughness = roughness;
                        roughness = mix(0.00001, 0.9, roughness);
                        float metalness = pow(spec_amount, 0.3);

                        /*#if defined(ITEM)
                            spec_amount = mix((1.0 - roughness) * 0.15, 1.0, metalness);
                        #endif*/

                        float spec_pow = 2/pow(max(0.25, roughness), 4.0) - 2.0;
                        float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,spec_pow);
                        spec *= (spec_pow + 8) / (8 * 3.141592);
                        spec_color = primary_light_color.xyz * vec3(spec);

                        #if defined(SWAMP)
                            spec_color *= 0.1;
                        #endif

                        vec3 spec_map_vec = normalize(reflect(ws_vertex,ws_normal));

                        #if defined(NO_REFLECTION_CAPTURE)
                            vec3 reflection_color = vec3(0.0);
                        #else
                            vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, spec_map_vec, min(2.5, reflection_roughness * 5.0));
                        #endif

                        // Disabled in case ambient_cube_color is not initialized
                        //reflection_color = mix(reflection_color, SampleAmbientCube(ambient_cube_color, spec_map_vec), max(0.0, reflection_roughness * 2.0 - 1.0));

                        #if defined(VOLCANO)
                            spec_color += reflection_color * ambient_mult;
                        #else
                            spec_color += reflection_color * ambient_mult * env_ambient_mult;
                        #endif

                        float glancing = max(0.0, min(1.0, 1.0 + dot(normalize(ws_vertex), ws_normal)));
                        float base_reflectivity = spec_amount;
                        float fresnel = pow(glancing, 4.0) * (1.0 - roughness) * mix(0.5, 1.0, metalness);
                        float spec_val = mix(base_reflectivity, 1.0, fresnel);
                        spec_amount = spec_val;

                        #if defined(VOLCANO)
                            spec_amount *= ambient_mult;
                        #else
                            spec_amount *= ambient_mult * env_ambient_mult;
                        #endif

                        //out_color.xyz = vec3(colormap.xyz);
                        //out_color.a = 1.0;
                        //return;
                    #else // Standard envobject
                        #if defined(WATER)
                            roughness = instance_color_tint.x * 0.3;
                            spec_amount = 0.03;
                            //colormap.xyz = vec3(0.0, 0.01, 0.01);
                            colormap.xyz = vec3(0.02, 0.03, 0.02);

                            #if defined(SWAMP)
                                colormap.xyz = vec3(188/255.0, 152/255.0, 98/255.0) * 0.05;
                            #endif

                            float spec_pow = 2500;

                            if(!gl_FrontFacing){
                                ws_normal *= -1.0;
                            }
                        #else
                            float spec_pow = mix(1200.0, 20.0, pow(roughness,2.0));
                        #endif

                        float reflection_roughness = roughness;
                        roughness = mix(0.00001, 0.9, roughness);
                        float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,spec_pow);

                        #if defined(WATER)
                            spec *= 15.0;
                        #else
                            spec *= 100.0* mix(1.0, 0.01, roughness);
                        #endif

                        spec_color = primary_light_color.xyz * vec3(spec);
                        vec3 spec_map_vec = normalize(reflect(ws_vertex,ws_normal));

                        #if defined(SCREEN_SPACE_REFLECTION) && defined(WATER)
                            vec3 reflection_color = screen_space_reflect;
                        #else
                            #if defined(NO_REFLECTION_CAPTURE)
                                vec3 reflection_color = vec3(1.0);
                            #else
                                vec3 reflection_color = LookUpReflectionShapes(ref_cap_cubemap, world_vert, spec_map_vec, reflection_roughness * 3.0) * ambient_mult * env_ambient_mult;
                            #endif
                        #endif

                        spec_color += reflection_color;

                        float glancing = max(0.0, min(1.0, 1.0 + dot(normalize(ws_vertex), ws_normal)));
                        float base_reflectivity = spec_amount;

                        #if defined(WATER)
                            float fresnel;

                            if(!gl_FrontFacing){
                                fresnel = pow(glancing, 0.2);
                            } else {
                                fresnel = 0.8 * pow(max(0.0, 1.0 + dot(normalize(ws_vertex), ws_normal)), 5.0);

                                #if defined(MORE_REFLECT)
                                    fresnel = pow(1.0 + dot(normalize(ws_vertex), ws_normal), 2.0);
                                #endif

                                #if defined(CAVE_ARENA)
                                    fresnel = pow(1.0 + dot(normalize(ws_vertex), ws_normal), 3.0);
                                #endif
                            }

                            float spec_val = mix(base_reflectivity, 1.0, fresnel);
                            spec_amount = 1.0;

                            //out_color.xyz = reflection_color;
                            //return;
                        #else
                            float fresnel = pow(glancing, 4.0) * (1.0 - roughness) * 0.05;
                            float spec_val = mix(base_reflectivity, 1.0, fresnel);
                            spec_amount = spec_val;
                        #endif
                    #endif

                    #if !defined(WATER)
                        #if defined(METALNESS_PBR)
                            colormap.xyz *= instance_color_tint.xyz;
                        #elif !defined(ALPHA) && !defined(DETAILMAP4) && !defined(CHARACTER) && !defined(ITEM)
                            colormap.xyz *= mix(vec3(1.0),instance_color_tint.xyz,normalmap.a);
                        #endif
                    #endif

                    #if defined(DAMP_FOG) || defined(RAINY) || defined(MISTY) || defined(VOLCANO) || defined(WATERFALL_ARENA) || defined(SKY_ARK) || defined(SHADOW_POINT_LIGHTS)
                        ambient_mult *= env_ambient_mult;
                    #endif

                    CalculateLightContrib(diffuse_color, spec_color, ws_vertex, world_vert, ws_normal, roughness, light_val, ambient_mult);

                    #if defined(WATER_HORIZON) || defined(MORE_REFLECT) || defined(SNOW_EVERYWHERE) || defined(BEACH) || defined(SKY_ARK) || defined(WATERFALL_ARENA)
                        Caustics(kBeachLevel, ws_normal, diffuse_color);
                    #endif

                    #if defined(METALNESS_PBR) || defined(ITEM)
                        spec_color = mix(spec_color, spec_color * colormap.xyz, metalness);
                    #endif

                    vec3 color = mix(diffuse_color * colormap.xyz, spec_color, spec_amount);

                    #if defined(INSTANCED_MESH) && defined(WATERFALL)
                        if(instance_color_tint[3] == -1.0){
                            color *= 2.0; // Make armor twice as bright on waterfall level, not sure why it's too dark otherwise
                        }
                    #endif
                #endif

                #if defined(CHARACTER)
                    // Add rim highlight
                    vec3 view = normalize(ws_vertex*-1.0);
                    float back_lit = max(0.0,dot(normalize(ws_vertex),ws_light));
                    float rim_lit = max(0.0,(1.0-dot(view,ws_normal)));
                    rim_lit *= pow((dot(ws_light,ws_normal)+1.0)*0.5,0.5);
                    color += vec3(back_lit*rim_lit) * (1.0 - blood_amount) * normalmap.a * primary_light_color.xyz * primary_light_color.a * shadow_tex.r * mix(vec3(1.0), colormap.xyz, 0.8);
                #endif

                //CALC_HAZE
                //AddHaze(color, ws_vertex, spec_cubemap);

                /*
                #if defined(CHARACTER)
                    out_color.xyz = vec3(roughness);
                    out_color.xyz = LookupCubemapSimpleLod(spec_map_vec, tex2, roughness * 5.0);

                    out_color.xyz = vec3(spec_color);
                #endif*/
                /*

                #if defined(CHARACTER)
                    vec3 spec_map_vec = reflect(ws_vertex,ws_normal);
                    float fresnel = dot(ws_normal, normalize(ws_vertex));
                    out_color.xyz = mix(textureLod(spec_cubemap,spec_map_vec,1.0).xyz, textureLod(spec_cubemap,ws_normal,5.0).xyz, 0.0);//max(0.0, min(1.0, pow(fresnel * -1.0, 2.0))));
                    out_color.xyz = textureLod(spec_cubemap,ws_vertex + ws_normal,0.0).xyz;
                #endif*/

                #if defined(ITEM)
                    // out_color.xyz = spec_color.xyz;
                #endif

                #if defined(WATER) // Handle water transparency, refraction, fog
                    #if !defined(SIMPLE_WATER)
                        vec4 proj_test_point = (projection_view_mat * vec4(world_vert, 1.0));
                        proj_test_point /= proj_test_point.w;
                        proj_test_point.xy += vec2(1.0);
                        proj_test_point.xy *= 0.5;
                        // proj_test_point is now world position in screen space
                        float flat_depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r);
                        float old_depth = flat_depth - water_depth;
                        // old_depth is now the amount of water we are looking through
                        const float refract_mult = 1.0;
                        vec2 distort = vec2(base_water_offset.xy) / max(0.1, instance_color_tint[1]);
                        distort *= max(0.0, min(old_depth, 1.0) ); // Scale refraction based on depth
                        distort /= (water_depth * 1.0 + 0.3); // Reduce refraction based on camera distance from water
                        distort *= refract_mult; // Arbitrarily control refraction amount
                        proj_test_point.xy += distort;

                        float depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r) - water_depth;

                        { // Prevent objects above the water from bleeding into the refraction
                            if(depth < 0.25){
                                proj_test_point.xy -= distort * 0.5;
                                depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r) - water_depth;
                            }

                            if(depth < 0.125){
                                proj_test_point.xy -= distort * 0.5;
                                depth = LinearizeDepth(textureLod(tex18, proj_test_point.xy, 0.0).r) - water_depth;
                            }
                        }

                        vec3 under_water = textureLod(tex17, proj_test_point.xy, 0.0).xyz; // color texture from opaque objects

                        if(gl_FrontFacing){ // Only add foam and fog if water is viewed from outside of water
                            //#if !defined(NO_DECALS)
                            #if defined(SWAMP)
                                under_water = mix(under_water, diffuse_color * colormap.xyz, max(0.0, min(1.0, pow(depth * 2.0 * instance_color_tint[2], 0.2)))); // Add fog
                            #elif defined(CAVE_ARENA)
                                under_water = mix(under_water, diffuse_color * colormap.xyz * 3.0, max(0.0, min(1.0, pow(depth * 8.0 * instance_color_tint[2], 0.2)))); // Add fog
                            #else
                                under_water = mix(under_water, diffuse_color * colormap.xyz, max(0.0, min(1.0, pow(depth * 0.3 * instance_color_tint[2], 0.2)))); // Add fog
                            #endif
                            //#endif

                            #if !defined(SWAMP) && !defined(RAINY)
                                float min_depth = -0.3;
                                float max_depth = 0.1;
                                float foam_detail = texture(tex0, (world_vert.xz + normalize(vec2(0.0, 1.0))*time*water_speed)*5.0).y +
                                texture(tex0, (world_vert.xz * 0.5 + normalize(vec2(1.0, 0.0))*time*water_speed)*7.0).y;
                                foam_detail *= 0.5;
                                foam_detail = min(1.0, foam_detail+0.3);

                                if(depth < max_depth && depth > min_depth && abs(old_depth - depth) < 0.1){ // Blend in foam
                                    if(depth > 0.0){
                                        under_water = mix(diffuse_color * 0.3, under_water, min(1.0, depth/max_depth + foam_detail));
                                    } else {
                                        under_water = mix(diffuse_color * 0.3, under_water, depth/-min_depth);
                                    }
                                }
                            #endif

                            color.xyz = mix(under_water, color.xyz, spec_val);

                            if(depth < length(ws_vertex) * 0.05){
                                float a = dFdx(depth);
                                float b = dFdy(depth);
                                float meniscus = min(1.0, max(0.0, 1.0 - depth / length(vec3(a,b,0)) * 0.5));
                                //meniscus *= max(0.0, 1.0 - abs(a) * 1000);

                                #if defined(CAVE_ARENA)
                                    color.xyz *= 1.0 + meniscus * 0.75;
                                #else
                                    color.xyz += diffuse_color * meniscus * 0.05;
                                #endif
                            }

                            {
                                vec2 temp_tex_coords = world_vert.xz * 0.1;
                                temp_tex_coords.x += sin(world_vert.x*4.0 + time * 2.0) * 0.004;
                                temp_tex_coords.y += sin(world_vert.z*4.0 + time * 2.6) * 0.004;
                                temp_tex_coords.x += ws_normal.x * 0.02;
                                temp_tex_coords.y += ws_normal.z * 0.02;

                                #if defined(WATERFALL_ARENA) && !defined(NO_WATER_SCROLL)
                                    temp_tex_coords.y += time * 0.4;
                                #endif

                                #if defined(WATER_FROTH_SCROLL_X_SLOW)
                                    temp_tex_coords.x -= time * 0.1;
                                #endif

                                vec3 temp_color = texture(tex1, temp_tex_coords).xyz;

                                #if defined(DIRECTED_WATER_DECALS)
                                    color.xyz = mix(color.xyz, diffuse_color * 4.0, pow(extra_froth, 1.0));
                                #else
                                    color.xyz = mix(color.xyz, diffuse_color * temp_color, pow((extra_froth * temp_color.r), 1.5));
                                #endif
                            }

                            //out_color.xyz = vec3(old_depth);
                            //out_color.xyz = vec3(max(0.0, min(1.0, pow(depth * 0.1,0.8))));
                        } else {
                            color.xyz *= 0.1;
                            color.xyz = mix(under_water, color.xyz, spec_val);
                        }
                    #else
                        color.xyz = mix(diffuse_color * colormap.xyz, color.xyz, spec_val);

                        {
                            vec2 temp_tex_coords = world_vert.xz * 0.1;
                            temp_tex_coords.x += sin(world_vert.x*4.0 + time * 2.0) * 0.004;
                            temp_tex_coords.y += sin(world_vert.z*4.0 + time * 2.6) * 0.004;
                            temp_tex_coords.x += ws_normal.x * 0.02;
                            temp_tex_coords.y += ws_normal.z * 0.02;

                            #if defined(WATERFALL_ARENA)
                                temp_tex_coords.y += time * 0.4;
                            #endif

                            vec3 temp_color = texture(tex1, temp_tex_coords).xyz;

                            #if defined(DIRECTED_WATER_DECALS)
                                color.xyz = mix(color.xyz, diffuse_color * 4.0, pow(extra_froth, 1.0));
                            #else
                                color.xyz = mix(color.xyz, diffuse_color * temp_color, pow((extra_froth * temp_color.r), 1.5));
                            #endif
                        }
                    #endif

                #endif  // WATER

                #if defined(SNOWY) || ( (defined(SNOW_EVERYWHERE2) || defined(SNOW_EVERYWHERE3) ) && defined(TERRAIN))
                    // Snow sparkle
                    if(old_spec * preserve_wetness > 0.4 && sin((world_vert.x - cam_pos.x * 0.5)*20.0) > 0.0 && sin((world_vert.z - cam_pos.z * 0.5)*13.0) > 0.0 && sin((world_vert.y - cam_pos.y * 0.5)*15.0) > 0.0){
                        color.xyz += reflection_color * 1.5;//vec3(1.0);
                        color.xyz += GetDirectColor(shadow_tex.r) * primary_light_color.a * 1.5;//vec3(1.0);
                    }
                #endif


                float haze_amount = GetHazeAmount(ws_vertex, haze_mult);
                vec3 fog_color;

                if(use_3d_tex){
                    fog_color = SampleAmbientCube(ambient_cube_color, normalize(ws_vertex) * -1.0);
                    /*
                    #if defined(CAN_USE_3D_TEX)
                        vec3 dir = normalize(ws_vertex) * -1.0;
                        vec3 sample_color;
                        fog_color = vec3(0.0);

                        for(int i=0; i<10; ++i){
                            Query3DTexture(sample_color, mix(cam_pos, world_vert, (i+1)/10.0), dir);
                            fog_color += sample_color;
                        }

                        fog_color /= 10.0;
                    #endif*/
                } else if(!use_amb_cube){
                    float val = min(5.0, 5.0 / (length(ws_vertex)*0.01+1.0) + haze_mult * 50.0);

                    #if defined(VOLCANO)
                        val = 5.0;
                    #endif

                    #if defined(MISTY) || defined(RAINY) || defined(WET) || defined(MISTY2) || (defined(WATER_HORIZON) && !defined(ALT2)) || defined(SKY_ARK) || defined(WATERFALL_ARENA)
                        val = 3.0;
                    #endif

                    fog_color = textureLod(spec_cubemap, ws_vertex, val).xyz;
                } else {
                    fog_color = SampleAmbientCube(ambient_cube_color, ws_vertex * -1.0);
                }

                #if defined(WATER_HORIZON) && defined(ALT2)
                    fog_color = mix(fog_color, vec3(1.0), max(0.0, min(1.0, length((cam_pos+ws_vertex).xz)/200.0-2)));
                #endif

                #if defined(SNOW_EVERYWHERE2)
                    fog_color = vec3(1.0);
                #endif

                fog_color *= GetFogColorMult();

                #if !defined(TERRAIN) && !defined(CHARACTER) && !defined(ITEM)
                    #if defined(EMISSIVE)
                        color.xyz = colormap.xyz * instance_color_tint.xyz;
                        //}
                    #endif
                #endif

                #if defined(MAGMA_FLOOR)
                    vec2 temp_tex_coords = world_vert.xz * 0.2;
                    temp_tex_coords.x += sin(world_vert.x*4.0 + time * 2.0) * 0.004;
                    temp_tex_coords.y += sin(world_vert.z*4.0 + time * 2.6) * 0.004;
                    vec2 frag_tex_coordsB = temp_tex_coords;                            //copy the variable
                    frag_tex_coordsB.x += time * 0.05;     //makes texture move back and fourth
                    //frag_tex_coordsB.y += sin(frag_tex_coordsB.x + time * 1) * 0.02;

                    float offset = fractal(world_vert.xz * 0.2);
                    float vec = (fractal(world_vert.xz * 1)+0.7)*0.7;
                    vec = (vec*vec*(3-2*vec));
                    vec = (vec*vec*(3-2*vec));
                    vec = (vec*vec*(3-2*vec));
                    vec3 temp_color = mix(texture(tex0, temp_tex_coords + vec2(offset*0.2)).xyz, texture(tex0, temp_tex_coords.yx*0.8+vec2(0.5,0.5)-vec2(offset*0.2)).xyz, vec);

                    color.xyz = temp_color * 16.0 * mix(1.0, texture(tex1, frag_tex_coordsB).a, pow(temp_color.r,0.2));
                    color.xyz *= max(0.0, fractal(world_vert.xz*0.5)+0.5);
                #endif

                #if defined(MAGMA_FLOW)
                    vec2 frag_tex_coordsB = frag_tex_coords;                            //copy the variable
                    vec2 frag_tex_coordsC = frag_tex_coords;                            //copy the variable
                    frag_tex_coordsB.y -= time * 0.3;                                   //makes texture 'scroll' in the y axis
                    frag_tex_coordsC.y -= time * 0.5;

                    vec3 temp_color = texture(tex0, frag_tex_coordsB).xyz * (pow(texture(tex1, frag_tex_coordsC).a, 2.2) + 0.1);
                    color.xyz = temp_color * 2.0;
                #endif

                #if defined(VOLUME_SHADOWS)
                    fog_color *= mix(VolumeShadow(world_vert), 1.0, 0.3);
                #endif

                #if !defined(NO_VELOCITY_BUF)

                    #if defined(CHARACTER)
                        out_vel.xyz = vel;
                        out_vel.a = 1.0;
                    #elif defined(ITEM)
                        out_vel.xyz = vel * 60.0;
                        out_vel.a = 1.0;
                    #else
                        out_vel = vec4(0.0);
                    #endif

                #endif  // ^ !defined(NO_VELOCITY_BUF)

                #if defined(FIRE_DECAL_ENABLED) && !defined(NO_DECALS)
                    color.xyz = mix(color.xyz, flame_final_color, flame_final_contrib);
                #endif // FIRE_DECAL_ENABLED

                #if defined(CHARACTER)
                    vec3 temp = orig_vert * 2.0;

                    int burn_int = int(blood_texel.b * 255.0);

                    if(burn_int > 0){
                        int on_fire = 0;

                        if(burn_int > 127){
                            on_fire = 1;
                        }

                        int burnt_amount = burn_int - on_fire * 128;

                        float burned = abs(fractal(temp.xz*11.0)+fractal(temp.xy*7.0)+fractal(temp.yz*5.0));
                        out_color.xyz *= mix(1.0, burned*0.3, float(burnt_amount)/127.0);

                        if(on_fire == 1){
                            float fade = 0.4;// max(0.0, (0.5 - length(temp))*8.0)* max(0.0, fractal(temp.xz*7.0)+0.3);
                            float fire = abs(fractal(temp.xz*11.0+time*3.0)+fractal(temp.xy*7.0-time*3.0)+fractal(temp.yz*5.0-time*3.0));
                            float flame_amount = max(0.0, 0.5 - (fire*0.5 / pow(fade, 2.0))) * 2.0;
                            //fade = pow(abs(fractal(temp.xz*3.0+time)+fractal(temp.xy*2.0-time)+fractal(temp.yz*3.0-time))*0.9, 4.0);
                            flame_amount += pow(max(0.0, 0.7-fire), 2.0);
                            float opac = mix(pow(1.0-abs(dot(ws_normal, -normalize(ws_vertex))), 12.0), 1.0, pow((ws_normal.y+1.0)/2.0, 20.0));
                            color.xyz = mix(color.xyz,
                                vec3(1.5 * pow(flame_amount, 0.7), 0.5 * flame_amount, 0.1 * pow(flame_amount, 2.0)),
                                opac);

                            #if !defined(NO_VELOCITY_BUF)
                                out_vel.xyz += vec3(0.0, fire, 0.0) * 10.0 * on_fire;
                            #endif  // NO_VELOCITY_BUF
                        }
                    }
                #endif

                #if defined(WATERFALL_ARENA) && !defined(CAVE)
                    haze_amount = max(haze_amount, min(1.0, length(ws_vertex) * 0.01));
                #endif

                color = mix(color, fog_color, haze_amount);

                #if defined(SKY_ARK) && defined(WATER)
                    float fade_amount = min(1.0, length(world_vert) * 0.0005);
                    fade_amount = max(0.0, fade_amount - 0.75);
                    fade_amount *= 4.0;
                    //color.xyz = vec3(pow(fade_amount, 10.0));
                    color.xyz = mix(color.xyz, textureLod(spec_cubemap, ws_vertex, 0).xyz * vec3(1.2,1.1,1.0), min(fade_amount, 1.0));
                #endif

                #if defined(ALPHA)
                    out_color = vec4(color,colormap.a);
                #elif defined(WATER)
                    out_color = vec4(color,spec_val);
                #else
                    out_color = vec4(color,1.0);
                #endif

                /*
                vec3 avg;

                for(int i=0; i<6; ++i){
                    avg += ambient_cube_color[i];
                }

                avg /= 6.0;
                out_color.xyz = avg;
                out_color.xyz = ambient_color;
                */

                /*
                #if defined(CHARACTER)
                    float dark_world_amount = sin(time)*0.5+0.5;
                    out_color.xyz = mix(out_color.xyz, vec3(mix(1.0,0.8, dark_world_amount) - dot(ws_normal, -normalize(ws_vertex))),dark_world_amount);
                    out_color.a = 1.0;
                #endif
                */

                #if defined(CHARACTER)
                    out_color.a = alpha;
                #endif

                #if defined(TERRAIN) && !defined(TERRAIN_NO_EDGE_FADE)
                    out_color.a *= (1.0 - terrain_edge_fade);
                #endif

                //out_color.xyz = vec3(1.0)
            #endif // DEPTH_ONLY
        #endif // PARTICLE

        /*
        #if !defined(DEPTH_ONLY) && !defined(PLANT)
            //out_color.xyz = vec3(blood_texel.b);

            #if defined(NO_REFLECTION_CAPTURE)
                out_color.xyz = vec3(0.0);
            #else
                out_color.xyz = LookUpReflectionShapes(ref_cap_cubemap, world_vert, normalize(ws_vertex), 0.0);
            #endif
        #endif
        */

        //out_color.xyz = colormap.xyz; // albedo
        //out_color.xyz = vec3(colormap.a); // metalness

    #endif  // ^ end in-scene objects section

    // Note: If you want to hack in some hard-coded setting here,
    // like:
    //
    // out_color.xyz = vec3(1.0);
    //
    // You must wrap it with:
    //
    // #if !defined(DEPTH_ONLY) && defined(YOUR_OBJECT_TYPE)
    // #endif
}
