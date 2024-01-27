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

uniform sampler2D tex1; // Fire noise texture.

uniform mat4 projection_view_mat;
uniform vec3 cam_pos;
uniform vec3 cam_dir;
uniform float time;
uniform mat4 shadow_matrix[4];

out vec2 frag_tex_coords;
out vec2 tex_coord;
out mat3 tangent_to_world;
out vec3 world_vert;
out vec3 model_position;
flat out int instance_id;

in vec3 vertex_attrib;
in vec2 tex_coord_attrib;

void main() {
	instance_id = gl_InstanceID;

	mat4 modelViewMatrix = instances[instance_id].model_mat;
	vec3 position = vertex_attrib;

	frag_tex_coords = tex_coord_attrib;
	tex_coord = tex_coord_attrib;
	tex_coord[1] = 1.0 - tex_coord[1];

	float horizontal_speed = 0.1;
	float vertical_speed = 0.5;

	vec4 noise_tex = texture(tex1, tex_coord + vec2(time * horizontal_speed, time * vertical_speed));
	vec3 model_scale = vec3(length(modelViewMatrix[0]), length(modelViewMatrix[1]), length(modelViewMatrix[2]));
	float height = frag_tex_coords.y;
	
	// Move the vertices towards the middle of the model based on height to make it pointy.
	position.x *= 1.0 - height / 1.5;

	// Add wobble using the noise texture and increase based on height.
	position.x += (noise_tex.r - 0.25) * 0.75 * min(1.0, height * 3.0);
	position.z += (noise_tex.r - 0.25) * 0.75 * min(1.0, height * 3.0);
	position.y += (noise_tex.r - 0.25) * 0.75 * min(1.0, height * 3.0);

	// The model world position of the center of the object
	vec3 model_position = (modelViewMatrix * vec4(vec3(0.0), 1.0)).xyz;

	#if defined(BILLBOARD)
		// Distance between the camera and the center.
		vec3 dist = cam_pos - model_position;

		// With atan the tree inverts when the camera has the same z position.
		float angle = atan(dist.x, dist.z);

		mat3 rotMatrix;
		float cosinus = cos(angle);
		float sinus = sin(-angle);

		// Rotation matrix in Y.
		rotMatrix[0].xyz = vec3(cosinus, 0, sinus);
		rotMatrix[1].xyz = vec3(0, 1, 0);
		rotMatrix[2].xyz = vec3(- sinus, 0, cosinus);

		// The position of the vertex after the rotation.
		position = rotMatrix * position;

		// The model matrix without the rotation.
		modelViewMatrix[0][0] = model_scale.x;
		modelViewMatrix[0][1] = 0;
		modelViewMatrix[0][2] = 0;

		modelViewMatrix[1][0] = 0;
		modelViewMatrix[1][1] = model_scale.y;
		modelViewMatrix[1][2] = 0;

		modelViewMatrix[2][0] = 0;
		modelViewMatrix[2][1] = 0;
		modelViewMatrix[2][2] = model_scale.z;
	#endif

	vec4 transformed_vertex = modelViewMatrix * vec4(position, 1.0);
	world_vert = transformed_vertex.xyz;
	gl_Position = projection_view_mat * transformed_vertex;
}
