// Syphax-Engine - Ougi Washi

#include "se_sdf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// ============================================================================
// String Buffer
// ============================================================================

void se_sdf_string_init(se_sdf_string* str, sz capacity) {
    str->data = (char*)malloc(capacity);
    str->capacity = capacity;
    str->size = 0;
    if (str->data) str->data[0] = '\0';
}

void se_sdf_string_free(se_sdf_string* str) {
    if (str->data) free(str->data);
    str->data = NULL;
    str->capacity = 0;
    str->size = 0;
}

void se_sdf_string_append(se_sdf_string* str, const char* fmt, ...) {
    if (!str->data || str->size >= str->capacity - 1) return;
    
    va_list args;
    va_start(args, fmt);
    sz remaining = str->capacity - str->size - 1;
    sz written = vsnprintf(str->data + str->size, remaining, fmt, args);
    va_end(args);
    
    if (written > 0 && (sz)written < remaining) {
        str->size += written;
    }
}

void se_sdf_string_append_vec2(se_sdf_string* str, s_vec2 v) {
    se_sdf_string_append(str, "vec2(%.6f, %.6f)", v.x, v.y);
}

void se_sdf_string_append_vec3(se_sdf_string* str, s_vec3 v) {
    se_sdf_string_append(str, "vec3(%.6f, %.6f, %.6f)", v.x, v.y, v.z);
}

// ============================================================================
// Shape Generators
// ============================================================================

void se_sdf_gen_sphere(se_sdf_string* out, const char* p, f32 radius) {
    se_sdf_string_append(out, "(length(%s) - %.6f)", p, radius);
}

void se_sdf_gen_box(se_sdf_string* out, const char* p, s_vec3 size) {
    se_sdf_string_append(out, "{ vec3 q = abs(%s) - ", p);
    se_sdf_string_append_vec3(out, size);
    se_sdf_string_append(out, "; length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }");
}

void se_sdf_gen_round_box(se_sdf_string* out, const char* p, s_vec3 size, f32 roundness) {
    se_sdf_string_append(out, "{ vec3 q = abs(%s) - ", p);
    se_sdf_string_append_vec3(out, size);
    se_sdf_string_append(out, " + vec3(%.6f); length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - %.6f; }", 
                          roundness, roundness);
}

void se_sdf_gen_box_frame(se_sdf_string* out, const char* p, s_vec3 size, f32 thickness) {
    se_sdf_string_append(out, "{ vec3 q = abs(%s) - ", p);
    se_sdf_string_append_vec3(out, size);
    se_sdf_string_append(out, "; vec3 w = abs(q + vec3(%.6f)) - vec3(%.6f); ", thickness, thickness);
    se_sdf_string_append(out, "min(min(length(max(vec3(q.x, w.y, w.z), 0.0)) + min(max(q.x, max(w.y, w.z)), 0.0), ");
    se_sdf_string_append(out, "length(max(vec3(w.x, q.y, w.z), 0.0)) + min(max(w.x, max(q.y, w.z)), 0.0)), ");
    se_sdf_string_append(out, "length(max(vec3(w.x, w.y, q.z), 0.0)) + min(max(w.x, max(w.y, q.z)), 0.0)); }");
}

void se_sdf_gen_torus(se_sdf_string* out, const char* p, f32 major_r, f32 minor_r) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz) - %.6f, %s.y); length(q) - %.6f; }",
                          p, major_r, p, minor_r);
}

void se_sdf_gen_capped_torus(se_sdf_string* out, const char* p, s_vec2 sc, f32 ra, f32 rb) {
    se_sdf_string_append(out, "{ vec3 pp = %s; pp.x = abs(pp.x); ", p);
    se_sdf_string_append(out, "float k = (%.6f * pp.x > %.6f * pp.y) ? dot(pp.xy, vec2(%.6f, %.6f)) : length(pp.xy); ",
                          sc.y, sc.x, sc.x, sc.y);
    se_sdf_string_append(out, "sqrt(dot(pp, pp) + %.6f * %.6f - 2.0 * %.6f * k) - %.6f; }",
                          ra, ra, ra, rb);
}

