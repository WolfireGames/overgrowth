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
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;

in vec2 tex;

out vec4 color;

// From http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
//uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
//uniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

//http://dev.theomader.com/gaussian-kernel-calculator/
// sigma 1.8

// 0 1 2 3 4 5 6 7 8
// 0.218818    0.188264    0.119895    0.056512    0.019711    0.005086    0.000971    0.000137    0.000014

// 0 1.39 3.26 5.16 7.09
// 0.218818 0.308159 0.076223 0.006057 0.000151

uniform float offset[5] = float[]( 0, 1.39, 3.26, 5.16, 7.09 );
uniform float weight[5] = float[]( 0.218818, 0.308159, 0.076223, 0.006057, 0.000151);

uniform int screen_height;
uniform int screen_width;
uniform float black_point;
uniform float white_point;
uniform float bloom_mult;
uniform float time_offset;
uniform int horz;

uniform mat4 proj_mat;
uniform mat4 view_mat;
uniform mat4 prev_view_mat;
uniform float time;
uniform float src_lod;
uniform vec3 tint;
uniform vec3 vignette_tint;

uniform float near_blur_amount;
uniform float far_blur_amount;
uniform float near_sharp_dist;
uniform float far_sharp_dist;
uniform float near_blur_transition_size;
uniform float far_blur_transition_size;

uniform float saturation;
uniform float motion_blur_mult;
uniform float brightness;

const float near = 0.1;
const float far = 1000.0;

float DistFromDepth(float depth) {   
    return (near) / (far - depth * (far - near)) * far;
}

vec4 ScreenCoordFromDepth(vec2 tex_uv, vec2 offset, out float distance) {
    float depth = textureLod( tex1, tex_uv + offset, 0.0 ).r;
    distance = DistFromDepth( depth );
    return vec4((tex_uv[0] + offset[0]) * 2.0 - 1.0, 
                (tex_uv[1] + offset[1]) * 2.0 - 1.0, 
                depth * 2.0- 1.0, 
                1.0);
}

