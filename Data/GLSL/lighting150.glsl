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
#if !defined(LIGHTING_GLSL)
#define LIGHTING_GLSL

uniform vec4 primary_light_color;

float rand(vec2 co){
    return fract(sin(dot(vec2(floor(co.x),floor(co.y)) ,vec2(12.9898,78.233))) * 43758.5453);
}

float GetDirectContribSimple( float amount ) {
    return amount * primary_light_color.a;
}

float GetDirectContrib( const vec3 light_pos,
                        const vec3 normal,
                        const float unshadowed ) {
    float direct_contrib;
    direct_contrib = max(0.0,dot(light_pos, normal));
    direct_contrib *= unshadowed;

    return GetDirectContribSimple(direct_contrib);
}

float GetDirectContribSoft( const vec3 light_pos,
                            const vec3 normal,
                            const float unshadowed ) {
    float direct_contrib;
    direct_contrib = max(0.0,dot(light_pos, normal)*0.5+0.5);
    direct_contrib *= unshadowed;

    return GetDirectContribSimple(direct_contrib);
}

/*
void SetCascadeShadowCoords(vec4 vert, mat5 inout vec4 sc[4]) {
    sc[0] = gl_TextureMatrix[0] * gl_ModelViewMatrix * vert;
    sc[1] = gl_TextureMatrix[1] * gl_ModelViewMatrix * vert;
    sc[2] = gl_TextureMatrix[2] * gl_ModelViewMatrix * vert;
    sc[3] = gl_TextureMatrix[3] * gl_ModelViewMatrix * vert;
}*/

const vec2 poisson_disc_4[4]=vec2[4](
    vec2(-0.4215465, -0.3005172),
    vec2(0.3845127, 0.1628611),
    vec2(-0.03371959, 0.46099),
    vec2(0.1679813, -0.5491683)
);

const vec2 poisson_disc_9[9] = vec2[9](
    vec2(0.5465816, -0.02595456),
    vec2(0.5313139, -0.5830789),
    vec2(0.5950379, 0.5558487),
    vec2(-0.2362069, -0.4548543),
    vec2(0.134974, 0.8385499),
    vec2(0.02610943, 0.05580516),
    vec2(-0.6288149, -0.1155209),
    vec2(-0.005609761, -0.9341435),
    vec2(-0.7329739, 0.5158124)
);

const vec2 poisson_disc_16[16] = vec2[16](
    vec2(0.4989825, -0.4092478),
    vec2(-0.09638281, -0.7609485),
    vec2(0.679416, -0.7299017),
    vec2(0.05394743, -0.3816648),
    vec2(0.2655547, -0.951362),
    vec2(-0.4713327, -0.186854),
    vec2(0.7385173, 0.1464785),
    vec2(0.2628677, 0.1818207),
    vec2(-0.04351781, -0.02468965),
    vec2(0.4662065, 0.6578184),
    vec2(-0.1140366, 0.6669924),
    vec2(-0.4714432, -0.7921887),
    vec2(-0.6617869, 0.1933294),
    vec2(-0.8934375, -0.1875305),
    vec2(-0.6286579, 0.5908005),
    vec2(-0.2781906, 0.3561186)
);

#if defined(SIMPLE_SHADOW)
    #define FOUR_TAP_STATIC
#else
    #define FOUR_TAP
#endif

#if defined(FRAGMENT_SHADER)

