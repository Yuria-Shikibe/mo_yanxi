vec4 mulAlpha(vec4 color) {
    return vec4(color.rgb * color.a, color.a);
}

vec4 alphaBlend(vec4 src, vec4 dst) {
    vec4 result = mulAlpha(src) + mulAlpha(dst) * (1.0 - src.a);
    return result;
}

vec4 blend(const in vec4 src, const in vec4 dst) {
    vec3 rgb = mix(dst.rgb * dst.a, src.rgb, src.a);
    float a = mix(dst.a, 1.0, src.a);
    return vec4(rgb, a);
}

vec4 blend2(vec4 src, vec4 dst) {
    vec4 result = vec4(src.rgb + dst.rgb * (1 - src.a), src.a + dst.a * (1 - src.a));
    return result;
}
