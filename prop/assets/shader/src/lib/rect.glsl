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
