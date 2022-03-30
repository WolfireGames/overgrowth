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
uniform samplerCube tex0;
uniform samplerCube tex1;
uniform float time;
uniform vec3 tint;
uniform float fog_amount;
in vec3 normal;

#pragma bind_out_color
out vec4 out_color;

#ifdef YCOCG_SRGB
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

#ifdef TEST_CLOUDS
float hash( vec2 p )
{
    float h = dot(p,vec2(127.1,311.7));
    
    return -1.0 + 2.0*fract(sin(h)*43758.5453123);
}

float noise( in vec2 p )
{
    vec2 i = floor( p );
    vec2 f = fract( p );
    
    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( hash( i + vec2(0.0,0.0) ), 
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ), 
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}

float fractal( in vec2 uv){
    float f = 0.0;
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    f  = 0.5000*noise( uv ); uv = m*uv;
    f += 0.2500*noise( uv ); uv = m*uv;
    f += 0.1250*noise( uv ); uv = m*uv;
    f += 0.0625*noise( uv ); uv = m*uv;
    f += 0.03125*noise( uv ); uv = m*uv;
    f += 0.016625*noise( uv ); uv = m*uv;
    return f;
}
#endif

#if defined(TEST_CLOUDS_2)
const float cloudscale = 1.1;
const float speed = 0.03;
const float clouddark = 0.5;
const float cloudlight = 0.3;
const float cloudcover = 0.2;
const float cloudalpha = 8.0;
const float skytint = 0.5;
const vec3 skycolour1 = vec3(0.2, 0.4, 0.6);
const vec3 skycolour2 = vec3(0.4, 0.7, 1.0);

#endif
#if defined(TEST_CLOUDS_2) || defined(ADD_STARS)
const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );

vec2 hash( vec2 p ) {
    p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
    return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p ) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;
    vec2 i = floor(p + (p.x+p.y)*K1);   
    vec2 a = p - i + (i.x+i.y)*K2;
    vec2 o = (a.x>a.y) ? vec2(1.0,0.0) : vec2(0.0,1.0); //vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y), sign(a.y-a.x));
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0*K2;
    vec3 h = max(0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
    vec3 n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot(n, vec3(70.0));  
}

float fbm(vec2 n) {
    float total = 0.0, amplitude = 0.1;
    for (int i = 0; i < 7; i++) {
        total += noise(n) * amplitude;
        n = m * n;
        amplitude *= 0.4;
    }
    return total;
}
#endif

void main() {    
    vec3 color;
#ifdef YCOCG_SRGB
    color = YCOCGtoRGB(texture(tex0,normal));
    color = pow(color,vec3(2.2));
#else
    color = texture(tex0,normal).xyz;
#endif
    float foggy = max(0.0, min(1.0, (fog_amount - 1.0) / 2.0));
    float fogness = mix(-1.0, 1.0, foggy);
    if(normal.y < 0.0){
        fogness = mix(fogness, 1.0, -normal.y * fog_amount / 5.0);
    }
    float blur = max(0.0, min(1.0, (1.0-abs(normalize(normal).y)+fogness)));
    color = mix(color, textureLod(tex1,normal, mix(pow(blur, 2.0), 1.0, fogness*0.5+0.5) * 5.0).xyz, min(1.0, blur * 4.0));
    color.xyz *= tint;
    //vec3 tint = vec3(1.0, 0.0, 0.0);
    //color *= tint;
    out_color = vec4(color,1.0);

    #ifdef ADD_STARS
    float star = noise(normalize(normal).xz/(normalize(normal).y+2.0) * 200.0+ vec2(time*0.05,0.0));
    star = max(0.0, star - 0.7);
    star = star * 100.0;
    star *= max(0.0, 1.0 - abs(noise(normalize(normal).xz/(normalize(normal).y+2.0) * 20.0 + vec2(time,0.0)))*1.0);
    if(out_color.b < 0.1){
        out_color.xyz += vec3(star)*0.004;
    }
    #endif

    #ifdef TEST_CLOUDS
    vec3 normalized = normalize(normal);
    vec3 plane_intersect = normalized / (normalized.y+0.2);
    vec2 uv = plane_intersect.xz * 4.0;
    //uv *= (2.0 - normalized.y) * 0.5;
    uv.x += time * 0.1;
    float f = (fractal(uv + vec2(time*0.2, 0.0))+fractal(uv + vec2(0.0, time*0.14)));
    f = min(1.0, f*0.5 + 0.5) * 0.9;

    float min_threshold = sin(time)*0.5+0.5;
    f = min(1.0, max(0.0, f - min_threshold)/(1.0-min_threshold));

    f *= max(0.0, pow(normalized.y, 0.2));

    out_color.xyz = mix(out_color.xyz, vec3(1.0), f);
    #endif

    #ifdef TEST_CLOUDS_2
    vec3 normalized = normalize(normal);
    vec3 plane_intersect = normalized / (normalized.y+0.2);
    vec2 old_uv = plane_intersect.xz;
    vec2 uv = old_uv; 
    float temp_time = time * speed;

    float q = fbm(uv * cloudscale * 0.5);
    
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
    
    vec3 skycolour = mix(skycolour2, skycolour1, normalized.y);
    vec3 cloudcolour = vec3(1.1, 1.1, 0.9) * clamp((clouddark + cloudlight*c), 0.0, 1.0);
   
    f = cloudcover + cloudalpha*f*r;
    
    vec3 result = mix(skycolour, clamp(skytint * skycolour + cloudcolour, 0.0, 1.0), clamp(f + c, 0.0, 1.0));
    
    float mix_amount = max(normalized.y, 0.0);
    out_color = mix(out_color, vec4( result * 0.5, 1.0 ), mix_amount);
    #endif
}
