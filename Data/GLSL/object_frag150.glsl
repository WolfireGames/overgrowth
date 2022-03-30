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
void object_frag(){} // This is just here to make sure it gets added to include paths

layout (std140) uniform ClusterInfo {
    uvec3 grid_size;
    uint num_decals;
    uint num_lights;
    uint light_cluster_data_offset;
    uint light_data_offset;
    uint cluster_width;
    mat4 inv_proj_mat;
    vec4 viewport;
    float z_near;
    float z_mult;
    float pad3;
    float pad4;
};

// this MUST match the one in source or bad things happen
struct PointLightData {
    vec3 pos;
    float radius;
    vec3 color;
    float padding;
};

#define POINT_LIGHT_SIZE_VEC4 2u

#define NUM_GRID_COMPONENTS 2u
#define ZCLUSTERFUNC(val) (log(-1.0 * (val) - z_near + 1.0) * z_mult)

#define light_decal_data_buffer tex15
#define cluster_buffer tex13

const uint COUNT_BITS = 24u;
const uint COUNT_MASK = ((1u << (32u - COUNT_BITS)) - 1u);
const uint INDEX_MASK = ((1u << (COUNT_BITS)) - 1u);

#if !defined(DEPTH_ONLY)
    uniform samplerBuffer light_decal_data_buffer;
    uniform usamplerBuffer cluster_buffer;

    PointLightData FetchPointLight(uint light_index) {
        PointLightData l;

        vec4 temp = texelFetch(light_decal_data_buffer, int(light_data_offset + POINT_LIGHT_SIZE_VEC4 * light_index + 0u));
        l.pos = temp.xyz;
        l.radius = temp.w;

        temp = texelFetch(light_decal_data_buffer, int(light_data_offset + POINT_LIGHT_SIZE_VEC4 * light_index + 1u));
        l.color = temp.xyz;
        l.padding = temp.w;

        return l;
    }

    void CalculateLightContrib(inout vec3 diffuse_color, inout vec3 spec_color, vec3 ws_vertex, vec3 world_vert, vec3 ws_normal, float roughness, uint light_val, float ambient_mult) {
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

            float light_size = 0.8;

            vec3 to_light = l.pos - world_vert;
            // TODO: inverse square falloff
            // TODO: real light equation
            float dist = max(light_size, length(to_light));
            float falloff = max(0.0, (1.0 / dist / dist) * (1.0 - dist / l.radius));

            vec3 n = normalize(to_light);
            float bias = max(0.0, (light_size - length(to_light)) / light_size * 2.0);

            #if defined(PLANT)
                float d = abs(dot(n, ws_normal)+bias)/(1.0 + bias);
            #elif defined(WATERFALL)
                float d = 0.5;
            #elif defined(LIGHT_AMB)
                float d = 0.5;
            #else
                float d = max(0.0, dot(n, ws_normal)+bias)/(1.0 + bias);
            #endif

            float temp_ambient_mult = ambient_mult;

            #if defined(LANTERN)
                if(l.color.r > 1.635 && l.color.r < 1.636){
                    temp_ambient_mult = 1.0;

                    if(n.y > 0.0){
                        temp_ambient_mult = max(0.0, temp_ambient_mult-pow(n.y,0.9));
                    }

                    if(n.y < 0.0){
                        temp_ambient_mult = max(0.0, temp_ambient_mult+n.y*0.99);
                    }

                    temp_ambient_mult *= 2.0;
                    bias = 0.05;
                    d = max(0.0, dot(n, ws_normal)+bias)/(1.0 + bias);
                }
            #endif

            falloff = min(1.0, falloff);
            diffuse_color += falloff * d * l.color * temp_ambient_mult;

            #if !defined(SWAMP2)
                roughness = max(roughness, 0.05);
                vec3 H = normalize(normalize(ws_vertex*-1.0) + n);
                float spec_pow = 2/pow(roughness, 4.0) - 2.0;
                float spec = pow(max(0.0,dot(ws_normal,H)), spec_pow);
                spec *= 0.25 * (spec_pow + 8) / (8 * 3.141592);

                spec_color += falloff * spec * l.color * temp_ambient_mult;
            #endif
        }
    }
#endif  // ^ !defined(DEPTH_ONLY)

// From iq on shadertoy
float hash(vec2 p)
{
    float h = dot(p, vec2(127.1, 311.7));

    return -1.0 + 2.0*fract(sin(h)*43758.5453123);
}

