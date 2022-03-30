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

#if !defined NO_DECALS

#define decal_normal_tex tex9
#define decal_color_tex tex10


//Disabled because we've run out of texture sampler.
#if defined(DECAL_NORMALS)
uniform sampler2D decal_normal_tex; // decal normal texture
#endif  // DECAL_NORMALS
uniform sampler2D decal_color_tex; // decal color texture


// this MUST match the one in source or bad things happen
struct DecalData {
    vec3 scale;
    float spawn_time;
    vec4 rotation;  // quaternion
    vec3 position;
    float pad1;

    vec4 tint;
    vec4 uv;
    vec4 normal;
};

#define DECAL_SIZE_VEC4 6u


#if !defined(DEPTH_ONLY)
DecalData FetchDecal(uint decal_index) {
    DecalData decal;

    vec4 temp        = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 0u));
    decal.scale      = temp.xyz;
    decal.spawn_time = temp.w;
    decal.rotation   = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 1u));
    decal.position   = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 2u)).xyz;
    decal.pad1       = 0.0f;

    decal.tint = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 3u));

    decal.uv = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 4u));

    decal.normal = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 5u));

    return decal;
}
#endif


mat3 mat_from_quat(vec4 q) {
    float qxx = q.x * q.x;
    float qyy = q.y * q.y;
    float qzz = q.z * q.z;
    float qxz = q.x * q.z;
    float qxy = q.x * q.y;
    float qyz = q.y * q.z;
    float qwx = q.w * q.x;
    float qwy = q.w * q.y;
    float qwz = q.w * q.z;

    mat3 m;
    m[0][0] = 1.0f - 2.0f * (qyy +  qzz);
    m[0][1] = 2.0f * (qxy + qwz);
    m[0][2] = 2.0f * (qxz - qwy);

    m[1][0] = 2.0f * (qxy - qwz);
    m[1][1] = 1.0f - 2.0f * (qxx +  qzz);
    m[1][2] = 2.0f * (qyz + qwx);

    m[2][0] = 2.0f * (qxz + qwy);
    m[2][1] = 2.0f * (qyz - qwx);
    m[2][2] = 1.0f - 2.0f * (qxx +  qyy);

    return m;
}

#if !defined(DEPTH_ONLY)

// https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float sdRoundBox(vec3 p, vec3 b, float r) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r;
}