float GetCascadeShadowIndex(sampler2DShadow tex5, vec4 shadow_coord, int index, float rand_a, float slope_dot) {
    //vec3 normal=normalize(cross(X,Y));

    #if !defined(SIMPLE_SHADOW)
        shadow_coord.z -= 0.00003 * pow(2.0, float(index)*1.7);
    #else
        #if defined(CHARACTER)
            shadow_coord.z -= 0.001;
        #else
            shadow_coord.z -= (1.0-abs(slope_dot))*0.0009 + 0.0001;
        #endif
    #endif

    #if !defined(SIMPLE_SHADOW)
        shadow_coord.x *= 0.5;
        shadow_coord.y *= 0.5;

        if(index == 1){
            shadow_coord.x += 0.5;
        }

        if(index == 2){
            shadow_coord.y += 0.5;
        }

        if(index == 3){
            shadow_coord.xy += vec2(0.5);
        }
    #endif

    if(shadow_coord.x >= 1.0 ||
       shadow_coord.y >= 1.0 ||
       shadow_coord.x <= 0.0 ||
       shadow_coord.y <= 0.0)
    {
        return 1.0;
    }

    float shadow_amount = 0.0;
    float offset = 1.5/2048.0 * mix(0.5,1.5,1.0 - primary_light_color.a / 3.0);
    float offset_angle = rand_a * 6.28;
    float sin_angle = sin(offset_angle);
    float cos_angle = cos(offset_angle);
    mat2 offset_rot;
    offset_rot[0] = vec2(cos_angle, sin_angle);
    offset_rot[1] = vec2(-sin_angle, cos_angle);

    #if defined(ONE_TAP)
        shadow_amount += textureProj(tex5,shadow_coord);
    #endif

    #if defined(ONE_TAP_Z_INTERP)
        float val = 10000.0;
        float temp_min = floor(shadow_coord.z * val) / val;
        float temp_max = ceil(shadow_coord.z * val) / val;
        float alpha = (shadow_coord.z - temp_min) / (temp_max - temp_min);
        alpha = max(0.0, min(1.0, alpha));
        shadow_coord.z = temp_max;
        shadow_amount += textureProj(tex5,shadow_coord) * alpha;
        shadow_coord.z = temp_min;
        shadow_amount += textureProj(tex5,shadow_coord) * (1.0 - alpha);
    #endif

    #if defined(FOUR_TAP_STATIC)
        float temp = 0.25/2048.0;
        shadow_amount += textureProj(tex5,shadow_coord+vec4(temp, 0.0, 0.0,0.0));
        shadow_amount += textureProj(tex5,shadow_coord+vec4(0.0, temp, 0.0,0.0));
        shadow_amount += textureProj(tex5,shadow_coord+vec4(-temp, 0.0, 0.0,0.0));
        shadow_amount += textureProj(tex5,shadow_coord+vec4(0.0, -temp, 0.0,0.0));
        shadow_amount /= 4.0;
    #endif

    #if defined(FOUR_TAP)
        for(int i=0; i<4; ++i){
            vec2 offset_vec = offset_rot * poisson_disc_4[i];
            offset_vec *= offset;
            shadow_amount += textureProj(tex5,shadow_coord+vec4(offset_vec.x, offset_vec.y, 0.0,0.0));
        }

        shadow_amount /= 4.0;
    #endif

    #if defined(NINE_TAP)
        for(int i=0; i<9; ++i){
            vec2 offset_vec = offset_rot * poisson_disc_9[i];
            offset_vec *= offset;
            shadow_amount += textureProj(tex5,shadow_coord+vec4(offset_vec.x, offset_vec.y, 0.0,0.0));
        }

        shadow_amount /= 9.0;
    #endif

    #if defined(SIXTEEN_TAP)
        for(int i=0; i<16; ++i){
            vec2 offset_vec = offset_rot * poisson_disc_16[i];
            offset_vec *= offset;
            shadow_amount += textureProj(tex5,shadow_coord+vec4(offset_vec.x, offset_vec.y, 0.0,0.0));
        }

        shadow_amount /= 16.0;
    #endif

    return shadow_amount;
}

float GetCascadeShadow(sampler2DShadow tex5, vec4 sc[4], float dist, float slope_dot){
    float rand_a = rand(gl_FragCoord.xy);
    vec3 shadow_tex = vec3(1.0);
    int index = 4;

    if(max(length(sc[0].xy-vec2(0.5)), dist * 0.02) < 0.49 - rand_a * 0.05){
        index = 0;
    } else if(max(length(sc[1].xy-vec2(0.5)), dist * 0.011) < 0.49 - rand_a * 0.05){
        index = 1;
    } else if(length(sc[2].xy-vec2(0.5)) < 0.49 - rand_a * 0.05){
        index = 2;
    } else if(length(sc[3].xy-vec2(0.5)) < 0.49 - rand_a * 0.05){
        index = 3;
    }

    #if defined(SIMPLE_SHADOW)
        index = 3;
    #endif

    if(index == 4){
        return 1.0;
    }

    return GetCascadeShadowIndex(tex5, sc[index], index, rand_a, slope_dot);
}

float GetCascadeShadow(sampler2DShadow tex5, vec4 sc[4], float dist){
    return GetCascadeShadow(tex5, sc, dist, 0.0);
}

#endif  // ^ defined(FRAGMENT_SHADER)


