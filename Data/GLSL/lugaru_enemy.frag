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

#pragma transparent
#pragma blendmode_add

#include "object_shared.glsl"
#include "object_frag.glsl"

uniform float time;

UNIFORM_COMMON_TEXTURES
VARYING_TAN_TO_WORLD
UNIFORM_LIGHT_DIR
UNIFORM_EXTRA_AO
UNIFORM_COLOR_TINT
UNIFORM_AVG_COLOR4
uniform float detail_scale;
VARYING_REL_POS
VARYING_SHADOW
varying vec3 tangent;
varying vec3 bitangent;

void main()
{        
	float a = tc0.y * 2.0;
	float tm = time * 0.5 * color_tint.g;		      //this number controls speed
	vec2 tc0_nv = tc0;

	tc0_nv.y -= tm;                                       //makes texture 'scroll' in the y axis
	tc0_nv.x += sin(tc0_nv.y + tm * 0.2) * 0.2;         //makes texture move back and fourth
	a = 1.0;

	vec3 ws_normal;
	vec4 normalmap = texture2D(normal_tex,tc0_nv);
	{
	    vec3 unpacked_normal = UnpackTanNormal(normalmap);
	    ws_normal = tangent_to_world * unpacked_normal;
	}

    vec4 colormap = texture2D(color_tex, tc0_nv);
	float fresnel = 1.0;
	//gl_FragColor = vec4(colormap.r + 0.4, colormap.g + 0.4, colormap.b + 0.1, min(max(pow(max(0.0, colormap.r - a + 0.5), 32.0 * color_tint.r), 0.0), 1.0) * fresnel * color_tint.b);
	gl_FragColor = vec4(colormap.r + 0.0, colormap.g + 0.0, colormap.b + 0.0, min(max(pow(0.2, 1.0 * color_tint.r), 0.0), 0.6) * fresnel * color_tint.b);

	//gl_FragColor = vec4(colormap.r + 0.0, colormap.g + 0.0, colormap.b + 0.0, min(max(pow(max(0.0, colormap.r - a + 0.5), 1.0 * color_tint.r), 0.0), 1.0) * fresnel * color_tint.b);
	//gl_FragColor = vec4(dot(normalize(ws_normal), normalize(ws_vertex)) + 0.5, 0.1, 0.5, 0.1);
    //CALC_FINAL_UNIVERSAL(max(1.0 - pow(a, 2.0), 0.0))
}