float noise(in vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 u = f*f*(3.0 - 2.0*f);

    return mix(mix(hash(i + vec2(0.0, 0.0)),
        hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)),
            hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fractal(in vec2 uv) {
    float f = 0.0;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    f = 0.5000*noise(uv); uv = m*uv;
    f += 0.2500*noise(uv); uv = m*uv;
    f += 0.1250*noise(uv); uv = m*uv;
    f += 0.0625*noise(uv); uv = m*uv;
    f += 0.03125*noise(uv); uv = m*uv;
    f += 0.016625*noise(uv); uv = m*uv;
    return f;
}

#include "lighting150.glsl"
#include "relativeskypos.glsl"

#if !defined(ARB_sample_shading_available)
    #define CALC_MOTION_BLUR \
        if(stipple_val != 1 && \
           (int(mod(gl_FragCoord.x + float(x_stipple_offset),float(stipple_val))) != 0 ||  \
            int(mod(gl_FragCoord.y + float(y_stipple_offset),float(stipple_val))) != 0)){  \
            discard;  \
        }
#else
    #define CALC_MOTION_BLUR \
        if(stipple_val != 1 && \
           (int(mod(gl_FragCoord.x + mod(float(gl_SampleID), float(stipple_val)) + float(x_stipple_offset),float(stipple_val))) != 0 || \
            int(mod(gl_FragCoord.y + float(gl_SampleID) / float(stipple_val) + float(y_stipple_offset),float(stipple_val))) != 0)){ \
            discard; \
        }
#endif

#define CALC_HALFTONE_STIPPLE \
if(mod(gl_FragCoord.x + gl_FragCoord.y, 2.0) == 0.0){ \
    discard; \
}

#define UNIFORM_SHADOW_TEXTURE \
    uniform sampler2DShadow shadow_sampler;

#define CALC_SHADOWED \
    vec3 shadow_tex = vec3(1.0);\
    shadow_tex.r = GetCascadeShadow(tex4, shadow_coords, length(ws_vertex));

#define CALC_DYNAMIC_SHADOWED CALC_SHADOWED

#define color_tex tex0
#define normal_tex tex1
#define spec_cubemap tex2
#define shadow_sampler tex4
#define projected_shadow_sampler tex5
#define translucency_tex tex5
#define blood_tex tex6
#define fur_tex tex7
#define tint_map tex8
#define ambient_grid_data tex11
#define ambient_color_buffer tex12

#define UNIFORM_COMMON_TEXTURES \
uniform sampler2D color_tex; \
uniform sampler2D normal_tex; \
uniform samplerCube spec_cubemap; \
UNIFORM_SHADOW_TEXTURE

#define weight_tex tex5
#define detail_color tex6
#define detail_normal tex7

#define UNIFORM_DETAIL4_TEXTURES \
uniform sampler2D weight_tex; \
uniform sampler2DArray detail_color; \
uniform vec4 detail_color_indices; \
uniform sampler2DArray detail_normal; \
uniform vec4 detail_normal_indices; \

#define UNIFORM_AVG_COLOR4 \
uniform vec4 avg_color0; \
uniform vec4 avg_color1; \
uniform vec4 avg_color2; \
uniform vec4 avg_color3;

#define CALC_BLOOD_AMOUNT \
float blood_amount, wetblood; \
ReadBloodTex(blood_tex, tc0, blood_amount, wetblood);

#define UNIFORM_BLOOD_TEXTURE \
uniform sampler2D blood_tex; \
uniform vec3 blood_tint;

#define UNIFORM_FUR_TEXTURE \
uniform sampler2D fur_tex;

#define UNIFORM_TINT_TEXTURE \
uniform sampler2D tint_map;

#define UNIFORM_TRANSLUCENCY_TEXTURE \
uniform sampler2D translucency_tex;

#define UNIFORM_PROJECTED_SHADOW_TEXTURE \
uniform sampler2DShadow projected_shadow_sampler;

#define UNIFORM_STIPPLE_FADE \
uniform float fade;

#define UNIFORM_STIPPLE_BLUR \
uniform int x_stipple_offset; \
uniform int y_stipple_offset; \
uniform int stipple_val;

#if !defined(SHADOW_CATCHER)
    #define UNIFORM_SIMPLE_SHADOW_CATCH uniform float in_light;
#else
    #define UNIFORM_SIMPLE_SHADOW_CATCH
#endif

#define UNIFORM_COLOR_TINT \
uniform vec3 color_tint;

#define UNIFORM_TINT_PALETTE \
uniform vec3 tint_palette[5];

#define CALC_BLOODY_WEAPON_SPEC \
float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,mix(100.0,50.0,(1.0-wetblood)*blood_amount)); \
spec *= 5.0; \
vec3 spec_color = primary_light_color.xyz * vec3(spec) * mix(1.0,0.3,blood_amount); \
vec3 spec_map_vec = reflect(ws_vertex,ws_normal); \
spec_color += LookupCubemapSimpleLod(spec_map_vec, tex2, 0.0) * 0.5 * \
              GetAmbientContrib(shadow_tex.g) * max(0.0,(1.0 - blood_amount * 2.0));

#define CALC_BLOODY_CHARACTER_SPEC \
float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r,mix(200.0,50.0,(1.0-wetblood)*blood_amount)); \
spec *= 5.0; \
vec3 spec_color = primary_light_color.xyz * vec3(spec) * 0.3; \
vec3 spec_map_vec = reflect(ws_vertex, ws_normal); \
spec_color += LookupCubemapSimpleLod(spec_map_vec, tex2, 0.0) * 0.2 * \
    GetAmbientContrib(shadow_tex.g) * max(0.0,(1.0 - blood_amount * 2.0));

#define CALC_STIPPLE_FADE \
if((rand(gl_FragCoord.xy)) < fade){\
    discard;\
};\

