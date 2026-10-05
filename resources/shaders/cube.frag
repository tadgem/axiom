#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform sampler2D diffuse;

void main()
{
    // Textures are sampled from an UNORM view of sRGB-encoded image data and
    // the swapchain is a linear UNORM target, so the colour is passed through
    // unchanged (an identity round trip).
    outColour = texture(diffuse, fragUV);
}
