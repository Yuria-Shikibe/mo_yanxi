#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_uv;
layout(location = 1) flat in uvec4 in_indices;
layout(location = 2) in vec4 in_color_base;
layout(location = 3) in vec4 in_color_light;
layout(location = 4) in vec4 in_color_override;

layout(set = 0, binding = 1) uniform UBO {
    uint depth_test;
    float camera_scale;
} ubo;

layout(set = 1, binding = 0) uniform sampler2DArray texSampler[4];

layout(location = 0) out vec4 out_color_base;
layout(location = 1) out vec4 out_color_light;
//TODO normal output

layout (depth_unchanged) out float gl_FragDepth;

const float Threshold = 0.65f;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float get(sampler2DArray samp, bool uniformed) {

    vec2 msdfUnit = 1. / vec2(textureSize(samp, 0)) * sqrt(ubo.camera_scale);
    vec3 col = texture(samp, vec3(in_uv.xy, in_indices[1])).rgb;
    float sigDist = median(col.r, col.g, col.b) - 0.5;

    if(uniformed)sigDist *= dot(msdfUnit, 0.5 / fwidth(in_uv.xy));

//    sigDist = clamp(sigDist + 0.5, 0.0, 1.0);
    return sigDist;
}

void main() {
    vec4 texColor = texture(texSampler[in_indices[0]], vec3(in_uv.xy, in_indices[1]));
//    float normalized = (texColor.r - .5f) * 2.f;
////


    texColor = vec4(1, 1, 1, smoothstep(-0.035, 0.035, get(texSampler[in_indices[0]], false)));
//    texColor = vec4(1, 1, 1, get(texSampler[in_indices[0]]));

    bool enable_depth = bool(ubo.depth_test);
    if(enable_depth){
        if(texColor.a * in_color_base.a > Threshold || texColor.a * in_color_light.a > Threshold){
            gl_FragDepth = gl_FragCoord.z;
        }else{
            discard;
        }
    }else{
        gl_FragDepth = gl_FragCoord.z;
    }


    vec4 base = texColor * in_color_base;
    vec4 light = texColor * in_color_light;
    float solid = enable_depth ? step(Threshold, base.a) : base.a;

    out_color_base = vec4(base.rgb, solid);

    out_color_light = enable_depth ? mix(vec4(0.f, 0.f, 0.f, solid), light, light.a) : light;


}

