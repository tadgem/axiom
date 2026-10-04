#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform UBO
{
    mat4 modelViewProj;
} ubo;

layout(location = 0) out vec2 fragUV;

void main()
{
    gl_Position = ubo.modelViewProj * vec4(inPosition, 1.0);
    fragUV      = inUV;
}
