#version 150
#extension GL_ARB_shading_language_420pack : enable

uniform float time;
uniform vec3 cam_pos;

uniform sampler2D tex1; // Fire noise texture.
uniform sampler2D tex17; // Screen texture

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

in vec3 world_vert;

#pragma bind_out_color
out vec4 out_color;

in vec2 frag_tex_coords;
in vec2 tex_coord;
in vec2 base_tex_coord;
in vec3 orig_vert;
in mat3 tangent_to_world;
in vec3 model_position;
flat in int instance_id;

uniform mat4 shadow_matrix[4];
uniform float overbright;

const vec2 CONSTANT_OFFSET = vec2(0.2642, 0.7245);
const float HEAT_DISTORTION_AMOUNT = 0.07;
const float MIDDLE = 0.5;
const float EDGE_FADEOUT = 5.0;
const float FLAT_AMOUNT = 0.095;

#if defined(SMALL_FLAME)
	const float HORIZONTAL_SPEED = 0.05;
	const float VERTICAL_SPEED = 0.5;
	const float UV_DIVIDE = 4.0;
#else
	const float HORIZONTAL_SPEED = 0.1;
	const float VERTICAL_SPEED = 1.75;
	const float UV_DIVIDE = 1.0;
#endif

void main() {
	vec2 model_offset = model_position.xz;
	vec4 fire_tex_1 = texture(tex1, model_offset + tex_coord / UV_DIVIDE + CONSTANT_OFFSET + vec2(time * HORIZONTAL_SPEED, time * VERTICAL_SPEED));
	vec4 fire_tex_2 = texture(tex1, model_offset + tex_coord / UV_DIVIDE + vec2(time * HORIZONTAL_SPEED, time * VERTICAL_SPEED));

	vec4 colormap = (fire_tex_1 + fire_tex_2) / 2.0;
	vec2 heat_offset = colormap.rg * HEAT_DISTORTION_AMOUNT;

	float edge = min((tex_coord.x > 0.5) ? (1.0 - tex_coord.x) : (tex_coord.x), (tex_coord.y > 0.5) ? (1.0 - tex_coord.y) : (tex_coord.y));
	float edge_detect = min(1.0, edge / MIDDLE * EDGE_FADEOUT);
	float height = max(0.0, tex_coord.y);

	// Make the flame more transparent at the top, bottom and side edges.
	colormap *= edge_detect;
	// Also reduce the color the higher the flame goes.
	colormap *= height;
	// Make the flame look more cartoony by flattening the color.
	#if defined(FLAT)
		vec3 flat_amount = vec3(step(FLAT_AMOUNT, colormap.r));
		colormap.rgb *= flat_amount;
	#endif

	float alpha = smoothstep(0.0, 1.0, colormap.r);
	// Start tinting the fire and multiply by the overbright.
	vec4 fire_color = instances[instance_id].color_tint;
	colormap.rgb = mix(fire_color.rgb, vec3(1.0), colormap.r) * length(fire_color);

	vec4 proj_test_point = (projection_view_mat * vec4(world_vert, 1.0));
	proj_test_point /= proj_test_point.w;
	proj_test_point.xy += vec2(1.0);
	proj_test_point.xy *= 0.5;

	#if defined(DITHER) || defined(SIMPLE_WATER)
		vec3 screen_texture = vec3(0.0);
	#elif defined(HEAT_DISTORTION)
		// Remove the heat destortion at the edges so it smoothly fades in.
		heat_offset *= edge_detect;
		// heat_offset *= height;
		vec3 screen_texture = textureLod(tex17, proj_test_point.xy + heat_offset, 0.0).xyz;
	#else
		vec3 screen_texture = texture(tex17, proj_test_point.xy).xyz; // color texture from opaque objects
	#endif
	
	out_color.xyz = mix(screen_texture, colormap.xyz, alpha);

	// When simple water is enabled, the screentexture isn't available anymore.
	// So switch to dithering to keep showing a convincing flame.
	#if defined(DITHER) || defined(SIMPLE_WATER)
		int x = int(gl_FragCoord.x) % 4;
		int y = int(gl_FragCoord.y) % 4;
		int index = x + y * 4;
		float limit = 0.0;

		if(x < 8){
			if (index == 0) limit = 0.0625;
			if (index == 1) limit = 0.5625;
			if (index == 2) limit = 0.1875;
			if (index == 3) limit = 0.6875;
			if (index == 4) limit = 0.8125;
			if (index == 5) limit = 0.3125;
			if (index == 6) limit = 0.9375;
			if (index == 7) limit = 0.4375;
			if (index == 8) limit = 0.25;
			if (index == 9) limit = 0.75;
			if (index == 10) limit = 0.125;
			if (index == 11) limit = 0.625;
			if (index == 12) limit = 1.0;
			if (index == 13) limit = 0.5;
			if (index == 14) limit = 0.875;
			if (index == 15) limit = 0.375;
		}

		if(out_color.r < limit){
			discard;
		}
	#endif
}
