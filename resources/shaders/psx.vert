#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform UBO
{
    mat4    modelViewProj;
    vec2    resolution;
    float     snapIntensity;

} ubo;

layout(location = 0) out vec2 fragUV;

void main()
{
    vec4 clipPos = ubo.modelViewProj * vec4(inPosition, 1.0);
    vec3 ndc = clipPos.xyz / clipPos.w;

    vec2 screenPos = ndc.xy * 0.5 + 0.5;
    vec2 simRes = ubo.resolution / ubo.snapIntensity;
    screenPos *= simRes;
    screenPos = floor(screenPos + 0.5) / simRes;
    ndc.xy = (screenPos * 2.0) - 1.0;
    
    gl_Position = vec4(ndc * clipPos.w, clipPos.w);
    fragUV      = inUV;
}
