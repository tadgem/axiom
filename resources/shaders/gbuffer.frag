#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

// G-Buffer targets:
//   0 : diffuse colour   (RGB10A2)
//   1 : normal xy        (RG16)
//   2 : uv               (RG16)
layout(location = 0) out vec4 outColour;
layout(location = 1) out vec2 outNormal;
layout(location = 2) out vec2 outUV;

layout(set = 0, binding = 1) uniform sampler2D diffuse;

void main()
{
    vec3 normal = normalize(fragNormal);

    outColour = texture(diffuse, fragUV);
    outNormal = normal.xy;
    outUV     = fragUV;
}