void se_sdf_gen_link(se_sdf_string* out, const char* p, f32 le, f32 r1, f32 r2) {
    se_sdf_string_append(out, "{ vec3 q = vec3(%s.x, max(abs(%s.y) - %.6f, 0.0), %s.z); ", p, p, le, p);
    se_sdf_string_append(out, "length(vec2(length(q.xy) - %.6f, q.z)) - %.6f; }", r1, r2);
}

void se_sdf_gen_cylinder(se_sdf_string* out, const char* p, s_vec3 c) {
    se_sdf_string_append(out, "(length(%s.xz - vec2(%.6f, %.6f)) - %.6f)",
                          p, c.x, c.y, c.z);
}

void se_sdf_gen_cone(se_sdf_string* out, const char* p, s_vec2 sc, f32 h) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "vec2 k1 = vec2(%.6f, %.6f); ", sc.y, h);
    se_sdf_string_append(out, "vec2 k2 = vec2(%.6f - %.6f, 2.0 * %.6f); ", sc.y, sc.x, h);
    se_sdf_string_append(out, "vec2 ca = vec2(q.x - min(q.x, (q.y < 0.0) ? %.6f : %.6f), abs(q.y) - %.6f); ",
                          sc.x, sc.y, h);
    se_sdf_string_append(out, "vec2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot(k2, k2), 0.0, 1.0); ");
    se_sdf_string_append(out, "float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0; ");
    se_sdf_string_append(out, "s * sqrt(min(dot(ca, ca), dot(cb, cb))); }");
}

void se_sdf_gen_cone_inf(se_sdf_string* out, const char* p, s_vec2 sc) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), -%s.y); ", p, p);
    se_sdf_string_append(out, "vec2 w = q - vec2(%.6f, %.6f) * max(dot(q, vec2(%.6f, %.6f)), 0.0); ",
                          sc.x, sc.y, sc.x, sc.y);
    se_sdf_string_append(out, "float d = length(w); ");
    se_sdf_string_append(out, "d * ((q.x * %.6f - q.y * %.6f < 0.0) ? -1.0 : 1.0); }", sc.y, sc.x);
}

void se_sdf_gen_plane(se_sdf_string* out, const char* p, s_vec3 n, f32 h) {
    se_sdf_string_append(out, "(dot(%s, vec3(%.6f, %.6f, %.6f)) + %.6f)",
                          p, n.x, n.y, n.z, h);
}

void se_sdf_gen_hex_prism(se_sdf_string* out, const char* p, s_vec2 h) {
    se_sdf_string_append(out, "{ const vec3 k = vec3(-0.8660254, 0.5, 0.57735); ");
    se_sdf_string_append(out, "vec3 pp = abs(%s); ", p);
    se_sdf_string_append(out, "pp.xy -= 2.0 * min(dot(k.xy, pp.xy), 0.0) * k.xy; ");
    se_sdf_string_append(out, "vec2 d = vec2(length(pp.xy - vec2(clamp(pp.x, -k.z * %.6f, k.z * %.6f), %.6f)) * sign(pp.y - %.6f), pp.z - %.6f); ",
                          h.x, h.x, h.x, h.x, h.y);
    se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)); }");
}

void se_sdf_gen_capsule(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r) {
    se_sdf_string_append(out, "{ vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); ");
    se_sdf_string_append(out, "length(pa - ba * h) - %.6f; }", r);
}

void se_sdf_gen_vert_capsule(se_sdf_string* out, const char* p, f32 h, f32 r) {
    se_sdf_string_append(out, "{ vec3 q = %s; q.y -= clamp(q.y, 0.0, %.6f); length(q) - %.6f; }",
                          p, h, r);
}

void se_sdf_gen_capped_cyl(se_sdf_string* out, const char* p, f32 r, f32 hh) {
    se_sdf_string_append(out, "{ vec2 d = abs(vec2(length(%s.xz), %s.y)) - vec2(%.6f, %.6f); ");
    se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)); }", p, p, r, hh);
}

