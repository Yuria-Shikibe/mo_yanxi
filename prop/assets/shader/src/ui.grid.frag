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
layout(location = 5) in vec2 in_raw_pos;

layout(location = 0) out vec4 out_color_base;
layout(location = 1) out vec4 out_color_light;
layout(location = 2) out vec4 out_color_background;

layout(set = 0, binding = 1) uniform UBO{
    ui_ubo_data ubo;
};

layout(set = 2, binding = 0) uniform GridUBO{
    vec2 chunk_size;
    ivec2 solid_spacing;

    vec4 line_color;
    vec4 main_line_color;

    float line_width;
    float main_line_width;
    float line_spacing;
    float line_gap;
}grid;

layout(set = 1, binding = 0) uniform sampler2D textures[4];

bool is_in_secondary_line(vec2 pos){
    vec2 spacing_pos = step(vec2(grid.line_gap), mod(pos + grid.line_width / 2.f, grid.line_spacing));

    vec2 local_pos1 = mod(pos + grid.line_width / 2.f, grid.chunk_size);
    bool xValid1 = (local_pos1.x < grid.line_width);
    bool yValid1 = (local_pos1.y < grid.line_width);


    return (xValid1 && spacing_pos.y > 0 || yValid1 && spacing_pos.x > 0);
}

bool is_in_primary_line(vec2 pos){

    vec2 local_pos2 = mod(pos + grid.line_width, grid.chunk_size * vec2(grid.solid_spacing));
    bool xValid2 = (local_pos2.x < grid.line_width * 2);
    bool yValid2 = (local_pos2.y < grid.line_width * 2);

    return (xValid2 || yValid2);
}

bool is_in_axis(vec2 pos, float scale){
    vec2 a = abs(pos / scale);
    return a.x < grid.main_line_width || a.y < grid.main_line_width;
}

void main() {
    vec4 texColor;
    const uint mode = in_indices.b;

    float scale = get_ui_alpha_scale(ubo, mode, in_pos, in_raw_pos);

    if(bool(mode & draw_mode_sdf)){
        float msdf = msdf(textures[in_indices[0]], in_uv, 1);
        msdf = smoothstep(-0.0375 * ubo.inv_scale, 0.07 * ubo.inv_scale, msdf);
        texColor = vec4(1, 1, 1, msdf);
    }else{
        texColor = in_indices[0] == 0xff ? vec4(1) : texture(textures[in_indices[0]], in_uv);
    }

    vec4 scaler = scale * texColor;

    out_color_background = vec4(0);
    out_color_light = vec4(0);

    if(is_in_axis(in_raw_pos, ubo.inv_scale)){
        out_color_base = scaler * grid.main_line_color;
    }else if(is_in_primary_line(in_raw_pos)){
        out_color_base = scaler * grid.line_color;
    }else if(is_in_secondary_line(in_raw_pos)){
        out_color_base = scaler * grid.line_color * .75f;
    }else{
        out_color_base = scaler * in_color;
    }
}

