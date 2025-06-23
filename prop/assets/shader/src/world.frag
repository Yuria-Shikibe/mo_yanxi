#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects: enable
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_float16: require

//layout (early_fragment_tests) in;

layout (location = 0) in vec2 in_uv;
layout (location = 1) flat in uvec4 in_indices;
layout (location = 2) in vec4 in_color_base;
layout (location = 3) in vec4 in_color_light;

layout (set = 0, binding = 1) uniform UBO {
    uint depth_test;
    float camera_scale;

    uint cap1;
    uint cap2;
} ubo;

struct oit_node {
    f16vec4 color_base;
    f16vec4 color_light;
    f16vec4 color_normal;

    float depth;
    uint next;
};

layout (set = 0, binding = 2, std430) restrict buffer oit_head_s {
    uint capacity;
    uint size;

    uint cap1;
    uint cap2;
} oit_stat;

layout (set = 0, binding = 3, std430) restrict writeonly buffer oit_list_s {
    oit_node nodes[];
} oit_list;

layout (set = 0, binding = 4, r32ui) restrict uniform uimage2D oit_head_indices;

layout (set = 1, binding = 0) uniform sampler2D/**Array*/ texSampler[4];

layout (location = 0) out vec4 out_color_base;
layout (location = 1) out vec4 out_color_light;
//TODO normal output

layout (depth_unchanged) out float gl_FragDepth;

const float Threshold = 0.95f;

bool is_alpha_valid(float a) {
    return a > 0 && a < Threshold;
}

void main() {
    //    vec4 texColor = texture(texSampler[in_indices[0]], vec3(in_uv.xy, in_indices[1]));
    vec4 texColor = in_indices[0] == 0xff ? vec4(1) : texture(texSampler[in_indices[0]], in_uv);
    float maxAlpha = texColor.a * (in_color_base.a + in_color_light.a);
    if (maxAlpha < 1.25 / 255.f)discard;

    vec4 out_base = texColor * in_color_base;
    vec4 out_light = texColor * in_color_light;

//    if(enable_depth){
        if(maxAlpha > Threshold){
            gl_FragDepth = gl_FragCoord.z;

            if(is_alpha_valid(out_base.a) || is_alpha_valid(out_light.a)){
                uint idx = atomicAdd(oit_stat.size, 1u);
                if(idx < oit_stat.capacity){
                    uint prev = imageAtomicExchange(oit_head_indices, ivec2(gl_FragCoord.xy), idx);

                    oit_list.nodes[idx].color_base = f16vec4(out_base);
                    oit_list.nodes[idx].color_light = f16vec4(out_light);
                    oit_list.nodes[idx].depth = gl_FragCoord.z;
                    oit_list.nodes[idx].next = prev;
                }

                discard;
            }else{
                out_color_base = out_base;
                out_color_light = out_light;
            }
        }else{
            uint idx = atomicAdd(oit_stat.size, 1u);
            if(idx < oit_stat.capacity){
                uint prev = imageAtomicExchange(oit_head_indices, ivec2(gl_FragCoord.xy), idx);

                oit_list.nodes[idx].color_base = f16vec4(out_base);
                oit_list.nodes[idx].color_light = f16vec4(out_light);
                oit_list.nodes[idx].depth = gl_FragCoord.z;
                oit_list.nodes[idx].next = prev;
            }

            discard;
        }
//    }else{
//        gl_FragDepth = gl_FragCoord.z;
//    }


}