void se_sdf_gen_capped_cyl_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float baba = dot(ba, ba); float paba = dot(pa, ba); ");
    se_sdf_string_append(out, "float x = length(pa * baba - ba * paba) - %.6f * baba; ", r);
    se_sdf_string_append(out, "float y = abs(paba - baba * 0.5) - baba * 0.5; ");
    se_sdf_string_append(out, "float x2 = x * x; float y2 = y * y * baba; ");
    se_sdf_string_append(out, "float d = (max(x, y) < 0.0) ? -min(x2, y2) : (((x > 0.0) ? x2 : 0.0) + ((y > 0.0) ? y2 : 0.0)); ");
    se_sdf_string_append(out, "sign(d) * sqrt(abs(d)) / baba; }");
}

void se_sdf_gen_rounded_cyl(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 hh) {
    se_sdf_string_append(out, "{ vec2 d = vec2(length(%s.xz) - %.6f + %.6f, abs(%s.y) - %.6f + %.6f); ");
    se_sdf_string_append(out, "min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - %.6f; }",
                          p, ra, rb, p, hh, rb, rb);
}

void se_sdf_gen_capped_cone(se_sdf_string* out, const char* p, f32 h, f32 r1, f32 r2) {
    se_sdf_gen_cone(out, p, (s_vec2){r1, h}, h); // Simplified
}

void se_sdf_gen_capped_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 ra, f32 rb) {
    se_sdf_string_append(out, "{ float rba = %.6f - %.6f; ", rb, ra);
    se_sdf_string_append(out, "vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float baba = dot(ba, ba); vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float paba = dot(pa, ba) / baba; ");
    se_sdf_string_append(out, "float x = length(pa - ba * paba); ");
    se_sdf_string_append(out, "float cax = max(0.0, x - ((paba < 0.5) ? %.6f : %.6f)); ", ra, rb);
    se_sdf_string_append(out, "float cay = abs(paba - 0.5) - 0.5; ");
    se_sdf_string_append(out, "float k = rba * rba + baba; ");
    se_sdf_string_append(out, "float f = clamp((rba * (x - %.6f) + paba * baba) / k, 0.0, 1.0); ", ra);
    se_sdf_string_append(out, "float cbx = x - %.6f - f * rba; ", ra);
    se_sdf_string_append(out, "float cby = paba - f; ");
    se_sdf_string_append(out, "float s = (cbx < 0.0 && cay < 0.0) ? -1.0 : 1.0; ");
    se_sdf_string_append(out, "s * sqrt(min(cax * cax + cay * cay * baba, cbx * cbx + cby * cby * baba)); }");
}

void se_sdf_gen_solid_angle(se_sdf_string* out, const char* p, s_vec2 sc, f32 r) {
    se_sdf_string_append(out, "{ vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "float l = length(q) - %.6f; ", r);
    se_sdf_string_append(out, "float m = length(q - vec2(%.6f, %.6f) * clamp(dot(q, vec2(%.6f, %.6f)), 0.0, %.6f)); ",
                          sc.x, sc.y, sc.x, sc.y, r);
    se_sdf_string_append(out, "max(l, m * sign(%.6f * q.x - %.6f * q.y)); }", sc.y, sc.x);
}

void se_sdf_gen_cut_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch) {
    se_sdf_string_append(out, "{ float w = sqrt(%.6f * %.6f - %.6f * %.6f); ", r, r, ch, ch);
    se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "float s = max((%.6f - %.6f) * q.x * q.x + w * w * (%.6f + %.6f - 2.0 * q.y), %.6f * q.x - w * q.y); ",
                          ch, r, ch, r, ch);
    se_sdf_string_append(out, "(s < 0.0) ? length(q) - %.6f : (q.x < w) ? %.6f - q.y : length(q - vec2(w, %.6f)); }",
                          r, ch, ch);
}

void se_sdf_gen_cut_hollow_sphere(se_sdf_string* out, const char* p, f32 r, f32 ch, f32 t) {
    se_sdf_string_append(out, "{ float w = sqrt(%.6f * %.6f - %.6f * %.6f); ", r, r, ch, ch);
    se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "((%.6f * q.x < w * q.y) ? length(q - vec2(w, %.6f)) : abs(length(q) - %.6f)) - %.6f; }",
                          ch, ch, r, t);
}

