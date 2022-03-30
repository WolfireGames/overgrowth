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
in vec4 color;

#ifdef FIRE
in vec3 world_vert;
#endif

#pragma bind_out_color
out vec4 out_color;

#ifdef FIRE
#ifndef NO_VELOCITY_BUF
#pragma bind_out_vel
out vec4 out_vel;
#endif  // NO_VELOCITY_BUF
#endif

uniform float opacity;
uniform sampler2D tex5;
uniform vec2 viewport_dims;
uniform vec3 cam_pos;
uniform mat4 mvp;

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
    uv= fract(uv / 100.0 + vec2(0.5)) * 100.0;
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

float LinearizeDepth(float z) {
  float n = 0.1; // camera z near
  float f = 100000.0; // camera z far
  float depth = (2.0 * n) / (f + n - z * (f - n));
  return (f-n)*depth + n;
}

void main() {    
    #ifdef FIRE
    const float kFireSpeed = 3.0;
    float env_depth = LinearizeDepth(texture(tex5,gl_FragCoord.xy / viewport_dims).r);
    float particle_depth = LinearizeDepth(gl_FragCoord.z);
    float heat = max(0.0, color[0]);
    float u = color[3];
    float v = color[1];
    float time = color[2];
    u -= 0.5;
    float old_u = u;
    vec3 rel = normalize(world_vert - cam_pos);
    float fractal_u = u + (rel.x)*0.2;
    float fractal_v = v + (rel.z)*0.02;
    float noise = fractal(vec2(fractal_u,fractal_v * 10.0));
    float noise2 = (fractal(vec2(fractal_u,fractal_v * 10.0 + time * kFireSpeed)*3) + fractal(vec2(fractal_u,fractal_v * 10.0 + time * kFireSpeed)*-2))*0.5;
    u += (noise * 0.3 + noise2 * 0.3 + 0.15)*1.0;
    float width = min(pow(heat, 0.6), 1.0) * 0.5 * 0.5;
    float alpha;
    if(abs(u) > width){
        alpha = pow(max(0.0, 1.0 - (abs(u) - width)), 60.0) * min(1.0, max(0.0, (color[0] + 0.02)*10.0));
    } else {
        alpha = pow((abs(u)/width), 2.0);
        alpha = max(alpha, min(1.0, (0.1-heat)*10.0));
        alpha += pow(0.5-abs(noise2), 2.0) * 8.0;
    }   
    alpha *= 0.25;
    float start_fade = max(0.0, min(1.0,  (time - v - 0.1)*20.0));
    start_fade *= 1.0 - max(0.0, min(1.0,  (time - v - 0.3)*20.0));
    start_fade *= min(1.0, (env_depth - particle_depth)*3.0);
    alpha = max(0.0, alpha + start_fade - 1.0);
    alpha *= max(0.0, min(1.0, (particle_depth-0.4)));
    out_color = vec4(6.0-heat, 1.0, 0.0, alpha);
    out_color.b = max(0.0, pow(heat * 2.5, 2.0)-1.0)*0.5;
    out_color.g = max(0.0, pow(heat,0.5) * 3.0);
    out_color.a *= pow(heat, 0.5);


    vec3 world_dx = dFdx(world_vert);
    vec3 world_dy = dFdy(world_vert);
    vec3 norm = normalize(cross(world_dx, world_dy));
    vec3 view_dir = normalize(cam_pos-world_vert);
    vec3 right = cross(view_dir, vec3(0,1,0));
    vec3 up = cross(right, view_dir);
    float dot_val = dot(norm, view_dir);
    //out_color = vec4(norm, dot(norm, view_dir));
    out_color.a *= min(1.0, max(0.0, (dot_val-0.4)*2.0));
    //out_color = vec4(vec3((dot_val-0.2)*0.1), 1.0);
    //out_color = vec4(1.0, 1.0, 1.0, 0.1*start_fade);
    const float motion_blur_amount = 5.0;

    #ifndef NO_VELOCITY_BUF

        out_vel.xyz = right * (noise+0.3)*motion_blur_amount;
        out_vel.xyz += up * (noise2+0.3)*motion_blur_amount;
        out_vel.a = start_fade * 0.3 * min(1.0, max(0.0, -0.3-color[0]));//0.0, min(1.0, pow(width / abs(u), 10.0))*start_fade);
        out_vel.xyz *= env_depth;

    #endif  // NO_VELOCITY_BUF

    //out_color = out_vel;
    #else  // FIRE
	out_color = vec4(color.rgb, color.w * opacity);
    #endif  // FIRE
}
