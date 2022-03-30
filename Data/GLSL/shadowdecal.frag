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
uniform sampler2DShadow tex2;

varying vec3 light_pos;
varying vec3 light2_pos;
varying vec3 vertex_pos;
varying vec4 ProjShadow;

void main()
{    
    float NdotL;
    vec3 color;
    
    vec3 H = normalize(normalize(vertex_pos*-1.0) + normalize(light_pos));
    
    float faded = max(abs(gl_TexCoord[0].z)-0.5,0.0)*2.0;
    float alpha = texture2D(tex0,gl_TexCoord[0].xy).a;
    float depth=max(0.3-faded,0.0);
    
    vec2 tex_offset;
    float height;
    vec4 normalmap = texture2D(tex1,gl_TexCoord[0].xy);
    
    height = -normalmap.a*depth+depth;
    tex_offset = height * normalize(vertex_pos).xy * normalmap.z;
    
    normalmap = texture2D(tex1,gl_TexCoord[0].xy+tex_offset);
    
    height = (height+(-normalmap.a*depth+depth))/2.0;
    tex_offset = height * normalize(vertex_pos).xy * normalmap.z;
    
    float spec;
    vec3 normal;
    vec4 color_tex;
    
    normalmap = texture2D(tex1,gl_TexCoord[0].xy+tex_offset);
    normal = normalize(vec3((normalmap.x-0.5)*2.0, (normalmap.y-0.5)*-2.0, normalmap.z));
    //normal = vec3(0,0,1);
    
    float offset = 1.0/4096.0/2.0;
    float shadowed = shadow2DProj(tex3, ProjShadow).r*.2;
    shadowed += shadow2DProj(tex3, ProjShadow + vec4(-offset*2.0,offset,0.0,0.0)).r*.2;
    shadowed += shadow2DProj(tex3, ProjShadow - vec4(offset*2.0,-offset,0.0,0.0)).r*.2;
    shadowed += shadow2DProj(tex3, ProjShadow + vec4(-offset,offset*2.0,0.0,0.0)).r*.2;
    shadowed += shadow2DProj(tex3, ProjShadow + vec4(offset,-offset*2.0,0.0,0.0)).r*.2;
    
    NdotL = max(dot(normal,normalize(light_pos)),0.0);
    spec = max(pow(dot(normal,H),40.0),0.0)*2.0 * NdotL ;
    
    color_tex = texture2D(tex0,gl_TexCoord[0].xy+tex_offset);
    
    color = gl_LightSourceDEPRECATED[0].diffuse.xyz * NdotL *(0.4+shadowed*0.6) * color_tex.xyz;
    color += spec * gl_LightSourceDEPRECATED[0].diffuse.xyz * normalmap.a * shadowed * 0.4;
    
    NdotL = max(dot(normal,normalize(light2_pos)),0.0);
    H = normalize(normalize(vertex_pos*-1.0) + normalize(light2_pos));
    spec = max(pow(dot(normal,H),4.0),0.0);
    
    color += gl_LightSourceDEPRECATED[1].diffuse.xyz * NdotL * color_tex.xyz;
    color += spec * gl_LightSourceDEPRECATED[1].diffuse.xyz * normalmap.a * 0.2;

    gl_FragColor = vec4(color,max(alpha-faded,0.0));
}