#define CALC_OBJ_NORMAL \
vec4 normalmap = texture(tex1,tc0); \
vec3 os_normal = UnpackObjNormal(normalmap); \
vec3 ws_normal = model_rotation_mat * os_normal; \
ws_normal = normalize(ws_normal);

#define CALC_DIRECT_DIFFUSE_COLOR \
float NdotL = GetDirectContrib(ws_light, ws_normal,shadow_tex.r);\
vec3 diffuse_color = GetDirectColor(NdotL);

#define CALC_DIFFUSE_LIGHTING \
CALC_DIRECT_DIFFUSE_COLOR \
diffuse_color += LookupCubemapSimpleLod(ws_normal, spec_cubemap, 5.0) *\
                 GetAmbientContrib(shadow_tex.g);

#define CALC_DIFFUSE_TRANSLUCENT_LIGHTING \
CALC_DIRECT_DIFFUSE_COLOR \
vec3 ambient = LookupCubemapSimpleLod(ws_normal, tex2, 5.0) * GetAmbientContrib(shadow_tex.g); \
diffuse_color += ambient; \
vec3 translucent_lighting = GetDirectColor(shadow_tex.r) * primary_light_color.a; \
translucent_lighting += ambient; \
translucent_lighting *= GammaCorrectFloat(0.6);

#define CALC_DETAIL_FADE \
float detail_fade_distance = 200.0; \
float detail_fade = min(1.0,max(0.0,length(ws_vertex)/detail_fade_distance));

#define CALC_SPECULAR_LIGHTING(amb_mult) \
vec3 spec_color;\
{\
    vec3 H = normalize(normalize(ws_vertex*-1.0) + normalize(ws_light));\
    float spec = GetSpecContrib(ws_light, ws_normal, ws_vertex, shadow_tex.r);\
    spec_color = primary_light_color.xyz * vec3(spec);\
    vec3 spec_map_vec = reflect(ws_vertex,ws_normal);\
    spec_color += LookupCubemapSimple(spec_map_vec, spec_cubemap) * amb_mult *\
                  GetAmbientContrib(shadow_tex.g);\
}

#define CALC_DISTANCE_ADJUSTED_ALPHA \
colormap.a = pow(colormap.a, max(0.1,min(1.0,4.0/length(ws_vertex))));

#define CALC_COLOR_MAP \
vec4 colormap = texture(color_tex, frag_tex_coords);

#define CALC_MORPHED_COLOR_MAP \
vec4 colormap = texture(color_tex,gl_TexCoord[0].zw);

#define CALC_BLOOD_ON_COLOR_MAP \
ApplyBloodToColorMap(colormap, blood_amount, wetblood, blood_tint);

#define CALC_RIM_HIGHLIGHT \
vec3 view = normalize(ws_vertex*-1.0); \
float back_lit = max(0.0,dot(normalize(ws_vertex),ws_light));  \
float rim_lit = max(0.0,(1.0-dot(view,ws_normal))); \
rim_lit *= pow((dot(ws_light,ws_normal)+1.0)*0.5,0.5); \
color += vec3(back_lit*rim_lit) * (1.0 - blood_amount) * GammaCorrectFloat(normalmap.a) * primary_light_color.xyz * primary_light_color.a * shadow_tex.r;

#define CALC_COMBINED_COLOR_WITH_NORMALMAP_TINT \
vec3 color = diffuse_color * colormap.xyz  * mix(vec3(1.0),color_tint,normalmap.a)+ \
             spec_color * GammaCorrectFloat(colormap.a);

#define CALC_COMBINED_COLOR_WITH_TINT \
vec3 color = diffuse_color * colormap.xyz * color_tint + spec_color * GammaCorrectFloat(normalmap.a);

#define CALC_COMBINED_COLOR \
vec3 color = diffuse_color * colormap.xyz + \
             spec_color * GammaCorrectFloat(colormap.a);

#define CALC_COLOR_ADJUST \
color *= BalanceAmbient(NdotL);

#define CALC_HAZE \
AddHaze(color, ws_vertex, spec_cubemap);

#define CALC_FINAL_UNIVERSAL(alpha) \
out_color = vec4(color,alpha);

#define CALC_FINAL \
CALC_FINAL_UNIVERSAL(1.0)

#define CALC_FINAL_ALPHA \
CALC_FINAL_UNIVERSAL(colormap.a)

#define decalDefault 0
#define decalShadow 1
#define decalBlood 2
#define decalFire 3
#define decalUnused 4
#define decalUnderwater 5
#define decalWatersplat 6
#define decalEnvShadow 7
#define decalWaterFroth 8
#define decalSpongeSquare 9
#define decalSpongeRound 10
#define decalLightSquare 11
#define decalLightRound 12
#define decalEnvShadowSquare 13
#define decalCharacter 14

vec4 GetWeightMap(sampler2D tex, vec2 coord){
    vec4 weight_map = texture(tex, coord);
    weight_map[3] = max(0.0, 1.0 - (weight_map[0]+weight_map[1]+weight_map[2]));
    return weight_map;
}
