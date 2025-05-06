#ifndef LIB_SLIDE_LINE_DEF
#define LIB_SLIDE_LINE_DEF

struct slide_line_style {
    float angle;
    float scale;
    float spacing;
    float stroke;
};

float slope(float value){
    return 1.0f - abs(value - 0.5f) * 2.0f;
}

float pingpong(float src, float dst, float value) {
    float len = dst - src;
    float t = abs(mod(value, 2.0 * abs(len)) / len - 1.0);
    return mix(src, dst, t);
}

float get_dst_at(const float angle, const vec2 T){
    const float sin_ = sin(radians(angle + 45));
    const float cos_ = cos(radians(angle + 45));

    const vec2 nor = vec2(cos_ - sin_, sin_ + cos_);
    const float len = dot(nor, T);
    return len;
}

bool is_in_slide_line(const vec2 T, const slide_line_style style, float phase){
    const float len = get_dst_at(style.angle, T) * style.scale;

    return mod(len + phase, style.spacing) < style.stroke;// step(, stroke);
}

float is_in_slide_line_smooth(const vec2 T, const slide_line_style style, float phase, float range){
    const float len = get_dst_at(style.angle, T) * style.scale;

    float v = slope(mod(len + phase, style.spacing) / style.spacing) * style.spacing / style.stroke;
    return smoothstep(1.f - range *.5f, 1.f + range * .5f, v);
}

#endif