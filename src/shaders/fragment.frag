#version 450

layout( location = 0 ) in vec4 inColor;
layout( location = 1 ) in float inDiffuse;

layout( location = 0 ) out vec4 outColor;

void main()
{
    vec3 lighting = vec3( inDiffuse );
    outColor = vec4( inColor.rgb * lighting, inColor.a );
}
