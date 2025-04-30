struct frect {
    vec2 src;
    vec2 dst;
};

bool rectNotContains(frect rect, vec2 pos){
    return (pos.x < rect.src.x || pos.x > rect.dst.x || pos.y < rect.src.y || pos.y > rect.dst.y);
}

bool rectContains(frect rect, vec2 pos){
    return !rectNotContains(rect, pos);
}

float dst_to_rect(vec2 pos, frect rect, vec2 scl){
    vec2 ppos = clamp(pos, rect.src, rect.dst);
    return distance(ppos * scl, pos * scl);
}