#version 450

layout( location = 0 ) in vec4 inPosition;

layout( location = 0 ) out vec4 outColor;

void main()
{
    gl_Position = inPosition;
    outColor = vec4( 0.0, 1.0, 0.0, 1.0 );
}
