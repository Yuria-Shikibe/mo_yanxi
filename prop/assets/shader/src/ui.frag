#pragma shader_stage(fragment)

#include "lib/rect.glsl"
#include "lib/sdf.glsl"
#include "lib/draw_modes.glsl"

#ifndef MaximumAllowedSamplersSize
#define MaximumAllowedSamplersSize 1
#endif


layout(location = 0) in vec2 in_pos;
layout(location = 1) flat in uvec4 in_indices;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_color_ovr;
layout(location = 4) in vec4 in_color_base;
layout(location = 5) in vec4 in_color_light;

layout(location = 0) out vec4 out_color_base;
layout(location = 1) out vec4 out_color_light;
layout(location = 2) out vec4 out_color_background;


layout(set = 0, binding = 1) uniform Scissor{
    frect rect;
};
layout(set = 1, binding = 0) uniform sampler2D textures[4];

const float LightColorRange = 2550.f;

void main() {
    vec4 texColor;
    const uint mode = in_indices.b;
    if(bool(mode & draw_mode_sdf)){
        float msdf = msdf(textures[in_indices[0]], in_uv, 1, bool(mode & draw_mode_uniformed));
        msdf = smoothstep(-0.03, 0.03, msdf);
        texColor = vec4(1, 1, 1, msdf);
    }else{
        texColor = texture(textures[in_indices[0]], in_uv);
    }


    vec4 base_scl;
    vec4 back_scl;

    switch(in_indices[1]){
        case 1: {
            base_scl = vec4(0.);
            back_scl = vec4(1.);
            break;
        }
        default: {
            back_scl = vec4(0.);
            base_scl = vec4(1.);
        }
    }

    out_color_base = texColor * in_color_base;
    out_color_light = texColor * in_color_light;
    out_color_background = in_color_light;
}

