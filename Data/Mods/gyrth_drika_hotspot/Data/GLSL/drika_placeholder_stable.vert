#version 150
#extension GL_ARB_shading_language_420pack : enable
#include "lighting150.glsl"

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

uniform mat4 projection_view_mat;
uniform vec3 cam_pos;
uniform mat4 shadow_matrix[4];
uniform float time;

out vec2 frag_tex_coords;
out mat3 tangent_to_world;
out vec3 orig_vert;
out vec2 tex_coord;
flat out int instance_id;
out vec3 frag_normal;
in vec3 normal_attrib;

in vec3 vertex_attrib;
in vec2 tex_coord_attrib;
vec3 transformed_vertex = (instances[gl_InstanceID].model_mat * vec4(vertex_attrib, 1.0)).xyz;
out vec3 world_vert;

void main() {
	instance_id = gl_InstanceID;
	frag_tex_coords = tex_coord_attrib;
 	frag_tex_coords[1] = 1.0 - frag_tex_coords[1];

	tex_coord = tex_coord_attrib;
	tex_coord[1] = 1.0 - tex_coord[1];

	world_vert = transformed_vertex;
	frag_normal = normal_attrib;

	instance_id = gl_InstanceID;
	orig_vert = vertex_attrib;

	gl_Position = projection_view_mat * vec4(transformed_vertex, 1.0);
}