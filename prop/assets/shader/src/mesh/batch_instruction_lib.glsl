#ifndef INSTRUCTION_BUF
const uint reserved_cap[1];
#define INSTRUCTION_BUF reserved_cap
#endif

#ifndef BATCH_LIB
#define BATCH_LIB


const uint HEAD_OFFSET = 2;

#define ACCESS_BUF(head, byte_offset, off) INSTRUCTION_BUF[head + HEAD_OFFSET + byte_offset / 4 + off]

const uint
noop = 0,
triangle = 1,
rectangle = 2,
line = 3,
circle = 4,
partial_circle = 5,
uniform_update = 6;


const float CircleVertPrecision = 12.f;

uint get_circle_vertices(const float radius){
    return clamp(uint(radius * 3.1415927f / CircleVertPrecision), uint(10U), uint(256U));
}

uint get_vertex_count__circle_fill(const uint instruction_head_offset) {
    const float radius_from = uintBitsToFloat(ACCESS_BUF(instruction_head_offset, 24, 0));
    const float radius_to = uintBitsToFloat(ACCESS_BUF(instruction_head_offset, 24, 1));

    const uint cnt = get_circle_vertices(radius_to);
    return radius_from == 0 ? cnt + 1 : cnt * 2;
}

uint get_vertex_count__partial_circle_fill(const uint instruction_head_offset) {
    const float radius_from = uintBitsToFloat(ACCESS_BUF(instruction_head_offset, 24, 0));
    const float radius_to = uintBitsToFloat(ACCESS_BUF(instruction_head_offset, 24, 1));

    const float ratio_from = uintBitsToFloat(ACCESS_BUF(instruction_head_offset, 24, 2));
    const float ratio_to = uintBitsToFloat(ACCESS_BUF(instruction_head_offset, 24, 3));

    const float dst = ratio_to - radius_from;
    const uint cnt = uint(ceil(get_circle_vertices(radius_to) * (dst >= 0 ? dst : 1.f + dst)));
    return radius_from == 0 ? cnt + 1 : cnt * 2;
}


uint get_vertex_count(const uint instruction_head_offset){
    switch(INSTRUCTION_BUF[instruction_head_offset]){
        case triangle: return 1U;
        case rectangle: return 2U;
        case line: return 2U;
        case circle: return get_vertex_count__circle_fill(instruction_head_offset);
        case partial_circle: return get_vertex_count__partial_circle_fill(instruction_head_offset);
        default: return -1;
    }
}

#endif