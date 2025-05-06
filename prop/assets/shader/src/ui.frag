#pragma shader_stage(fragment)

#include "lib/rect.glsl"
#include "lib/ui_lib.glsl"
#include "lib/sdf.glsl"
#include "lib/slide_line.glsl"
#include "lib/draw_modes.glsl"

layout(location = 0) in vec2 in_pos;
layout(location = 1) flat in uvec4 in_indices;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_color;
layout(location = 4) flat in vec4 in_color_channel;

layout(location = 0) out vec4 out_color_base;
layout(location = 1) out vec4 out_color_light;
layout(location = 2) out vec4 out_color_background;

layout(set = 0, binding = 1) uniform UBO{
    ui_ubo_data ubo;
};

layout(set = 1, binding = 0) uniform sampler2D textures[4];

void main() {
    vec4 texColor;
    const uint mode = in_indices.b;

    float scale = get_ui_alpha_scale(ubo, mode, in_pos);

    if(bool(mode & draw_mode_sdf)){
        float a = msdf(textures[in_indices[0]], in_uv, 1, bool(mode & draw_mode_uniformed));
        a = smoothstep(-0.0375 * ubo.inv_scale, 0.07 * ubo.inv_scale, a);
//        if(a == 0)discard;

        texColor = vec4(1, 1, 1, a);
    }else{
        texColor = texture(textures[in_indices[0]], in_uv);
    }

    vec4 color = scale * texColor * in_color;

    out_color_light = in_color_channel[0] * color;
    out_color_base = in_color_channel[1] * color;
    out_color_background = in_color_channel[2] * color;
}