vec3 CalcVertexOffset (const vec3 world_pos, float wind_amount, float time, float plant_shake) {
    vec3 vertex_offset = vec3(0.0);

    float wind_shake_amount = 0.02;
    float wind_time_scale = 8.0;
    float wind_shake_detail = 6.0;
    float wind_shake_offset = (world_pos.x+world_pos.y)*wind_shake_detail;
    wind_shake_amount *= max(0.0,sin((world_pos.x+world_pos.y)+time*0.3));
    wind_shake_amount *= sin((world_pos.x*0.1+world_pos.z)*0.3+time*0.6)+1.0;
    wind_shake_amount = max(0.002,wind_shake_amount);
    wind_shake_amount += plant_shake;
    wind_shake_amount *= wind_amount;

    vertex_offset.x += sin(time*wind_time_scale+wind_shake_offset);
    vertex_offset.z += cos(time*wind_time_scale*1.2+wind_shake_offset);
    vertex_offset.y += cos(time*wind_time_scale*1.4+wind_shake_offset);

    vertex_offset *= wind_shake_amount;

    return vertex_offset;
}

vec3 UnpackObjNormal(const vec4 normalmap) {
    return normalize(vec3(2.0,2.0,0.0-2.0)*normalmap.xzy + vec3(0.0-1.0,0.0-1.0,1.0));
    /*x = 2.0 * nm.x - 1.0
    y = 2.0 * nm.z - 1.0
    z = -2.0 * nm.y + 1.0

    nm.x = 0.5 * x + 0.5
    nm.y = -0.5 * z + 0.5
    nm.z = 0.5 * y + 0.5*/
}

vec3 UnpackObjNormalV3(const vec3 normalmap) {
    return normalize(vec3(2.0,2.0,0.0-2.0)*normalmap.xzy + vec3(0.0-1.0,0.0-1.0,1.0));
}

vec3 PackObjNormal(const vec3 normal) {
    return vec3(0.5,0.0-0.5,0.5)*normal.xzy + vec3(0.5,0.5,0.5);
}

vec3 UnpackTanNormal(const vec4 normalmap) {
    return normalize(vec3(vec2(2.0,0.0-2.0)*normalmap.xy + vec2(0.0-1.0,1.0),normalmap.z));
}

vec3 GetDirectColor(const float intensity) {
    return primary_light_color.xyz * intensity;
}

vec3 LookupCubemap(const mat3 obj2world_mat3,
                   const vec3 vec,
                   const samplerCube cube_map) {
    vec3 world_space_vec = obj2world_mat3 * vec;
    return texture(cube_map,world_space_vec).xyz;
}

vec3 LookupCubemapMat4(const mat4 obj2world,
                   const vec3 vec,
                   const samplerCube cube_map) {
    vec3 world_space_vec = (obj2world * vec4(vec,0.0)).xyz;
    return texture(cube_map,world_space_vec).xyz;
}

vec3 LookupCubemapSimple(const vec3 vec,
                   const samplerCube cube_map) {
    vec3 world_space_vec = vec;
    return texture(cube_map,world_space_vec).xyz;
}

vec3 LookupCubemapSimpleLod(const vec3 vec,
                   const samplerCube cube_map, float lod) {
    vec3 world_space_vec = vec;
    return textureLod(cube_map,world_space_vec,lod).xyz;
}

float GetAmbientMultiplier() {
    return (1.5-primary_light_color.a*0.5);
}

float GetAmbientMultiplierScaled() {
    return GetAmbientMultiplier()/1.5;
}

float GetAmbientContrib (const float unshadowed) {
    float contrib = min(1.0,max(unshadowed * 1.5, 0.5));
    contrib *= GetAmbientMultiplier();
    return contrib;
}

float GetSpecContrib ( const vec3 light_pos,
                       const vec3 normal,
                       const vec3 vertex_pos,
                       const float unshadowed ) {
    vec3 H = normalize(normalize(vertex_pos*(0.0-1.0)) + normalize(light_pos));
    return min(1.0, pow(max(0.0,dot(normal,H)),10.0)*1.0)*unshadowed*primary_light_color.a;
}

float GetSpecContrib ( const vec3 light_pos,
                       const vec3 normal,
                       const vec3 vertex_pos,
                       const float unshadowed,
                       const float pow_val) {
    vec3 H = normalize(normalize(vertex_pos*(0.0-1.0)) + normalize(light_pos));
    return min(1.0, pow(max(0.0,dot(normal,H)),pow_val)*1.0)*unshadowed*primary_light_color.a;
}

