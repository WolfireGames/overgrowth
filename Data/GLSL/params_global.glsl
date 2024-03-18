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

#og_version_major 1
#og_version_minor 5

layout(std140) uniform GlobalVec4Params {
    vec4 global_vec4_params[1024];
};

layout(std140) uniform GlobalIVec4Params {
    ivec4 global_ivec4_params[1024];
};
