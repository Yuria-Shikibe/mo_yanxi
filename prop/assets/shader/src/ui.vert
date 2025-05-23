#pragma shader_stage(vertex)

layout(location = 0) in vec2 in_pos;
layout(location = 1) in uvec4 in_indices;
layout(location = 2) in vec4 in_color_scl;
layout(location = 3) in vec2 in_uv;



layout(location = 0) out vec2 out_pos;
layout(location = 1) flat out uvec4 out_indices;
layout(location = 2) out vec2 out_uv;

layout(location = 3) out vec4 out_color;
layout(location = 4) flat out vec4 out_color_channel;
layout(location = 5) out vec2 out_raw_pos;


out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UBO {
    mat3 view;
} ubo;

void main() {
    out_raw_pos = in_pos.xy;
    vec2 spos = (ubo.view * vec3(in_pos.xy, 1.0)).xy;
    out_pos = spos;

    gl_Position = vec4(spos, 0.f, 1.f);

    out_uv = in_uv;
    out_indices = in_indices;

    out_color = in_color_scl;

    vec4 channel = vec4(0);

    switch(in_indices[1]){
        case 0: channel[0] = 1; break;
        case 1: channel[1] = 1; break;
        case 2: channel[2] = 1; break;
        default: break;
    }

    out_color_channel = channel;
}