void se_sdf_gen_death_star(se_sdf_string* out, const char* p, f32 ra, f32 rb, f32 d) {
    se_sdf_string_append(out, "{ float a = (%.6f * %.6f - %.6f * %.6f + %.6f * %.6f) / (2.0 * %.6f); ",
                          ra, ra, rb, rb, d, d, d);
    se_sdf_string_append(out, "float b = sqrt(max(%.6f * %.6f - a * a, 0.0)); ", ra, ra);
    se_sdf_string_append(out, "vec2 pp = vec2(%s.x, length(%s.yz)); ", p, p);
    se_sdf_string_append(out, "if (pp.x * b - pp.y * a > %.6f * max(b - pp.y, 0.0)) ");
    se_sdf_string_append(out, "length(pp - vec2(a, b)); ");
    se_sdf_string_append(out, "else max(length(pp) - %.6f, -(length(pp - vec2(%.6f, 0.0)) - %.6f)); }",
                          ra, d, rb);
}

void se_sdf_gen_round_cone(se_sdf_string* out, const char* p, f32 r1, f32 r2, f32 h) {
    se_sdf_string_append(out, "{ float b = (%.6f - %.6f) / %.6f; ", r1, r2, h);
    se_sdf_string_append(out, "float a = sqrt(1.0 - b * b); ");
    se_sdf_string_append(out, "vec2 q = vec2(length(%s.xz), %s.y); ", p, p);
    se_sdf_string_append(out, "float k = dot(q, vec2(-b, a)); ");
    se_sdf_string_append(out, "if (k < 0.0) length(q) - %.6f; ", r1);
    se_sdf_string_append(out, "else if (k > a * %.6f) length(q - vec2(0.0, %.6f)) - %.6f; ", h, h, r2);
    se_sdf_string_append(out, "else dot(q, vec2(a, b)) - %.6f; }", r1);
}

void se_sdf_gen_round_cone_arb(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 r1, f32 r2) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float l2 = dot(ba, ba); float rr = %.6f - %.6f; float a2 = l2 - rr * rr; float il2 = 1.0 / l2; ",
                          r1, r2);
    se_sdf_string_append(out, "vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; float y = dot(pa, ba); float z = y - l2; ");
    se_sdf_string_append(out, "float x2 = dot(pa * l2 - ba * y, pa * l2 - ba * y); ");
    se_sdf_string_append(out, "float y2 = y * y * l2; float z2 = z * z * l2; ");
    se_sdf_string_append(out, "float k = sign(rr) * rr * rr * x2; ");
    se_sdf_string_append(out, "if (sign(z) * a2 * z2 > k) sqrt(x2 + z2) * il2 - %.6f; ", r2);
    se_sdf_string_append(out, "else if (sign(y) * a2 * y2 < k) sqrt(x2 + y2) * il2 - %.6f; ", r1);
    se_sdf_string_append(out, "else (sqrt(x2 * a2 * il2) + y * rr) * il2 - %.6f; }", r1);
}

void se_sdf_gen_vesica(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, f32 w) {
    se_sdf_string_append(out, "{ vec3 c = (");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, " + ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, ") * 0.5; float l = length(");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "); vec3 v = (");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, ") / l; float y = dot(%s - c, v); vec2 q = vec2(length(%s - c - y * v), abs(y)); ",
                          p, p);
    se_sdf_string_append(out, "float r = 0.5 * l; float d = 0.5 * (r * r - %.6f * %.6f) / %.6f; ", w, w, w);
    se_sdf_string_append(out, "vec3 h = (r * q.x < d * (q.y - r)) ? vec3(0.0, r, 0.0) : vec3(-d, 0.0, d + %.6f); ", w);
    se_sdf_string_append(out, "length(q - h.xy) - h.z; }");
}

