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
void object_vert(){} // This is just here to make sure it gets added to include paths

#include "pseudoinstance.glsl"
#include "shadowpack.glsl"
#include "texturepack.glsl"
#include "lighting.glsl"

#define UNIFORM_REL_POS \
uniform vec3 cam_pos;

#define CALC_REL_POS \
ws_vertex = transformed_vertex.xyz - cam_pos;

#define CALC_TAN_TO_WORLD \
mat3 obj2worldmat3 = GetPseudoInstanceMat3Normalized(); \
mat3 tan_to_obj = mat3(gl_MultiTexCoord1.xyz, gl_MultiTexCoord2.xyz, normalize(gl_Normal)); \
tangent_to_world = obj2worldmat3 * tan_to_obj;

#define CALC_TRANSFORMED_VERTEX \
mat4 obj2world = GetPseudoInstanceMat4(); \
vec4 transformed_vertex = obj2world * gl_Vertex; \
gl_Position = gl_ModelViewProjectionMatrix * transformed_vertex; \

#define CALC_TEX_COORDS \
tc0 = gl_MultiTexCoord0.xy;\
tc1 = GetShadowCoords();\
CALC_CASCADE_TEX_COORDS

#define TERRAIN_LIGHT_OFFSET vec2(0.0005)+ws_light.xz*0.0005