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
uniform mat4 mvp;
#ifdef COLOR_UNIFORM
	uniform vec4 color_uniform;
#endif
uniform vec3 cam_pos;

in vec3 vert_attrib;
#ifdef COLOR_ATTRIB
	in vec4 color_attrib;
#endif

out vec4 color;

#ifdef FIRE
    out vec3 world_vert;
#endif

void main() {
#ifdef FIRE
    world_vert = vert_attrib + normalize(cam_pos - vert_attrib) * 0.1;
    gl_Position = mvp * vec4(world_vert, 1.0);
#else
    gl_Position = mvp * vec4(vert_attrib, 1.0);
#endif

#ifdef COLOR_ATTRIB
    color = color_attrib;
#elif defined(COLOR_UNIFORM)
    color = color_uniform;
#else
    color = vec4(1.0);
#endif
} 