vec4 FXAA(sampler2D buf0, vec2 texCoords, vec2 frameBufSize) {
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0/8.0;
    float FXAA_REDUCE_MIN = 1.0/128.0;

    vec3 rgbNW=texture(buf0,texCoords+(vec2(-1.0,-1.0)/frameBufSize)).xyz;
    vec3 rgbNE=texture(buf0,texCoords+(vec2(1.0,-1.0)/frameBufSize)).xyz;
    vec3 rgbSW=texture(buf0,texCoords+(vec2(-1.0,1.0)/frameBufSize)).xyz;
    vec3 rgbSE=texture(buf0,texCoords+(vec2(1.0,1.0)/frameBufSize)).xyz;
    vec3 rgbM=texture(buf0,texCoords).xyz;

    vec3 luma=vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) / frameBufSize;

    vec3 rgbA = (1.0/2.0) * (
        texture(buf0, texCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(buf0, texCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texture(buf0, texCoords.xy + dir * (0.0/3.0 - 0.5)).xyz +
        texture(buf0, texCoords.xy + dir * (3.0/3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax)){
        return vec4(rgbA, 1.0);
    }else{
        return vec4(rgbB, 1.0);
    }
}

//#define VOLCANO

void main(void)
{
#if defined(BLUR_VERT)
    vec4 FragmentColor;
    float pixel_height = 1.0 / float(screen_height);
    FragmentColor = textureLod( tex0, tex, src_lod ) * weight[0];
    for (int i=1; i<5; i++) {
        FragmentColor +=
            textureLod( tex0, tex+vec2(0.0, offset[i]*pixel_height), src_lod )
                * weight[i];
        FragmentColor +=
            textureLod( tex0, tex-vec2(0.0, offset[i]*pixel_height), src_lod )
                * weight[i];
    }
    color = FragmentColor;
#elif defined(BLUR_HORZ)
    vec4 FragmentColor;
    float pixel_width = 1.0 / float(screen_width);
    FragmentColor = textureLod( tex0, tex, src_lod ) * weight[0];
    for (int i=1; i<5; i++) {
        FragmentColor +=
            textureLod( tex0, tex+vec2(offset[i]*pixel_width, 0.0), src_lod )
                * weight[i];
        FragmentColor +=
            textureLod( tex0, tex-vec2(offset[i]*pixel_width, 0.0), src_lod )
                * weight[i];
    }
    color = FragmentColor;
#elif defined(BLUR_DIR)
    if(horz == 1){
        vec4 FragmentColor;
        float pixel_width = 1.0 / float(screen_width);
        FragmentColor = textureLod( tex0, tex, src_lod ) * weight[0];
        for (int i=1; i<5; i++) {
            FragmentColor +=
                textureLod( tex0, tex+vec2(offset[i]*pixel_width, 0.0), src_lod )
                    * weight[i];
            FragmentColor +=
                textureLod( tex0, tex-vec2(offset[i]*pixel_width, 0.0), src_lod )
                    * weight[i];
        }
        color = FragmentColor;
    } else {
        vec4 FragmentColor;
        float pixel_height = 1.0 / float(screen_height);
        FragmentColor = textureLod( tex0, tex, src_lod ) * weight[0];
        for (int i=1; i<5; i++) {
            FragmentColor +=
                textureLod( tex0, tex+vec2(0.0, offset[i]*pixel_height), src_lod )
                    * weight[i];
            FragmentColor +=
                textureLod( tex0, tex-vec2(0.0, offset[i]*pixel_height), src_lod )
                    * weight[i];
        }
        color = FragmentColor;        
    }
#elif defined(COPY)
    color = textureLod( tex0, tex, 0.0 );
#elif defined(DOF)
    #if defined(DOF_LESS)
        float near_blur_amount_scaled = near_blur_amount * 0.25;
        float far_blur_amount_scaled = far_blur_amount * 0.25;
    #else
        float near_blur_amount_scaled = near_blur_amount;
        float far_blur_amount_scaled = far_blur_amount;
    #endif

    color = vec4(0.0);
    const int num_samples = 10;
    vec4 noise = textureLod( tex3, gl_FragCoord.xy / 256.0, 0.0 );
    float angle = noise.x * 6.28;
    float curr_angle = angle;
    float delta_angle = 6.28 / float(num_samples);
    float sample_depth = DistFromDepth(textureLod( tex1, tex, 0.0).r);
    float blur_amount = max(0.0, min(2.0, (pow(sample_depth, 0.5) - far_sharp_dist) / far_blur_transition_size));
    float total = 0.0;
    if(far_blur_amount_scaled > 0.0){
        for(int i=0; i<num_samples; ++i){
            float weight = 1.0;
            vec2 offset = vec2(sin(curr_angle) / screen_width, cos(curr_angle) / screen_height) * 5.0 * blur_amount * far_blur_amount_scaled;
            float sample_depth = DistFromDepth(textureLod( tex1, tex + offset, 0.0).r);
            float temp_blur_amount = max(0.0, min(1.0, (pow(sample_depth, 0.5) - far_sharp_dist) / far_blur_transition_size));
            if(temp_blur_amount < blur_amount){
                weight *= temp_blur_amount + 0.0001;
            }
            color += textureLod( tex0, tex + offset, 0.0) * weight;
            total += weight;
            curr_angle += delta_angle;
        }
        color /= total;
    } else {
        color = textureLod( tex0, tex, 0.0 );
    }

    if(near_blur_amount_scaled > 0.0){
        blur_amount = min(1.0, max(0.0, near_sharp_dist - sample_depth) / near_blur_transition_size);
        vec4 orig_color = color;
        total = 0.0;
        color = vec4(0.0);
        for(int i=0; i<num_samples; ++i){
            vec2 offset = vec2(sin(curr_angle) / screen_width, cos(curr_angle) / screen_height) * mix(0.0, 10.0, pow(near_blur_amount_scaled, 1.0));
            float sample_depth = DistFromDepth(textureLod( tex1, tex + offset, 0.0).r);
            float temp_blur_amount = min(1.0, max(0.0, near_sharp_dist - sample_depth) / near_blur_transition_size);
            float weight = max(blur_amount, temp_blur_amount);
            color += textureLod( tex0, tex + offset, 0.0) * weight;
            total += weight;
            color += orig_color * (1.0 - weight);
            total += (1.0 - weight);
            curr_angle += delta_angle;
        }
        color /= total;
    }
#elif defined(DOWNSAMPLE)
    float pixel_width = 1.0 / float(screen_width);
    float pixel_height = 1.0 / float(screen_height);
    color = textureLod( tex0, tex + vec2( pixel_width,  pixel_height), src_lod) +
           textureLod( tex0, tex + vec2( pixel_width, -pixel_height), src_lod) +
           textureLod( tex0, tex + vec2(-pixel_width,  pixel_height), src_lod) +
           textureLod( tex0, tex + vec2(-pixel_width, -pixel_height), src_lod);
   color *= 0.25;
    #if defined(OVERBRIGHT)
        color = max(vec4(0.0), color - vec4(1.0)) * bloom_mult;
    #endif
#elif defined(DOWNSAMPLE_DEPTH)
    vec2 temp = gl_FragCoord.xy*2.0+vec2(0.5,0.5);
    vec2 dim = vec2(screen_width, screen_height)*2;
    gl_FragDepth = min(              textureLod( tex0, (temp+vec2( 0.0, 0.0))/dim, src_lod).r, 
                                     textureLod( tex0, (temp+vec2(-1.0, 0.0))/dim, src_lod).r);
    gl_FragDepth = min(gl_FragDepth, textureLod( tex0, (temp+vec2(-1.0,-1.0))/dim, src_lod).r);
    gl_FragDepth = min(gl_FragDepth, textureLod( tex0, (temp+vec2( 0.0,-1.0))/dim, src_lod).r);
#elif defined(TONE_MAP)
    //float temp_wp = 0.3;
    //float temp_bp = 0.002;

    float temp_wp = white_point;
    float temp_bp = black_point;
    float contrast = 1.0 / (temp_wp - temp_bp);
    vec4 src_color = textureLod(tex0, tex, 0.0);


    for(int i=0; i<3; ++i){
        src_color[i] = pow(src_color[i], 1.0/2.2);
    }
    
    // Luminosity grayscale method
    float avg = 0.21*src_color[0] + 0.72*src_color[1] + 0.07*src_color[2];
    src_color.xyz = mix(vec3(avg), src_color.xyz, saturation);

    color.w = 1.0;
    color.xyz = max(vec3(0.0), (src_color.xyz - vec3(temp_bp)) * contrast);  
    for(int i=0; i<3; ++i){
        color[i] = pow(color[i], 2.2);
    }

    color.xyz *= tint;

    if(vignette_tint != vec3(1.0)){
        float vignette_amount = 1.0;
        // Vignette
        float vignette = 1.0 - distance(gl_FragCoord.xy, vec2(screen_width*0.5, screen_height*0.5)) / max(screen_width, screen_height);
        float vignette_opac = 1.0 - mix(1.0, pow(vignette, 3.0), vignette_amount);
        color.xyz *= mix(vec3(1.0), vignette_tint, vignette_opac);
    }

    color.a = src_color.a;   
#elif defined(ADD)
    vec4 bloom = mix(textureLod(tex3, tex, 2.0) , textureLod(tex3, tex, 4.0), 0.5);
    color = textureLod(tex2, tex, 0.0) + bloom;
    vec3 overbright = max(vec3(0.0), color.xyz - vec3(1.0));
    float avg = (overbright[0] + overbright[1] + overbright[2]) / 3.0;
    color = 1.0 - max(vec4(0.0), (vec4(1.0) - textureLod(tex2, tex, 0.0))) * max(vec4(0.0), (vec4(1.0) - bloom));
    color.xyz = mix(color.xyz, vec3(1.0), min(1.0,avg*0.3));
    //color.xyz = textureLod(tex2, tex, 0.0).xyz;
#elif defined(CALC_MOTION_BLUR)
    vec3 vel = textureLod( tex2, tex, 0.0).rgb;
    float depth = textureLod( tex1, tex, 0.0).r;
    vec4 noise = textureLod( tex3, gl_FragCoord.xy / 256.0, 0.0 );
    float dist;
    vec4 screen_coord = ScreenCoordFromDepth(tex, vec2(0,0), dist);
    vec4 world_pos = inverse(proj_mat * view_mat) * screen_coord;
    vec4 world_pos2 = inverse(proj_mat * prev_view_mat) * screen_coord;
    world_pos.xyz -= vel * time_offset * world_pos[3];
    color = textureLod( tex0, tex, 0.0 );
    vec3 vel_3d = (view_mat * world_pos - view_mat * world_pos2).xyz;
    vel_3d /= time_offset;
    vel_3d.x += vel_3d.z * screen_coord.x;
    vel_3d.y += vel_3d.z * screen_coord.y;
    vel_3d.z = 0.0;
    vel_3d *= 0.002;
    color.xyz = vel_3d;
#elif defined(APPLY_MOTION_BLUR)
    vec3 vel = texture( tex2, tex).rgb * motion_blur_mult;
    float depth = DistFromDepth(texture( tex1, tex).r);
    vec4 noise = textureLod( tex3, gl_FragCoord.xy / 256.0, 0.0 );
    float dist;
    vec2 dominant_dir = normalize(textureLod( tex2, tex, 4.0).xy);
    if(isnan(dominant_dir[0])){
        dominant_dir = vec2(0.0);
    }
    color = vec4(0.0);
    vec4 base_color = textureLod( tex0, tex, 0.0);
    float total = 0.0001;
    color = base_color * 0.0001;
    const int num_samples = 3;
    float max_blur_dist = 0.01 / float(num_samples) * motion_blur_mult;
    float temp_mult = motion_blur_mult * 1.0;
    float base_speed = dot(textureLod( tex2, tex, 0.0).rg, dominant_dir) * temp_mult;
    for(int i=-num_samples; i<num_samples; ++i){
        float offset_amount;
        offset_amount = max_blur_dist * float(i+noise.r);
        vec2 offset = dominant_dir * offset_amount;
        offset.y *= screen_width/screen_height;
        vec2 coord = tex + offset;
        float weight = 1.0;
        if(coord[0] >= 0.0 && coord[0] <= 1.0 && coord[1] >= 0.0 && coord[1] <= 1.0){
            float sample_speed = dot(textureLod( tex2, coord, 0.0).rg, dominant_dir) * temp_mult;
            /*float sample_depth = DistFromDepth(textureLod( tex1, coord, 0.0).r);
            if(sample_depth < depth + 0.1){
                if(abs(offset_amount) < length(sample_vel)){
                    color += textureLod( tex0, coord, 0.0) * weight;
                } else {
                    color += base_color * weight;
                }
                total += weight;
            }*/
            if(false){

                float sample_depth = DistFromDepth(textureLod( tex1, coord, 0.0).r);
                if(sample_depth < depth - 0.05){
                    if(abs(sample_speed) > abs(offset_amount)){
                        color += textureLod( tex0, coord, 0.0) * weight;
                        total += weight;
                    }  else {
                        color += base_color * weight;
                        total += weight;
                    }
                } else {
                    if(abs(base_speed) > abs(offset_amount)){
                        color += textureLod( tex0, coord, 0.0) * weight;
                        total += weight;
                    }/* else {
                        color += base_color * weight;
                        total += weight;
                    }*/
                }
            }

            if(abs(sample_speed) > abs(offset_amount) || abs(base_speed) > abs(offset_amount)){
                color += textureLod( tex0, coord, 0.0) * weight;
                total += weight;
            } else {
                color += base_color * weight;
                total += weight;
            }
        }
    }
    color /= float(total);

    // Close stationary, background moving
    /*color = vec4(0);
    total = 0.0;
    for(int i=-num_samples; i<num_samples; ++i){
        float offset_amount;
        vec2 coord = tex + vec2(i / float(screen_width), 0.0);
        float weight = 1.0;
        if(coord[0] >= 0.0 && coord[0] <= 1.0 && coord[1] >= 0.0 && coord[1] <= 1.0){
        } else {
            weight *= 0.0001;   
        }
        float sample_depth = DistFromDepth(textureLod( tex1, coord, 0.0).r);
        if(depth > 1.0){
            if(sample_depth > 1.0){
                color += textureLod( tex0, coord, 0.0) * weight;
                total += weight;
            }
        } else {
            color += base_color;
            total += weight;
        }
    }
    color /= float(total);

    // Close moving, background stationary
    color = vec4(0);
    total = 0.0;
    float temp = max(0.01, abs(textureLod( tex2, tex, 0.0).r) * 10000.0);
    for(int i=-num_samples; i<num_samples; ++i){
        float offset_amount;
        vec2 coord = tex + vec2(i / float(screen_width), 0.0);
        float weight = 1.0;
        if(coord[0] >= 0.0 && coord[0] <= 1.0 && coord[1] >= 0.0 && coord[1] <= 1.0){
        } else {
            weight *= 0.0001;   
        }
        float sample_depth = DistFromDepth(textureLod( tex1, coord, 0.0).r);
        if(abs(i) < temp){
            if(depth < 1.0 || sample_depth < 1.0){
                color += textureLod( tex0, coord, 0.0) * weight;
                total += weight;
            } else {
                color += base_color * weight;
                total += weight;
            }
        }
    }
    color /= float(total);*/
    //color.xyz = abs(textureLod( tex2, tex, 0.0).rgb) * 10.0;
#else
#ifdef VOLCANO
    float depth = DistFromDepth(textureLod( tex1, tex, 0.0).r);
    vec2 temp_tex = tex;
    //temp_tex.x += depth * 0.00004 * sin(time*8.0+sin(temp_tex.x*100.0));
    vec3 noise = textureLod( tex3, gl_FragCoord.xy / 256.0 / 8 + vec2(0.0, -time*0.05), 0.0).xyz - vec3(0.5);
    vec3 noise2 = textureLod( tex3, gl_FragCoord.xy / 256.0 / 32 + vec2(0.0, -time*0.01), 0.0).xyz - vec3(0.5);
    vec3 noise3 = textureLod( tex3, gl_FragCoord.xy / 256.0, 0.0).xyz - vec3(0.5);
    #ifdef LESS_SHIMMER
        temp_tex.y += min(depth * 2.0, 30.0) * 0.0001 * (noise.x + noise2.x + noise3.x);//sin(time*8.0+sin(temp_tex.x*100.0));
        temp_tex.x += min(depth * 2.0, 30.0) * 0.00002 * (noise.y + noise2.y + noise2.x);//sin(time*8.0+sin(temp_tex.x*100.0));
    #else
        temp_tex.y += min(depth, 100.0) * 0.0001 * (noise.x + noise2.x + noise3.x);//sin(time*8.0+sin(temp_tex.x*100.0));
        temp_tex.x += min(depth, 100.0) * 0.00002 * (noise.y + noise2.y + noise2.x);//sin(time*8.0+sin(temp_tex.x*100.0));
    #endif
    color = textureLod( tex0, temp_tex, 0.0 );
#else
    color = textureLod( tex0, tex, 0.0 );
#endif
#ifdef BRIGHTNESS
    for(int i=0; i<3; ++i){
        color[i] = pow(color[i], 1.7 - brightness * 0.7);
#ifdef GAMMA_CORRECT_OUTPUT
        color[i] = pow(color[i], 1/2.2);
#endif
    }
    
#endif
    // 0.2 to 1.8
    //vec2 buf = vec2(screen_width, screen_height);
    //color = FXAA(tex0, tex, buf);
#endif
}