void CalculateDecals(inout vec4 colormap, inout vec3 ws_normal, inout float spec_amount, inout float roughness, inout float preserve_wetness, inout float ambient_mult, inout float env_ambient_mult, inout vec3 diffuse_color, in vec3 world_vert, float time, uint decal_val,
    inout vec3 flame_final_color, inout float flame_final_contrib) {
    // number of decals in current cluster
    uint decal_count = (decal_val >> COUNT_BITS) & COUNT_MASK;

    //colormap.xyz = vec3(decal_count) / 16.0;
    //return;

    // debug option, uncomment to visualize clusters
    //colormap.xyz = vec3(min(decal_count, 63u) / 63.0);
    //colormap.xyz = vec3(g.z / grid_size.z);

    // index into cluster_decals
    uint first_decal_index = decal_val & INDEX_MASK;

    // decal list data is immediately after cluster lookup data
    uint num_clusters = grid_size.x * grid_size.y * grid_size.z;
    first_decal_index = first_decal_index + 2u * num_clusters;

    vec3 world_dx = dFdx(world_vert);
    vec3 world_dy = dFdy(world_vert);
    for (uint i = 0u; i < decal_count; ++i) {
        // texelFetch takes int
        uint decal_index = texelFetch(cluster_buffer, int(first_decal_index + i)).x;

        DecalData decal = FetchDecal(decal_index);
        float spawn_time = decal.spawn_time;

        vec2 start_uv = decal.uv.xy;
        vec2 size_uv = decal.uv.zw;

        vec2 start_normal = decal.normal.xy;
        vec2 size_normal = decal.normal.zw;

        // We omit scale component because we want to keep normal unit length
        // We omit translation component because normal
        mat3 rotation_mat = mat_from_quat(decal.rotation);
        vec3 decal_ws_normal = rotation_mat * vec3(0.0, 1.0, 0.0);

        vec3 inv_scale = vec3(1.0f) / decal.scale;
        mat3 inv_rotation_mat = transpose(rotation_mat);
        vec3 temp = (inv_rotation_mat * (world_vert - decal.position)) * inv_scale;
        if(abs(temp[0]) < 0.5 && abs(temp[1]) < 0.5 && abs(temp[2]) < 0.5){
            bool ambient_shadow = false;
            int type = int(decal.tint[3]);
            bool skip = false;
            switch (type) {
                case decalShadow: {
                    #if defined(CHARACTER) || defined(ITEM)
                    #else
                        float mult = max(0.5, min(1.0, pow(length(temp*2.0), 1.0)));
                        ambient_mult *= mult;
                    #endif
                        skip = true;
                    break;}
                case decalEnvShadow:{
                    float mult = mix((max(0.5, min(1.0, pow(length(temp*2.0), 1.0))) - 0.5) * 2.0, 1.0, decal.tint[0]);
                    env_ambient_mult *= mult;
                    skip = true;
                    break;}
                case decalEnvShadowSquare:{
                    float mult = mix((clamp(sdRoundBox(temp * 4.0, vec3(0.875), 0.125), 0.5, 1.0) - 0.5) * 2.0, 1.0, decal.tint[0]);
                    env_ambient_mult *= mult;
                    skip = true;
                    break;}
                case decalUnderwater:{
                    #if defined(PARTICLE)
                        ws_normal = vec3(-1.0);
                    #endif
                    skip = true;
                    break;}
                case decalSpongeSquare:{
                    preserve_wetness *= 1.0 - decal.tint[0];
                    skip = true;
                    break;}
                case decalSpongeRound:{
                    float mult = mix((max(0.5, min(1.0, pow(length(temp*2.0), 8.0))) - 0.5) * 2.0, 1.0, 1.0 - decal.tint[0]);
                    preserve_wetness *= mult;
                    skip = true;
                    break;}
#if defined(AMBIENT_LIGHT_DECALS)
                case decalLightSquare:{
                    float mult = 1.0 - (clamp(sdRoundBox(temp * 4.0, vec3(0.875), 0.125), 0.5, 1.0));
                    diffuse_color += ambient_mult * decal.tint.rgb * mult;
                    skip = true;
                    break;}
                case decalLightRound:{
                    float mult = 1.0 - (2.0 * (clamp(length(temp * 2.0), 0.5, 1.0) - 0.5));
                    diffuse_color += ambient_mult * decal.tint.rgb * mult;
                    skip = true;
                    break;}
#endif
            #if defined(WATER)
                case decalWaterFroth: {
                    #if defined(DIRECTED_WATER_DECALS)
                        float mult = 1.0 - (max(0.5, min(1.0, pow(length(temp*2.0), 1.0))) - 0.5) * 2.0;
                        mult *= decal.tint[0];

                        vec2 color_tex_coord = vec2(temp.x * decal.scale.x * 0.1 + time * decal.tint[1], temp.z * decal.scale.z * 0.1);
                        vec4 decal_color = texture(tex1, color_tex_coord);
                        colormap.xyz = mix(colormap.xyz, decal_color.xyz * 0.1, mult);
                    #else
                        const float fade_time = 2.0;
                        float opac = max(decal.tint[0], min(1.0, (time - spawn_time)*3.0) * max(0.0, (spawn_time - time + fade_time)/fade_time));
                        float mult = mix((max(0.5, min(1.0, pow(length(temp*2.0), 1.0))) - 0.5) * 2.0, 1.0, 1.0-opac);
                        roughness = max(1.0-mult, roughness);
                    #endif
                    skip = true;}
                    break;
            #endif
            #if defined(CHARACTER) && defined(CHARACTER_DECALS)
                case decalCharacter:
                    break;
            #elif defined(CHARACTER) || defined(ITEM) || defined(PARTICLE)
            #else
                case decalDefault:
                case decalBlood:
                case decalWatersplat:
                case decalFire:
                    break;
            #endif
            default:
                skip = true;
                break;
            }
            if(!skip){
                // we must supply texture gradients here since we have non-uniform control flow
                // non-uniformness happens when we have z cluster discontinuities

                vec2 color_tex_dx = ((inv_rotation_mat * world_dx) * inv_scale).xz * size_uv;
                vec2 color_tex_dy = ((inv_rotation_mat * world_dy) * inv_scale).xz * size_uv;

                vec2 color_tex_coord = start_uv + size_uv * vec2(temp.x + 0.5, 1.0 - (temp.z + 0.5));
                vec4 decal_color = textureGrad(decal_color_tex, color_tex_coord, color_tex_dx, color_tex_dy);

        #if defined(DECAL_NORMALS)

                vec2 normal_tex_coord = start_normal + size_normal * (temp.xz + vec2(0.5));

                vec2 normal_tex_dx = ((inv_rotation_mat * world_dx) * inv_scale).xz * size_normal;
                vec2 normal_tex_dy = ((inv_rotation_mat * world_dy) * inv_scale).xz * size_normal;

                vec4 decal_normal = textureGrad(decal_normal_tex, normal_tex_coord, normal_tex_dx, normal_tex_dy);

        #endif  // DECAL_NORMALS
                float decal_normal_dot = dot(decal_ws_normal.xyz,(ws_normal.xyz*2.0)/1.0);
                if( decal_normal_dot > 0.80 )
                {
                    float submix = 1.0;

                    if( decal_normal_dot < 0.85 )
                    {
                        submix = (decal_normal_dot - 0.80)*(1.0/0.05);
                    }

                    if(type == decalBlood){ // Decal is blood
                        float wetness_lifetime = 10.0;
                        float wetness = max(0.0, (spawn_time - time + wetness_lifetime)/wetness_lifetime);
                        spec_amount = mix(spec_amount, 0.05, decal_color.a * submix * mix(0.1, 0.3, wetness));
                        roughness = mix(roughness, 0.0, decal_color.a * submix * mix(0.5, 1.0, wetness));
                        decal_color.xyz *= mix(0.15, 0.4, wetness);
                    } else if(type == decalWatersplat){ // Decal is water
                        float water_drop_lifetime = 2.0;
                        decal_color.a *= pow(max(0.0, (spawn_time - time + water_drop_lifetime)/water_drop_lifetime), 0.5);
                        spec_amount = mix(spec_amount, 0.05, decal_color.a * submix);
                        roughness = mix(roughness, 0.0, decal_color.a * submix);
                        decal_color.xyz *= 0.3;
                        decal.tint.xyz = vec3(0.0);
                        decal_color.a *= 0.7;
                    } else if (type == decalFire) {
    #if defined(FIRE_DECAL_ENABLED) && !defined(NO_DECALS)
                        float fade = max(0.0, (0.5 - length(temp))*8.0)* max(0.0, fractal(temp.xz*7.0) + 0.3);
                        temp = world_vert * 2.0;
                        float fire = abs(fractal(temp.xz*11.0 + time*3.0) + fractal(temp.xy*7.0 - time*3.0) + fractal(temp.yz*5.0 - time*3.0));
                        float flame_amount = max(0.0, 0.5 - (fire*0.5 / pow(fade, 2.0))) * 2.0;
                        flame_amount += pow(max(0.0, 0.7 - fire), 2.0);
                        vec3 flame_color = vec3(1.5 * pow(flame_amount, 0.7), 0.5 * flame_amount, 0.1 * pow(flame_amount, 2.0));
                        float mix_amount = min(1.0, max(0.0, fade * 4.0));

                        flame_final_color = mix(flame_final_color, flame_color, mix_amount);
                        flame_final_contrib = flame_final_contrib * (1.0 - mix_amount) + mix_amount;
    #endif // FIRE_DECAL_ENABLED
                    }
                    if (type != decalFire) {
                        colormap.xyz = mix(colormap.xyz, decal_color.xyz * decal.tint.xyz, decal_color.a * submix);
    #if defined(DECAL_NORMALS)
                        vec3 decal_tan = cross(ws_normal, inv_rotation_mat * vec3(0.0, 0.0, 1.0));
                        vec3 decal_bitan = cross(ws_normal, decal_tan);
                        vec3 new_normal = vec3(0);
                        new_normal += ws_normal * (decal_normal.b*2.0-1.0);
                        new_normal += (decal_normal.r*2.0-1.0) * decal_tan;
                        new_normal += (decal_normal.g*2.0-1.0) * decal_bitan;
                        ws_normal = normalize(mix(ws_normal, new_normal, decal_color.a));
    #endif  // DECAL_NORMALS
                    }
                }
            }
        }
    }
}
#endif //!DEPTH_ONLY

#endif //NO_DECALS
