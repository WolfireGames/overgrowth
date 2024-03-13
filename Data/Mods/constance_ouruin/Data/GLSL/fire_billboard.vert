#version 150
#extension GL_ARB_shading_language_420pack : enable

#include "lighting150.glsl"

vec3 quat_mul_vec3(vec4 q, vec3 v) {
    // Adapted from https://github.com/g-truc/glm/blob/master/glm/detail/type_quat.inl
    // Also from Fabien Giesen, according to - https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
    vec3 quat_vector = q.xyz;
    vec3 uv = cross(quat_vector, v);
    vec3 uuv = cross(quat_vector, uv);
    return v + ((uv * q.w) + uuv) * 2;
}

vec3 transform_vec3(vec3 scale, vec4 rotation_quat, vec3 translation, vec3 value) {
    vec3 result = scale * value;
    result = quat_mul_vec3(rotation_quat, result);
    result += translation;
    return result;
}

#if !defined(ATTRIB_ENVOBJ_INSTANCING)
	#if defined(UBO_BATCH_SIZE_8X)
		const int kMaxInstances = 256 * 8;
	#elif defined(UBO_BATCH_SIZE_4X)
		const int kMaxInstances = 256 * 4;
	#elif defined(UBO_BATCH_SIZE_2X)
		const int kMaxInstances = 256 * 2;
	#else
		const int kMaxInstances = 256 * 1;
	#endif

	struct Instance {
		vec3 model_scale;
		vec4 model_rotation_quat;
		vec4 color_tint;
		vec4 detail_scale;  // TODO: DETAILMAP4 only?
	};

	uniform InstanceInfo {
		Instance instances[kMaxInstances];
	};
#endif

vec3 GetInstancedModelScale(int instance_id) {
	#if defined(ATTRIB_ENVOBJ_INSTANCING)
		return model_scale_attrib;
	#else
		return instances[instance_id].model_scale;
	#endif
}

vec4 GetInstancedModelRotationQuat(int instance_id) {
	#if defined(ATTRIB_ENVOBJ_INSTANCING)
		return model_rotation_quat_attrib;
	#else
		return instances[instance_id].model_rotation_quat;
	#endif
}

vec4 GetInstancedColorTint(int instance_id) {
	#if defined(ATTRIB_ENVOBJ_INSTANCING)
		return color_tint_attrib;
	#else
		return instances[instance_id].color_tint;
	#endif
}

#if defined(DETAILMAP4)
	vec4 GetInstancedDetailScale(int instance_id) {
		#if defined(ATTRIB_ENVOBJ_INSTANCING)
			return detail_scale_attrib;
		#else
			return instances[instance_id].detail_scale;
		#endif
	}
#endif

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
in vec3 model_translation_attrib;  // set per-instance. separate from rest because it's not needed in the fragment shader, so is not slow on low-end GPUs

in vec3 vertex_attrib;
in vec2 tex_coord_attrib;

void main() {
	instance_id = gl_InstanceID;

	vec3 position = vertex_attrib;

	frag_tex_coords = tex_coord_attrib;
	tex_coord = tex_coord_attrib;
	tex_coord[1] = 1.0 - tex_coord[1];

	float horizontal_speed = 0.1;
	float vertical_speed = 0.5;

	vec4 noise_tex = texture(tex1, tex_coord + vec2(time * horizontal_speed, time * vertical_speed));
	vec3 model_scale = GetInstancedModelScale(instance_id);
	float height = frag_tex_coords.y;
	
	// Move the vertices towards the middle of the model based on height to make it pointy.
	position.x *= 1.0 - height / 1.5;

	// Add wobble using the noise texture and increase based on height.
	position.x += (noise_tex.r - 0.25) * 0.75 * min(1.0, height * 3.0);
	position.z += (noise_tex.r - 0.25) * 0.75 * min(1.0, height * 3.0);
	position.y += (noise_tex.r - 0.25) * 0.75 * min(1.0, height * 3.0);

	// The model world position of the center of the object
	model_position = model_translation_attrib;
	vec4 rotation_quat = GetInstancedModelRotationQuat(instance_id);

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

		// Use an empty rotation quat when it's a billboard.
		rotation_quat = vec4(0.0, 0.0, 0.0, 1.0);
	#endif

	vec3 transformed_vertex = transform_vec3(GetInstancedModelScale(instance_id), rotation_quat, model_translation_attrib, position);
	world_vert = transformed_vertex.xyz;
	gl_Position = projection_view_mat * vec4(transformed_vertex, 1.0);
}
