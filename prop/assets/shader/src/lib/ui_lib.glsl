#ifndef LIB_UI_DEF
#define LIB_UI_DEF

#include "rect.glsl"
#include "slide_line.glsl"
#include "draw_modes.glsl"


const slide_line_style line_style = slide_line_style(
    45.f, 1.f, 25, 12.5
);


struct ui_ubo_data{
    frect rect;
    float dst;
    float time;
    vec2 extent;
    float inv_scale;
    uint cap1;
    uint cap2;
    uint cap3;
};

float get_ui_alpha_scale(const ui_ubo_data ubo, const uint mode, const vec2 in_pos, const vec2 in_raw_pos){
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

    if(bool(mode & draw_mode_slide_line)){
        float scl = is_in_slide_line_smooth(in_raw_pos/** / ubo.inv_scale*/, line_style, ubo.time * 60, 0.175f);
        if(scl == 0)discard;
        scale *= scl;
    }

    return scale;
}

#endif