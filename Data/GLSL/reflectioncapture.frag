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

uniform samplerCube tex1;
uniform vec3 cam_pos;

in vec3 world_vert;
in vec3 normal;

#pragma bind_out_color
out vec4 out_color;

void main()
{    
    if(int(gl_FragCoord.x + gl_FragCoord.y)%2==0){
        discard;
    }
    //vec3 total = SampleAmbientCube(acc, normalize(vert));
    vec3 ws_normal = normalize(normal);
    vec3 spec_map_vec = reflect(world_vert - cam_pos, ws_normal);
    vec3 spec = textureLod(tex1, ws_normal, 0.0).xyz;
    vec3 diffuse = textureLod(tex1, ws_normal, 5.0).xyz;
    float fresnel = dot(normalize(world_vert - cam_pos), ws_normal);
    out_color.xyz = spec;//mix(diffuse, spec, 1.0 + fresnel);
    out_color.a = 1.0;
}
