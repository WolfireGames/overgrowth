#version 150
#extension GL_ARB_shading_language_420pack : enable

uniform float time;
uniform vec3 cam_pos;

#include "object_frag150.glsl"
#include "object_shared150.glsl"
#include "ambient_tet_mesh.glsl"

uniform vec4 color_tint;
uniform sampler2D tex0; // ColorMap
uniform sampler2D tex1; // Normalmap
uniform sampler2D tex2; // Diffuse cubemap
uniform sampler2D tex3; // Diffuse cubemap
uniform sampler2D tex4; // Diffuse cubemap
uniform sampler2D tex10;
uniform sampler2D tex11;
uniform sampler2D tex12;
uniform sampler2D tex14;
uniform sampler2D tex16;
uniform sampler2D tex17;
uniform sampler2D tex18;
uniform sampler2D tex19;
uniform sampler2D tex20;

const int kMaxInstances = 100;

struct Instance {
    mat4 model_mat;
    mat3 model_rotation_mat;
    vec4 color_tint;
    vec4 detail_scale;
};

uniform InstanceInfo {
	Instance instances[kMaxInstances];
};

uniform mat4 shadow_matrix[4];
uniform mat4 projection_view_mat;

in vec3 world_vert;

#pragma bind_out_color
out vec4 out_color;

in vec2 frag_tex_coords;
in vec2 tex_coord;
in vec2 base_tex_coord;
in vec3 orig_vert;
in mat3 tangent_to_world;
flat in int instance_id;

uniform float overbright;

void main() {
	vec4 colormap = texture(tex0, frag_tex_coords);
	out_color = colormap;

	float scale = 2.0;

	if(int(gl_FragCoord.x) % 2 != 0 || int(gl_FragCoord.y) % 2 != 0){
	// if(int((gl_FragCoord.x + gl_FragCoord.y) / scale) % 2 == 0 || int((gl_FragCoord.x - gl_FragCoord.y) / scale) % 2 == 0){
		discard;
	}
}
