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
#version 400

layout(triangles) in;
layout(triangle_strip) out;
layout(max_vertices = 256) out;

uniform float time;
uniform mat4 projection_view_mat;

in vec2 frag_tex_coords[];
in mat3 tan_to_obj[];
out vec2 frag_tex_coords_fs;
out mat3 tan_to_obj_fs;

#define USE_GEOM_SHADER

void AddVert(vec3 bary, float height){
     gl_Position = projection_view_mat * (inverse(projection_view_mat)*(gl_PositionIn[0] * bary[0] + gl_PositionIn[1] * bary[1] + gl_PositionIn[2] * bary[2]) + vec4(tan_to_obj[0]*vec3(0,0,height),0));
     frag_tex_coords_fs = frag_tex_coords[0] * bary[0] + frag_tex_coords[1] * bary[1] + frag_tex_coords[2] * bary[2];
     tan_to_obj_fs = tan_to_obj[0] * bary[0] + tan_to_obj[1] * bary[1] + tan_to_obj[2] * bary[2];
     EmitVertex();
 }

void main()
{
   for(int i = 0; i < gl_VerticesIn; i++)
   {
     gl_Position = gl_PositionIn[i];
     frag_tex_coords_fs = frag_tex_coords[i];
     tan_to_obj_fs = tan_to_obj[i];
     EmitVertex();
   }
   EndPrimitive();

   for(int i=0; i<8; ++i){
    for(int j=0; j<8; ++j){
        float x = (i)/8.0;
        float y = (j)/8.0;
        vec3 pos;
        pos = mix(vec3(0,0,1), vec3(1,0,0), x);
        pos = mix(pos, vec3(0,1,0), y);
        pos[2] = 1.0 - pos[0] - pos[1];
        AddVert(pos, 0.0);
        AddVert(pos+vec3(0.05,0.0,-0.05), 1.0);
        AddVert(pos+vec3(0.1,0.0,-0.1), 0.0);
        EndPrimitive();
    }
   }

}
