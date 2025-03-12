#pragma shader_stage(vertex)

layout(location = 0) in vec2 in_pos;
layout(location = 1) in uvec4 in_indices;
layout(location = 2) in vec4 in_color_scl;
layout(location = 3) in vec4 in_color_ovr;
layout(location = 4) in vec2 in_uv;



layout(location = 0) out vec2 out_pos;
layout(location = 1) flat out uvec4 out_indices;
layout(location = 2) out vec2 out_uv;

layout(location = 3) out vec4 out_color_ovr;
layout(location = 4) out vec4 out_color_base;
layout(location = 5) out vec4 out_color_light;

//layout(location = 5) out vec4 out_color_base;
//layout(location = 6) out vec4 out_color_light;
//layout(location = 7) out vec4 out_color_backgrounod;


const float Overflow = .001f;
const float LightColorRange = 2550.f;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UBO {
    mat3 view;
} ubo;

void main() {
    vec2 spos = (ubo.view * vec3(in_pos.xy, 1.0)).xy;
    out_pos = spos;

    gl_Position = vec4(spos, 0.f, 1.f);

    out_uv = in_uv;
    out_indices = in_indices;

    out_color_base = mod(in_color_scl, 10.f);
    out_color_light = in_color_scl / LightColorRange;

    out_color_ovr = in_color_ovr;


//    if(inTextureID[1] == 0){
//        baseColor = inColor;
//    }else
//    baseColor = inColor;//mod(inColor, 10.f);
//    lightColor = inColor / LightColorRange;

}
