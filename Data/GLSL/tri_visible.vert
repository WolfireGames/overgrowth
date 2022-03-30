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
uniform int output_width;
uniform int output_height;

in vec2 pixel_uv;

void main() {    
    vec4 color = textureLod(tex0, pixel_uv, 0.0);

    if(color[3] != 0.0){ // pixel represents a valid triangle
        int id = int(color[0] * 255.0 + 0.5); // Extract packed triangle ID
        id += int(color[1] * 255.0 + 0.5) * 256;
        id += int(color[2] * 255.0 + 0.5) * 256 * 256;
        // Create output coordinates based on ID
        int row = id / output_width;
        int column = id % output_width;
        // Draw to those coordinates in the texture
        gl_Position.x = ((float(column) * 2.0 + 0.5) / float(output_width) - 1.0);
        gl_Position.y = ((float(row) * 2.0 + 0.5) / float(output_height) - 1.0);
        gl_Position.z = 0.0;
        gl_Position.w = 1.0;
    } else { // This triangle is not needed, draw outside of the texture (don't draw)
    	gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
    }
}
