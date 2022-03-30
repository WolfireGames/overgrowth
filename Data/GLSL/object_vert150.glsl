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
#include "lighting.glsl"

#define UNIFORM_REL_POS \
uniform vec3 cam_pos;

#define CALC_REL_POS \
ws_vertex = transformed_vertex.xyz - cam_pos;

#define TERRAIN_LIGHT_OFFSET vec2(0.0005)+ws_light.xz*0.0005