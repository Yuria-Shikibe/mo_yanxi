
vec4 resolve_samples(int count, ivec2 pos, sampler2DMS tex) {
    float weight = 1.f / float(count);

    vec4 avg_color = vec4(0.0);
    for (int i = 0; i < count; i++) {
        avg_color += texelFetch(tex, pos, i);
    }
    avg_color *= weight;

    return avg_color;
}