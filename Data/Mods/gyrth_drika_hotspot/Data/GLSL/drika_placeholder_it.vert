#version 450 core

#og_version_major 1
#og_version_minor 5

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

in vec3 model_translation_attrib;  // set per-instance. separate from rest because it's not needed in the fragment shader, so is not slow on low-end GPUs

#if defined(ATTRIB_ENVOBJ_INSTANCING)
	in vec3 model_scale_attrib;
	in vec4 model_rotation_quat_attrib;
	in vec4 color_tint_attrib;
	in vec4 detail_scale_attrib;
#endif

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

uniform mat4 projection_view_mat;
uniform vec3 cam_pos;
uniform mat4 shadow_matrix[4];
uniform float time;

in vec3 vertex_attrib;
in vec2 tex_coord_attrib;

out vec2 frag_tex_coords;
out mat3 tangent_to_world;
out vec3 orig_vert;
flat out int instance_id;

out vec3 world_vert;
out vec3 model_position;

void main() {
	model_position = model_translation_attrib;

	instance_id = gl_InstanceID;

	vec3 transformed_vertex = transform_vec3(GetInstancedModelScale(instance_id), GetInstancedModelRotationQuat(instance_id), model_translation_attrib, vertex_attrib);

	vec3 temp = transformed_vertex.xyz;
	transformed_vertex.xyz = temp;
	frag_tex_coords = tex_coord_attrib;

	frag_tex_coords[1] = 1.0 - frag_tex_coords[1];

	world_vert = transformed_vertex;
	gl_Position = projection_view_mat * vec4(transformed_vertex, 1.0);
}
