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

in vec2 pixel_uv;

void main() {    
    vec4 color = texture2DLod(tex0, pixel_uv, 0.0);
    float avg = (color[0] + color[1] + color[2]) / 3.0;
    float bucket = avg;
    gl_Position = vec4(max(-0.99999, min(0.99999, bucket*2.0-1.0)), 0.5, 0.0, 1.0);
}
