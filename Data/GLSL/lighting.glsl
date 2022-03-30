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
#ifndef LIGHTING_GLSL
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

void SetCascadeShadowCoords(vec4 vert, inout vec4 sc[4]) {
    sc[0] = gl_TextureMatrix[0] * gl_ModelViewMatrix * vert;
    sc[1] = gl_TextureMatrix[1] * gl_ModelViewMatrix * vert;
    sc[2] = gl_TextureMatrix[2] * gl_ModelViewMatrix * vert;
    sc[3] = gl_TextureMatrix[3] * gl_ModelViewMatrix * vert;
}

#ifdef FRAGMENT_SHADER
float GetCascadeShadow(sampler2DShadow tex5, vec4 sc[4], float dist){
    float rand_a = rand(gl_FragCoord.xy);
    vec3 shadow_tex = vec3(1.0);
    int index = 0;
    if(length(sc[0].xy-vec2(0.5)) > 0.49 - rand_a * 0.05){
        index = 1;
    }    
    if(length(sc[1].xy-vec2(0.5)) > 0.49 - rand_a * 0.05){
        index = 2;
    }    
    if(length(sc[2].xy-vec2(0.5)) > 0.49 - rand_a * 0.05){
        index = 3;
    }
    vec4 shadow_coord = sc[index];
    shadow_coord.z -= 0.00003 * pow(2.0, float(index)*1.7);
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
    if(shadow_coord.x >= 1.0 ||
       shadow_coord.y >= 1.0 ||
       shadow_coord.x <= 0.0 ||
       shadow_coord.y <= 0.0)
    {
        return 1.0;
    }
    float shadow_amount = 0.0;
    float offset = 1.5/2048.0;
    float offset_angle =  rand_a * 6.28;
    vec2 offset_dir;
    offset_dir.x = sin(offset_angle) * offset;
    offset_dir.y = cos(offset_angle) * offset;

    vec4 offset_shadow_coord = shadow_coord;
    offset_shadow_coord.x += (rand_a - 0.5) * offset;
    offset_shadow_coord.y += (rand(gl_FragCoord.xy + vec2(500.0,500.0)) - 0.5) * offset;

    shadow_amount += shadow2DProj(tex5,offset_shadow_coord).r * 0.2;
    shadow_amount += shadow2DProj(tex5,shadow_coord+vec4(offset_dir.x, offset_dir.y, 0.0,0.0)).r * 0.2;
    shadow_amount += shadow2DProj(tex5,shadow_coord+vec4(-offset_dir.x, -offset_dir.y,0.0,0.0)).r * 0.2;
    shadow_amount += shadow2DProj(tex5,shadow_coord+vec4(-offset_dir.y, offset_dir.x,0.0,0.0)).r * 0.2;
    shadow_amount += shadow2DProj(tex5,shadow_coord+vec4(offset_dir.y, -offset_dir.x,0.0,0.0)).r * 0.2;
    return shadow_amount;
}
#endif
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
    return textureCube(cube_map,world_space_vec).xyz;
}

vec3 LookupCubemapMat4(const mat4 obj2world, 
                   const vec3 vec, 
                   const samplerCube cube_map) {
    vec3 world_space_vec = (obj2world * vec4(vec,0.0)).xyz;
    return textureCube(cube_map,world_space_vec).xyz;
}

vec3 LookupCubemapSimple(const vec3 vec, 
                   const samplerCube cube_map) {
    vec3 world_space_vec = vec;
    return textureCube(cube_map,world_space_vec).xyz;
}

vec3 LookupCubemapSimpleLod(const vec3 vec, 
                   const samplerCube cube_map, float lod) {
    vec3 world_space_vec = vec;
    return textureCubeLod(cube_map,world_space_vec,lod).xyz;
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

float BalanceAmbient ( const float direct_contrib ) {
    return 1.0-direct_contrib*0.2;
}

float GetHazeAmount( in vec3 relative_position ) {
    #ifdef DISABLE_FOG
    return 0.0f;
    #endif 
    float near = 0.1;
    float far = 1000.0;
    float fog_opac = min(1.0,length(relative_position)/far);
    return fog_opac;
}

void AddHaze( inout vec3 color, 
              in vec3 relative_position,
              in samplerCube fog_cube ) { 
    vec3 fog_color = textureCubeLod(fog_cube,relative_position,5.0).xyz;
    color = mix(color, fog_color, GetHazeAmount(relative_position));
}

float GammaCorrectFloat(in float val) {
#ifdef GAMMA_CORRECT
    return pow(val,2.2);
#else
    return val;
#endif
}

vec3 GammaCorrectVec3(in vec3 val) {
#ifdef GAMMA_CORRECT
    return vec3(pow(val.r,2.2),pow(val.g,2.2),pow(val.b,2.2));
#else
    return val;
#endif
}

vec3 GammaAntiCorrectVec3(in vec3 val) {
#ifdef GAMMA_CORRECT
    return vec3(pow(val.r,1.0/2.2),pow(val.g,1.0/2.2),pow(val.b,1.0/2.2));
#else
    return val;
#endif
}

void ReadBloodTex(in sampler2D tex, in vec2 tex_coords, out float blood_amount, out float wetblood){
    vec4 blood_tex = texture2D(tex,tex_coords);
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
#endif
