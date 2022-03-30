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
void object_shared(){} // This is just here to make sure it gets added to include paths
#include "lighting150.glsl"

#if !defined(BAKED_SHADOWS)
    #define VARYING_SHADOW \
        varying vec4 shadow_coords[4];
    #define CALC_CASCADE_TEX_COORDS SetCascadeShadowCoords(transformed_vertex, shadow_coords);
#else
    #define VARYING_SHADOW
    #define CALC_CASCADE_TEX_COORDS
#endif

#define VARYING_REL_POS \
varying vec3 ws_vertex;

#define VARYING_TAN_TO_WORLD \
varying mat3 tangent_to_world;

#define UNIFORM_LIGHT_DIR \
uniform vec3 ws_light;
