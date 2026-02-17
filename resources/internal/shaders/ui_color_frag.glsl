#version 300 es

precision mediump float;
precision mediump int;

in vec2 tex_coord;
in vec4 instance_color;
in vec4 instance_border_color;
in vec3 instance_border_info;
in vec4 instance_effect;

out vec4 frag_color;
uniform float u_time;

void main() {
	vec4 fill = instance_color;
	vec4 border = instance_border_color;
	float blink_enabled = step(0.5, instance_effect.x);
	float blink_period = max(0.0001, instance_effect.y);
	float blink_duty = clamp(instance_effect.z, 0.0, 1.0);
	float blink_phase = instance_effect.w;
	float blink_t = fract((u_time + blink_phase) / blink_period);
	float blink_mask = mix(1.0, step(blink_t, blink_duty), blink_enabled);
	fill.w *= blink_mask;
	border.w *= blink_mask;
	float half_w = max(0.00001, instance_border_info.x);
	float half_h = max(0.00001, instance_border_info.y);
	float border_width = max(0.0, instance_border_info.z);
	float width = half_w * 2.0;
	float height = half_h * 2.0;
	float edge_dist = min(
		min(tex_coord.x, 1.0 - tex_coord.x) * width,
		min(tex_coord.y, 1.0 - tex_coord.y) * height);
	float border_mask = step(edge_dist, border_width);
	float has_border = step(0.00001, border_width) * step(0.00001, border.w);
	frag_color = mix(fill, border, border_mask * has_border);
	if (frag_color.w <= 0.00001) {
		discard;
	}
}
