#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

//#include "lib/vertex_indices.glsl"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in uvec4 in_indices;
layout(location = 2) in vec4 in_color_scl;
layout(location = 3) in vec2 in_uv;


layout(location = 0) out vec2 out_uv;
layout(location = 1) flat out uvec4 out_indices;
layout(location = 2) out vec4 out_color_base;
layout(location = 3) out vec4 out_color_light;

const float Overflow = .001f;
const float LightColorRange = 2550.f;
const float zScale = 1.f;//512.f;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UBO {
    mat3 view;
    mat3 proj;
} transform;

float nor_luma(float luminance, float maxLuminance) {
    // 1. 归一化到 [0, 1] 范围
    float normalized = clamp(luminance / maxLuminance, 0.0, 1.0);

    // 2. 可选 Gamma 校正（sRGB 标准 ≈ 2.2）
    const float gamma = 2.2;
    normalized = pow(normalized, 1.0 / gamma); // 逆 Gamma 校正

    return normalized;
}
float nor_luma(
    float luminance
) {
//    // 1. 截断到 [minLuminance, maxLuminance] 范围
//    float clamped = clamp(luminance, minLuminance, maxLuminance);
//
//    // 2. 线性映射到 [0, 1]
    float normalized = luminance;// (clamped - minLuminance) / (maxLuminance - minLuminance);

    // 3. 可选 Gamma 校正（sRGB 标准 ≈ 2.2）
    const float gamma = 2.2;
    normalized = pow(normalized, 1.0 / gamma); // 逆 Gamma 校正

    return normalized;
}

const float sqrt3 = 1.7320508075688772935274463415059;
float luma(vec3 color) {

//    return length(color) / sqrt3;
//    const vec3 luminanceCoeff = vec3(0.2126, 0.7152, 0.0722); // BT.709 系数
    const vec3 luminanceCoeff = vec3(1. / 3., 1. / 3., 1. / 3.); // BT.709 系数
    return dot(color, luminanceCoeff); // 自动处理超量值
}

void main() {

    const float minLuma = 1;
    const float maxLuma = 2;
    gl_Position = vec4((transform.view * transform.proj * vec3(in_pos.xy, 1.0)).xy , in_pos.z / zScale, 1.0);

    float base_luma = luma(in_color_scl.rgb);
    float clamped = clamp(base_luma, minLuma, maxLuma);
    float normalized = (clamped - minLuma) / (maxLuma - minLuma);

    float light = normalized;

    out_color_base = vec4(in_color_scl.rgb, min(in_color_scl.a, 1.f) * (1 - light));
    out_color_light = vec4(in_color_scl.rgb, light * in_color_scl.a);

    out_indices = in_indices;
    out_uv = in_uv;
}