float GetHazeAmount( in vec3 relative_position ) {
    #if defined(DISABLE_FOG)
        return 0.0f;
    #endif

    float haze_mult = 0.0008;
    float fog_opac = (1.0 - (1.0 / pow(length(relative_position) * haze_mult + 1.0, 2.0)));

    return fog_opac;
}

#if defined(FRAGMENT_SHADER) && !defined(DEPTH_ONLY)

#if defined(MISTY) || defined(MISTY2) || defined(SKY_ARK) || defined(DAMP_FOG) || defined(WATERFALL_ARENA)
    float noise_3d( in vec3 x )
    {
        vec3 p = floor(x);
        vec3 f = fract(x);
        f = f*f*(3.0-2.0*f);

        vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
        vec2 rg;
        rg[0] = noise(uv);
        rg[1] = noise(uv + vec2(37.0,17.0));

        return mix( rg.x, rg.y, f.z );
    }

    float fractal_3d(in vec3 x){
        vec3 p = floor(x);
        vec3 f = fract(x);
        f = f*f*(3.0-2.0*f);

        vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
        vec2 rg;
        rg[0] = fractal(uv);
        rg[1] = fractal(uv + vec2(37.0,17.0));

        return mix( rg.x, rg.y, f.z );
    }
#endif

float GetHazeAmount( in vec3 relative_position, float haze_mult) {
    #if defined(DISABLE_FOG)
        return 0.0f;
    #endif

    /*float near = 0.1;
    float far = 100.0;
    float fog_opac = min(1.0,length(relative_position)/far);
    return fog_opac;*/

    float fog_opac = (1.0 - (1.0 / pow(length(relative_position) * haze_mult + 1.0, 2.0)));

    #if !defined(SIMPLE_FOG)
        #if defined(MISTY) || defined(MISTY2) || defined(DAMP_FOG) || defined(SKY_ARK) || defined(WATERFALL_ARENA)
            #if defined(MISTY)
                float fog_height = 95.0;
            #elif defined(MISTY2)
                float fog_height = 18.0;
            #elif defined(WATERFALL_ARENA)
                float fog_height = 90.5;
            #elif defined(DAMP_FOG)
                float fog_height = -44.0;
            #elif defined(SKY_ARK)
                float fog_height = min(75, 80.0 + (cam_pos+relative_position).z*-0.05);
            #endif

            {
                vec3 dir = normalize(relative_position);
                float second_layer_opac = 1.0;//pow(min(1.0, dist * 0.025), 0.9);
                float first_layer_opac = 1.0;//pow(min(1.0, dist * 0.05) - second_layer_opac * 0.5, 0.9);
                //haze_amount *= 1.0 + sin(dir.y * 10.0 + time * 32.0 + cos(dir.x * 10.0)+ cos(dir.z * 7.0)) * 0.05 * first_layer_opac;
                float angle = atan(dir.z, -dir.x);
                float fade = 1.0 - dir.y * dir.y;

                float speed = .4;
                float active_time = time*speed;

                float old_fog_opac = 0.5 + fractal(vec2(angle * 1.5-active_time*0.2, dir.y * 1.2 - active_time * 0.3))*0.5;
                old_fog_opac += 0.5 + fractal(vec2(angle * 2.5+active_time*0.2, dir.y * 2.2 + active_time * 0.3))*0.5;
                float val = abs(angle-3.14) * 5.0;

                if(val < 1.0){
                    float temp_angle = angle - 6.28;
                    float new_fog_opac = 0.5 + fractal(vec2(temp_angle * 1.5-active_time*0.2, dir.y * 1.2 - active_time * 0.3))*0.5;
                    new_fog_opac += 0.5 + fractal(vec2(temp_angle * 2.5+active_time*0.2, dir.y * 2.2 + active_time * 0.3))*0.5;
                    val = mix(new_fog_opac, old_fog_opac, val);
                } else {
                    val = old_fog_opac;
                }

                val *= 0.5;
                val = mix(0.5, val, fade);

                fog_height += val - 0.5;
                vec3 world_vert = (relative_position+cam_pos);

                #if defined(MISTY2) || defined(SKY_ARK) || defined(DAMP_FOG) || defined(WATERFALL_ARENA)
                    float base_fog_opac = fog_opac * pow(min(1.0, 1.0 - normalize(relative_position).y), 2.0) * val * 1.5;
                #elif defined(MISTY)
                    float base_fog_opac = 0.0;
                #endif

                if(cam_pos.y > fog_height && world_vert.y > fog_height){
                    return base_fog_opac;
                }

                vec3 temp_cam_pos = cam_pos;

                if(world_vert.y > fog_height){
                    float amount = (world_vert.y - fog_height) / (relative_position.y);
                    relative_position = relative_position * (1.0 - amount);
                } else if(cam_pos.y > fog_height){
                    float amount = (fog_height - world_vert.y) / (-relative_position.y);
                    relative_position = relative_position * amount;
                    temp_cam_pos = world_vert - relative_position;
                }

                float dist = length(relative_position);
                float orig_fog_opac = (1.0 - (1.0 / pow(dist * haze_mult + 1.0, 2.0)));

                dist *= 0.5;
                temp_cam_pos *= 0.5;
                temp_cam_pos.x += active_time;
                temp_cam_pos.y += val * 8.0;
                fog_opac = 0.0;
                fog_opac += noise_3d(temp_cam_pos+dir)*0.1*min(1.0, max(0.0, dist));
                fog_opac += noise_3d(temp_cam_pos+dir*2.0)*0.1*min(1.0, max(0.0, dist-1.0));
                fog_opac += noise_3d(temp_cam_pos+dir*3.0)*0.1*min(1.0, max(0.0, (dist-2.0)*0.5));
                fog_opac += noise_3d(temp_cam_pos+dir*4.0)*0.1*min(1.0, max(0.0, (dist-4.0)*0.25));

                #if defined(SKY_ARK) // fade out noise as camera height increases
                    if(cam_pos.y > fog_height){
                        fog_opac *= 1.0 / ((cam_pos.y - fog_height)*0.2 + 1.0);
                    }
                #endif

                //fog_opac *= min(1.0, max(0.0, 8/dist));
                #if defined(MISTY) || defined(DAMP_FOG) || defined(WATERFALL_ARENA)
                    fog_opac = orig_fog_opac + fog_opac * 0.3;
                #elif defined(MISTY2) || defined(SKY_ARK)
                    fog_opac = orig_fog_opac + fog_opac;
                #endif

                fog_opac = fog_opac + base_fog_opac;
                fog_opac = min(1.0, max(0.0, fog_opac));
            }
        #endif

        #if defined(RAINY)
        {
            float dist = length(relative_position)*0.1;
            vec3 dir = normalize(relative_position);
            float second_layer_opac = pow(min(1.0, dist * 0.025), 0.9);
            float first_layer_opac = pow(min(1.0, dist * 0.05) - second_layer_opac * 0.5, 0.9);
            //haze_amount *= 1.0 + sin(dir.y * 10.0 + time * 32.0 + cos(dir.x * 10.0)+ cos(dir.z * 7.0)) * 0.05 * first_layer_opac;
            float angle = atan(dir.z, -dir.x);
            float fade = 1.0 - dir.y * dir.y;
            float old_fog_opac = 1.0 - ((1.0 - fog_opac) * (1.0 + fractal(vec2(angle * 100, dir.y * 5.0 + time * 16.0)) * 0.2 * first_layer_opac * fade));
            old_fog_opac = 1.0 - ((1.0 - old_fog_opac) * (1.0 + fractal(vec2(angle * 2, dir.y + time * 2.0)) * 0.8 * second_layer_opac * fade));
            float val = abs(angle-3.14) * 5.0;

            if(val < 1.0){
                float temp_angle = angle - 6.28;
                float new_fog_opac = 1.0 - ((1.0 - fog_opac) * (1.0 + fractal(vec2(temp_angle * 100, dir.y * 5.0 + time * 16.0)) * 0.2 * first_layer_opac * fade));
                new_fog_opac = 1.0 - ((1.0 - new_fog_opac) * (1.0 + fractal(vec2(temp_angle * 2, dir.y + time * 2.0)) * 0.8 * second_layer_opac * fade));
                fog_opac = mix(new_fog_opac, old_fog_opac, val);
            } else {
                fog_opac = old_fog_opac;
            }

            fog_opac = min(1.0, max(0.0, fog_opac));
        }
        #endif

    #endif  // ^ !defined(SIMPLE_FOG)

    /*#if defined(VOLCANO)
    {
        float dist = length(relative_position);
        vec3 dir = normalize(relative_position);
        float second_layer_opac = pow(min(1.0, dist * 0.025), 0.9);
        float first_layer_opac = pow(min(1.0, dist * 0.05) - second_layer_opac * 0.5, 0.9);
        //haze_amount *= 1.0 + sin(dir.y * 10.0 + time * 32.0 + cos(dir.x * 10.0)+ cos(dir.z * 7.0)) * 0.05 * first_layer_opac;
        float angle = atan(dir.z, -dir.x);
        float fade = 1.0 - dir.y * dir.y;
        float old_fog_opac = 1.0 - ((1.0 - fog_opac) * (1.0 + fractal(vec2(angle * 3, dir.y * 1.2 - time * 0.3)) * 0.2 * first_layer_opac * fade));
        old_fog_opac = 1.0 - ((1.0 - old_fog_opac) * (1.0 + fractal(vec2(angle * 2, dir.y - time * 0.2)) * 0.8 * second_layer_opac * fade));
        float val = abs(angle-3.14) * 5.0;

        if(val < 1.0){
            float temp_angle = angle - 6.28;
            float new_fog_opac = 1.0 - ((1.0 - fog_opac) * (1.0 + fractal(vec2(temp_angle * 3, dir.y * 1.2 - time * 0.3)) * 0.2 * first_layer_opac * fade));
            new_fog_opac = 1.0 - ((1.0 - new_fog_opac) * (1.0 + fractal(vec2(temp_angle * 2, dir.y - time * 0.2)) * 0.8 * second_layer_opac * fade));
            fog_opac = mix(new_fog_opac, old_fog_opac, val);
        } else {
            fog_opac = old_fog_opac;
        }

        fog_opac = min(1.0, max(0.0, fog_opac));
    }
    #endif*/

    #if defined(WATER) && defined(WATER_HORIZON)
        #if defined(ALT)
            fog_opac = min(1.0, max(fog_opac, length((cam_pos+relative_position).xz-vec2(94,144))/1000.0));
        #elif defined(ALT2)
            fog_opac = min(1.0, max(fog_opac, length((cam_pos+relative_position).xz)/600.0));
        #else
            fog_opac = min(1.0, max(fog_opac, length((cam_pos+relative_position).xz)/200.0));
        #endif
    #endif

    return fog_opac;
}

