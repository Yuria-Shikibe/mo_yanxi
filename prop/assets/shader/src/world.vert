#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

//#include "lib/vertex_indices.glsl"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in uvec4 in_indices;
layout(location = 2) in vec4 in_color_scl;
layout(location = 3) in vec4 in_color_override;
layout(location = 4) in vec2 in_uv;


layout(location = 0) out vec2 out_uv;
layout(location = 1) flat out uvec4 out_indices;
layout(location = 2) out vec4 out_color_base;
layout(location = 3) out vec4 out_color_light;
layout(location = 4) out vec4 out_color_override;

const float Overflow = .001f;
const float LightColorRange = 2550.f;
const float zScale = 1.f;//512.f;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UBO {
    mat3 view;
} transform;

void main() {

    gl_Position = vec4((transform.view * vec3(in_pos.xy, 1.0)).xy , in_pos.z / zScale, 1.0);

    out_color_base = mod(in_color_scl, 10.f);
    out_color_light = in_color_scl / LightColorRange;
    out_color_override = in_color_override;

    out_indices = in_indices;
    out_uv = in_uv;
}
