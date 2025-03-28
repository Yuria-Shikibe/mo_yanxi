struct sline_line_style {
    float angle;
    float scale;
    float spacing;
    float stroke;
};

float get_dst_at(const float angle, const vec2 T){
    const float sin_ = sin(radians(angle + 45));
    const float cos_ = cos(radians(angle + 45));

    const vec2 nor = vec2(cos_ - sin_, sin_ + cos_);
    const float len = dot(nor, T);
    return len;
}

bool is_in_slide_line(const vec2 T, const sline_line_style style, float phase){
    const float len = get_dst_at(style.angle, T) * style.scale;

    return mod(len + phase, style.spacing) < style.stroke;// step(, stroke);
}

float is_in_slide_line_smooth(const vec2 T, const sline_line_style style, float phase, float range){
    const float len = get_dst_at(style.angle, T) * style.scale;

    float v = mod(len + phase, style.spacing) / style.stroke;
    return smoothstep(1.f - range *.5f, 1.f + range * .5f, v);
}