void se_sdf_gen_rhombus(se_sdf_string* out, const char* p, f32 la, f32 lb, f32 h, f32 r) {
    se_sdf_string_append(out, "{ vec3 pp = abs(%s); ", p);
    se_sdf_string_append(out, "float f = clamp((%.6f * pp.x - %.6f * pp.z + %.6f * %.6f) / (%.6f * %.6f + %.6f * %.6f), 0.0, 1.0); ",
                          la, lb, lb, lb, la, la, lb, lb);
    se_sdf_string_append(out, "vec2 w = pp.xz - vec2(%.6f, %.6f) * vec2(f, 1.0 - f); ", la, lb);
    se_sdf_string_append(out, "vec2 q = vec2(length(w) * sign(w.x) - %.6f, pp.y - %.6f); ", r, h);
    se_sdf_string_append(out, "min(max(q.x, q.y), 0.0) + length(max(q, 0.0)); }");
}

void se_sdf_gen_octa(se_sdf_string* out, const char* p, f32 s) {
    se_sdf_string_append(out, "{ vec3 pp = abs(%s); ", p);
    se_sdf_string_append(out, "float m = pp.x + pp.y + pp.z - %.6f; ", s);
    se_sdf_string_append(out, "vec3 q; if (3.0 * pp.x < m) q = pp.xyz; else if (3.0 * pp.y < m) q = pp.yzx; ");
    se_sdf_string_append(out, "else if (3.0 * pp.z < m) q = pp.zxy; else return m * 0.57735027; ");
    se_sdf_string_append(out, "float k = clamp(0.5 * (q.z - q.y + %.6f), 0.0, %.6f); ", s, s);
    se_sdf_string_append(out, "length(vec3(q.x, q.y - %.6f + k, q.z - k)); }", s);
}

void se_sdf_gen_octa_bound(se_sdf_string* out, const char* p, f32 s) {
    se_sdf_string_append(out, "{ vec3 pp = abs(%s); (pp.x + pp.y + pp.z - %.6f) * 0.57735027; }",
                          p, s);
}

void se_sdf_gen_pyramid(se_sdf_string* out, const char* p, f32 h) {
    se_sdf_string_append(out, "{ float m2 = %.6f * %.6f + 0.25; ", h, h);
    se_sdf_string_append(out, "vec3 pp = %s; pp.xz = abs(pp.xz); ", p);
    se_sdf_string_append(out, "if (pp.z > pp.x) pp.xz = pp.zx; pp.xz -= 0.5; ");
    se_sdf_string_append(out, "vec3 q = vec3(pp.z, %.6f * pp.y - 0.5 * pp.x, %.6f * pp.x + 0.5 * pp.y); ", h, h);
    se_sdf_string_append(out, "float s = max(-q.x, 0.0); ");
    se_sdf_string_append(out, "float t = clamp((q.y - 0.5 * pp.z) / (m2 + 0.25), 0.0, 1.0); ");
    se_sdf_string_append(out, "float a = m2 * (q.x + s) * (q.x + s) + q.y * q.y; ");
    se_sdf_string_append(out, "float b = m2 * (q.x + 0.5 * t) * (q.x + 0.5 * t) + (q.y - m2 * t) * (q.y - m2 * t); ");
    se_sdf_string_append(out, "float d2 = (min(q.y, -q.x * m2 - q.y * 0.5) > 0.0) ? 0.0 : min(a, b); ");
    se_sdf_string_append(out, "sqrt((d2 + q.z * q.z) / m2) * sign(max(q.z, -pp.y)); }");
}

void se_sdf_gen_tri(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 cb = ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 pb = %s - ", p);
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 ac = ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 pc = %s - ", p);
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 nor = cross(ba, ac); ");
    se_sdf_string_append(out, "sqrt((sign(dot(cross(ba, nor), pa)) + sign(dot(cross(cb, nor), pb)) + ");
    se_sdf_string_append(out, "sign(dot(cross(ac, nor), pc)) < 2.0) ? min(min(");
    se_sdf_string_append(out, "dot(ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa, ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa), ");
    se_sdf_string_append(out, "dot(cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb, cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb)), ");
    se_sdf_string_append(out, "dot(ac * clamp(dot(ac, pc) / dot(ac, ac), 0.0, 1.0) - pc, ac * clamp(dot(ac, pc) / dot(ac, ac), 0.0, 1.0) - pc)) ");
    se_sdf_string_append(out, ": dot(nor, pa) * dot(nor, pa) / dot(nor, nor)); }");
}

