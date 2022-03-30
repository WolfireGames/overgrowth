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
#ifndef PSEUDO_INSTANCE_GLSL
#define PSEUDO_INSTANCE_GLSL

uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

#ifdef VERTEX_SHADER

mat4 GetPseudoInstanceMat4() {
    return modelMatrix;
}
 
mat3 GetPseudoInstanceMat3() {
     return normalMatrix;
}

mat3 GetPseudoInstanceMat3Normalized() {
    return normalMatrix;
}
#endif

#endif
