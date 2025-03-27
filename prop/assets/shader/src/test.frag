#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in PerVertexData
{
    vec4 color;
} fragIn;

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = fragIn.color;
}