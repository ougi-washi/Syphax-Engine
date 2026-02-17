#version 300 es

precision mediump float;
precision mediump int;

in vec2 tex_coord;
in vec4 glyph_uv;
in vec4 glyph_effect;

out vec4 frag_color;

uniform sampler2D u_atlas_texture;
uniform float u_time;

void main() {
    vec2 final_uv = glyph_uv.xy + tex_coord * (glyph_uv.zw - glyph_uv.xy);
    float alpha = texture(u_atlas_texture, final_uv).r;
    float blink_enabled = step(0.5, glyph_effect.x);
    float blink_period = max(0.0001, glyph_effect.y);
    float blink_duty = clamp(glyph_effect.z, 0.0, 1.0);
    float blink_phase = glyph_effect.w;
    float blink_t = fract((u_time + blink_phase) / blink_period);
    float blink_mask = mix(1.0, step(blink_t, blink_duty), blink_enabled);
    alpha *= blink_mask;
    if (alpha < 0.01) discard;
    frag_color = vec4(1.0, 1.0, 1.0, alpha);
}