void se_sdf_gen_quad(se_sdf_string* out, const char* p, s_vec3 a, s_vec3 b, s_vec3 c, s_vec3 d) {
    se_sdf_string_append(out, "{ vec3 ba = ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 pa = %s - ", p);
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, "; vec3 cb = ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 pb = %s - ", p);
    se_sdf_string_append_vec3(out, b);
    se_sdf_string_append(out, "; vec3 dc = ");
    se_sdf_string_append_vec3(out, d);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 pc = %s - ", p);
    se_sdf_string_append_vec3(out, c);
    se_sdf_string_append(out, "; vec3 ad = ");
    se_sdf_string_append_vec3(out, a);
    se_sdf_string_append(out, " - ");
    se_sdf_string_append_vec3(out, d);
    se_sdf_string_append(out, "; vec3 pd = %s - ", p);
    se_sdf_string_append_vec3(out, d);
    se_sdf_string_append(out, "; vec3 nor = cross(ba, ad); ");
    se_sdf_string_append(out, "sqrt((sign(dot(cross(ba, nor), pa)) + sign(dot(cross(cb, nor), pb)) + ");
    se_sdf_string_append(out, "sign(dot(cross(dc, nor), pc)) + sign(dot(cross(ad, nor), pd)) < 3.0) ? ");
    se_sdf_string_append(out, "min(min(min(dot(ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa, ba * clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa), ");
    se_sdf_string_append(out, "dot(cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb, cb * clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb)), ");
    se_sdf_string_append(out, "dot(dc * clamp(dot(dc, pc) / dot(dc, dc), 0.0, 1.0) - pc, dc * clamp(dot(dc, pc) / dot(dc, dc), 0.0, 1.0) - pc)), ");
    se_sdf_string_append(out, "dot(ad * clamp(dot(ad, pd) / dot(ad, ad), 0.0, 1.0) - pd, ad * clamp(dot(ad, pd) / dot(ad, ad), 0.0, 1.0) - pd)) ");
    se_sdf_string_append(out, ": dot(nor, pa) * dot(nor, pa) / dot(nor, nor)); }");
}

// ============================================================================
// Transform & Operations
// ============================================================================

void se_sdf_gen_transform(se_sdf_string* out, const char* p_var, s_mat4 transform, const char* result_var) {
    // Generate: vec3 result_var = (inverse(mat4(...)) * vec4(p_var, 1.0)).xyz;
    se_sdf_string_append(out, "mat4 _t = mat4(");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            se_sdf_string_append(out, "%.6f", transform.m[i][j]);
            if (i != 3 || j != 3) se_sdf_string_append(out, ", ");
        }
    }
    se_sdf_string_append(out, "); ");
    se_sdf_string_append(out, "vec3 %s = (inverse(_t) * vec4(%s, 1.0)).xyz; ", result_var, p_var);
}

void se_sdf_gen_operation(se_sdf_string* out, se_sdf_operation op, const char* d1, const char* d2) {
    switch (op) {
        case SE_SDF_OP_UNION:
            se_sdf_string_append(out, "min(%s, %s)", d1, d2);
            break;
        case SE_SDF_OP_INTERSECTION:
            se_sdf_string_append(out, "max(%s, %s)", d1, d2);
            break;
        case SE_SDF_OP_SUBTRACTION:
            se_sdf_string_append(out, "max(%s, -%s)", d1, d2);
            break;
        default:
            se_sdf_string_append(out, "%s", d1);
            break;
    }
}

// ============================================================================
// Recursive Generator
// ============================================================================

static sz se_sdf_count_children(se_sdf_object* obj) {
    return s_array_get_size(&obj->children);
}

static se_sdf_object* se_sdf_get_child(se_sdf_object* obj, sz index) {
    s_handle h = s_array_handle(&obj->children, (u32)index);
    return s_array_get(&obj->children, h);
}

