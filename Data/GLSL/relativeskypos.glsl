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
#if !defined(RELATIVE_SKY_POS_GLSL)
#define RELATIVE_SKY_POS_GLSL

#if defined(VERTEX_SHADER)
    vec3 CalcRelativePositionForSky(const mat4 obj2world, const vec3 cam_pos) {
        vec3 position = (obj2world * vec4(gl_Vertex.xyz,0.0)).xyz - cam_pos;
        return position;
    }
#endif

#endif
