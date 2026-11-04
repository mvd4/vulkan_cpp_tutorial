#version 450

layout( location = 0 ) in vec4 inColor;

layout( location = 0 ) out vec4 outColor;

layout( push_constant ) uniform PushConstants
{
    layout( offset = 64 ) vec4 ambientColor;
} pushConstants;

void main()
{
    outColor = inColor * pushConstants.ambientColor;
}
