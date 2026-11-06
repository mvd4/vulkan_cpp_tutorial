#version 450

layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec4 inColor;
layout( location = 2 ) in vec4 inNormal;

layout( location = 0 ) out vec4 outColor;
layout( location = 1 ) out float outDiffuse;

layout( push_constant ) uniform PushConstants
{
    mat4 mvp;
    mat4 normalMatrix;
} pushConstants;

// Direction from the surface towards a fixed directional light, in world space.
const vec3 lightDirection = normalize( vec3( 1.0, 0.0, 2.0 ) );

void main()
{
    gl_Position = pushConstants.mvp * inPosition;

    vec3 worldNormal = normalize( mat3( pushConstants.normalMatrix ) * inNormal.xyz );
    outDiffuse = max( dot( worldNormal, lightDirection ), 0.0 );

    outColor = inColor;
}