#endif  // ^ defined(FRAGMENT_SHADER) && !defined(DEPTH_ONLY)


void AddHaze( inout vec3 color,
              in vec3 relative_position,
              in samplerCube fog_cube ) {
    vec3 fog_color = textureLod(fog_cube,relative_position,5.0).xyz;
    color = mix(color, fog_color, GetHazeAmount(relative_position));
}

float GammaCorrectFloat(in float val) {
    #if defined(GAMMA_CORRECT)
        return pow(val,2.2);
    #else
        return val;
    #endif
}

vec3 GammaCorrectVec3(in vec3 val) {
    #if defined(GAMMA_CORRECT)
        return vec3(pow(val.r,2.2),pow(val.g,2.2),pow(val.b,2.2));
    #else
        return val;
    #endif
}

vec3 GammaAntiCorrectVec3(in vec3 val) {
    #if defined(GAMMA_CORRECT)
        return vec3(pow(val.r,1.0/2.2),pow(val.g,1.0/2.2),pow(val.b,1.0/2.2));
    #else
        return val;
    #endif
}

void ReadBloodTex(in sampler2D tex, in vec2 tex_coords, out float blood_amount, out float wetblood){
    vec4 blood_tex = texture(tex,tex_coords);
    blood_amount = min(blood_tex.r*5.0, 1.0);
    wetblood = max(0.0,blood_tex.g*1.4-0.4);
}

void ApplyBloodToColorMap(inout vec4 colormap, in float blood_amount, in float temp_wetblood, in vec3 blood_tint_color){
    float wetblood = mix(temp_wetblood, 0.4, colormap.a);
    vec4 old_blood = vec4(blood_tint_color * (0.8*wetblood+0.2), wetblood);
    vec4 new_blood = vec4(colormap.xyz * blood_tint_color, wetblood*0.5+0.5);
    vec4 blood_color = mix(old_blood, new_blood, (1.0-wetblood)*0.5);
    colormap = mix(colormap, blood_color, blood_amount);
}

#endif  // ^ !defined(LIGHTING_GLSL)
