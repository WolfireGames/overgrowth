#version 150
#extension GL_ARB_shading_language_420pack : enable

uniform float time;
uniform vec3 cam_pos;

#include "object_frag150.glsl"
#include "object_shared150.glsl"
#include "ambient_tet_mesh.glsl"

UNIFORM_COMMON_TEXTURES

UNIFORM_LIGHT_DIR

uniform sampler3D tex16;
uniform sampler2D tex17;
uniform sampler2D tex18;
uniform sampler2D tex5;

#if defined(NO_REFLECTION_CAPTURE)
	#define ref_cap_cubemap tex0
#else
	uniform sampler2DArray tex19;

	#define ref_cap_cubemap tex19
#endif

uniform mat4 reflection_capture_matrix[10];
uniform mat4 reflection_capture_matrix_inverse[10];
uniform int reflection_capture_num;

uniform mat4 light_volume_matrix[10];
uniform mat4 light_volume_matrix_inverse[10];
uniform int light_volume_num;
uniform mat4 prev_projection_view_mat;

uniform float haze_mult;

#define INSTANCED_MESH

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

uniform vec4 color_tint;
uniform samplerCube tex3;

#define ref_cap_cubemap tex19

struct Instance {
	mat4 model_mat;
	mat3 model_rotation_mat;
	vec4 color_tint;
	vec4 detail_scale;
};

uniform mat4 shadow_matrix[4];
uniform mat4 projection_view_mat;

in vec3 world_vert;
in vec2 frag_tex_coords;

#pragma bind_out_color
out vec4 out_color;

in vec2 tex_coord;
in vec2 base_tex_coord;
in vec3 orig_vert;
in mat3 tangent_to_world;
in vec3 frag_normal;
flat in int instance_id;
in mat3 tan_to_obj;
const float cloud_speed = 0.1;

uniform float overbright;

#include "decals.glsl"

#define shadow_tex_coords tc1
#define tc0 frag_tex_coords

void main() {
	vec4 colormap = texture(tex0, frag_tex_coords);
	out_color = colormap;

	float scale = 2.0;

	if(int(gl_FragCoord.x) % 2 != 0 || int(gl_FragCoord.y) % 2 != 0){
	// if(int((gl_FragCoord.x + gl_FragCoord.y) / scale) % 2 == 0 || int((gl_FragCoord.x - gl_FragCoord.y) / scale) % 2 == 0){
		discard;
	}
}