void se_sdf_object_to_string(se_sdf_string* out, se_sdf_object* obj, const char* p_var) {
    // Handle transform
    char transformed_p[64];
    snprintf(transformed_p, sizeof(transformed_p), "_p%p", (void*)obj);
    
    b8 has_transform = 0;
    for (int i = 0; i < 4 && !has_transform; i++) {
        for (int j = 0; j < 4 && !has_transform; j++) {
            if ((i == j && obj->transform.m[i][j] != 1.0f) ||
                (i != j && obj->transform.m[i][j] != 0.0f)) {
                has_transform = 1;
            }
        }
    }
    
    if (has_transform) {
        se_sdf_gen_transform(out, p_var, obj->transform, transformed_p);
        p_var = transformed_p;
    }
    
    // Generate distance for this shape
    sz child_count = se_sdf_count_children(obj);
    
    if (child_count == 0) {
        // Leaf node - generate shape distance
        switch (obj->type) {
            case SE_SDF_SPHERE:
                se_sdf_gen_sphere(out, p_var, obj->sphere.radius);
                break;
            case SE_SDF_BOX:
                se_sdf_gen_box(out, p_var, obj->box.size);
                break;
            case SE_SDF_ROUND_BOX:
                se_sdf_gen_round_box(out, p_var, obj->round_box.size, obj->round_box.roundness);
                break;
            case SE_SDF_BOX_FRAME:
                se_sdf_gen_box_frame(out, p_var, obj->box_frame.size, obj->box_frame.thickness);
                break;
            case SE_SDF_TORUS:
                se_sdf_gen_torus(out, p_var, obj->torus.radii.x, obj->torus.radii.y);
                break;
            case SE_SDF_CAPPED_TORUS:
                se_sdf_gen_capped_torus(out, p_var, obj->capped_torus.axis_sin_cos, 
                                       obj->capped_torus.major_radius, obj->capped_torus.minor_radius);
                break;
            case SE_SDF_LINK:
                se_sdf_gen_link(out, p_var, obj->link.half_length, 
                               obj->link.outer_radius, obj->link.inner_radius);
                break;
            case SE_SDF_CYLINDER:
                se_sdf_gen_cylinder(out, p_var, obj->cylinder.axis_and_radius);
                break;
            case SE_SDF_CONE:
                se_sdf_gen_cone(out, p_var, obj->cone.angle_sin_cos, obj->cone.height);
                break;
            case SE_SDF_CONE_INFINITE:
                se_sdf_gen_cone_inf(out, p_var, obj->cone_infinite.angle_sin_cos);
                break;
            case SE_SDF_PLANE:
                se_sdf_gen_plane(out, p_var, obj->plane.normal, obj->plane.offset);
                break;
            case SE_SDF_HEX_PRISM:
                se_sdf_gen_hex_prism(out, p_var, obj->hex_prism.half_size);
                break;
            case SE_SDF_CAPSULE:
                se_sdf_gen_capsule(out, p_var, obj->capsule.a, obj->capsule.b, obj->capsule.radius);
                break;
            case SE_SDF_VERTICAL_CAPSULE:
                se_sdf_gen_vert_capsule(out, p_var, obj->vertical_capsule.height, obj->vertical_capsule.radius);
                break;
            case SE_SDF_CAPPED_CYLINDER:
                se_sdf_gen_capped_cyl(out, p_var, obj->capped_cylinder.radius, obj->capped_cylinder.half_height);
                break;
            case SE_SDF_CAPPED_CYLINDER_ARBITRARY:
                se_sdf_gen_capped_cyl_arb(out, p_var, obj->capped_cylinder_arbitrary.a,
                                          obj->capped_cylinder_arbitrary.b, obj->capped_cylinder_arbitrary.radius);
                break;
            case SE_SDF_ROUNDED_CYLINDER:
                se_sdf_gen_rounded_cyl(out, p_var, obj->rounded_cylinder.outer_radius,
                                      obj->rounded_cylinder.corner_radius, obj->rounded_cylinder.half_height);
                break;
            case SE_SDF_CAPPED_CONE:
                se_sdf_gen_capped_cone(out, p_var, obj->capped_cone.height,
                                      obj->capped_cone.radius_base, obj->capped_cone.radius_top);
                break;
            case SE_SDF_CAPPED_CONE_ARBITRARY:
                se_sdf_gen_capped_cone_arb(out, p_var, obj->capped_cone_arbitrary.a,
                                          obj->capped_cone_arbitrary.b, obj->capped_cone_arbitrary.radius_a,
                                          obj->capped_cone_arbitrary.radius_b);
                break;
            case SE_SDF_SOLID_ANGLE:
                se_sdf_gen_solid_angle(out, p_var, obj->solid_angle.angle_sin_cos, obj->solid_angle.radius);
                break;
            case SE_SDF_CUT_SPHERE:
                se_sdf_gen_cut_sphere(out, p_var, obj->cut_sphere.radius, obj->cut_sphere.cut_height);
                break;
            case SE_SDF_CUT_HOLLOW_SPHERE:
                se_sdf_gen_cut_hollow_sphere(out, p_var, obj->cut_hollow_sphere.radius,
                                            obj->cut_hollow_sphere.cut_height, obj->cut_hollow_sphere.thickness);
                break;
            case SE_SDF_DEATH_STAR:
                se_sdf_gen_death_star(out, p_var, obj->death_star.radius_a,
                                     obj->death_star.radius_b, obj->death_star.separation);
                break;
            case SE_SDF_ROUND_CONE:
                se_sdf_gen_round_cone(out, p_var, obj->round_cone.radius_base,
                                     obj->round_cone.radius_top, obj->round_cone.height);
                break;
            case SE_SDF_ROUND_CONE_ARBITRARY:
                se_sdf_gen_round_cone_arb(out, p_var, obj->round_cone_arbitrary.a,
                                         obj->round_cone_arbitrary.b, obj->round_cone_arbitrary.radius_a,
                                         obj->round_cone_arbitrary.radius_b);
                break;
            case SE_SDF_VESICA_SEGMENT:
                se_sdf_gen_vesica(out, p_var, obj->vesica_segment.a,
                                 obj->vesica_segment.b, obj->vesica_segment.width);
                break;
            case SE_SDF_RHOMBUS:
                se_sdf_gen_rhombus(out, p_var, obj->rhombus.length_a, obj->rhombus.length_b,
                                  obj->rhombus.height, obj->rhombus.corner_radius);
                break;
            case SE_SDF_OCTAHEDRON:
                se_sdf_gen_octa(out, p_var, obj->octahedron.size);
                break;
            case SE_SDF_OCTAHEDRON_BOUND:
                se_sdf_gen_octa_bound(out, p_var, obj->octahedron_bound.size);
                break;
            case SE_SDF_PYRAMID:
                se_sdf_gen_pyramid(out, p_var, obj->pyramid.height);
                break;
            case SE_SDF_TRIANGLE:
                se_sdf_gen_tri(out, p_var, obj->triangle.a, obj->triangle.b, obj->triangle.c);
                break;
            case SE_SDF_QUAD:
                se_sdf_gen_quad(out, p_var, obj->quad.a, obj->quad.b, obj->quad.c, obj->quad.d);
                break;
            case SE_SDF_NONE:
            default:
                se_sdf_string_append(out, "1e10"); // Large distance for empty container
                break;
        }
    } else if (child_count == 1) {
        // Single child - just recurse
        se_sdf_object* child = se_sdf_get_child(obj, 0);
        se_sdf_object_to_string(out, child, p_var);
    } else {
        // Multiple children - combine with operation
        se_sdf_string_append(out, "(");
        
        for (sz i = 0; i < child_count; i++) {
            if (i > 0) {
                se_sdf_string_append(out, " ");
                se_sdf_gen_operation(out, obj->operation, "", "");
                se_sdf_string_append(out, " ");
            }
            
            se_sdf_object* child = se_sdf_get_child(obj, i);
            se_sdf_object_to_string(out, child, p_var);
        }
        
        se_sdf_string_append(out, ")");
    }
}

void se_sdf_generate_distance_function(se_sdf_string* out, se_sdf_object* root, const char* func_name) {
    se_sdf_string_append(out, "float %s(vec3 p) {\n    return ", func_name);
    se_sdf_object_to_string(out, root, "p");
    se_sdf_string_append(out, ";\n}\n");
}
