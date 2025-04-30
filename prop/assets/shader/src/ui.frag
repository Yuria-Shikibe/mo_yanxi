#pragma shader_stage(fragment)

#include "lib/rect.glsl"
#include "lib/sdf.glsl"
#include "lib/slide_line.glsl"
#include "lib/draw_modes.glsl"

#ifndef MaximumAllowedSamplersSize
#define MaximumAllowedSamplersSize 1
#endif


layout(location = 0) in vec2 in_pos;
layout(location = 1) flat in uvec4 in_indices;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_color_base;
layout(location = 4) in vec4 in_color_light;

layout(location = 0) out vec4 out_color_base;
layout(location = 1) out vec4 out_color_light;
layout(location = 2) out vec4 out_color_background;


layout(set = 0, binding = 1) uniform UBO{
    frect rect;
    float dst;
    float time;
    vec2 extent;
    float inv_scale;
    uint cap1;
    uint cap2;
    uint cap3;
}ubo;

layout(set = 1, binding = 0) uniform sampler2D textures[4];

const sline_line_style line_style = sline_line_style(
    45.f, 1.f, 30, 16
);

void main() {
    vec4 texColor;
    const uint mode = in_indices.b;

    float scale;
    if(ubo.dst == 0.){
        if(rectNotContains(ubo.rect, in_pos)){
            discard;
        }else{
            scale = 1.f;
        }
    }else{
        scale = 1 - smoothstep(0, 2.5, (dst_to_rect(in_pos, ubo.rect, ubo.extent) - ubo.dst));
        if(scale < 1.f / 255.f){
            discard;
        }
    }


    if(bool(mode & draw_mode_sdf)){
        float msdf = msdf(textures[in_indices[0]], in_uv, 1, bool(mode & draw_mode_uniformed));
        msdf = smoothstep(-0.0375 * ubo.inv_scale, 0.07 * ubo.inv_scale, msdf);
        texColor = vec4(1, 1, 1, msdf);
    }else{
        texColor = texture(textures[in_indices[0]], in_uv);
    }

    if(bool(mode & draw_mode_slide_line)){
        float scl = is_in_slide_line_smooth(in_pos * ubo.extent * ubo.inv_scale, line_style, ubo.time * 60, 0.175f);
        texColor.a *= scl;
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

    out_color_light = scale * texColor * in_color_base;
    out_color_base = scale * texColor * in_color_light;
    out_color_background = scale * in_color_light;
}

