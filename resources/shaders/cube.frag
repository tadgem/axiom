#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform sampler2D diffuse;

void main()
{
    vec4 col  = texture(diffuse, fragUV);
    outColour = vec4(pow(col.rgb, vec3(2.2)), 1.0);
}
