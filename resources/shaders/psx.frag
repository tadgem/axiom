#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 1) uniform sampler2D diffuse;

float findDitherValue(vec2 coords) {
    int x = int(mod(coords.x, 16.0));
    int y = int(mod(coords.y, 16.0));
    int index = x + y * 16;
    
    float dither[16] = float[16](
        0.0 / 16.0,  8.0 / 16.0,  2.0 / 16.0, 10.0 / 16.0,
       12.0 / 16.0,  4.0 / 16.0, 14.0 / 16.0,  6.0 / 16.0,
        3.0 / 16.0, 11.0 / 16.0,  1.0 / 16.0,  9.0 / 16.0,
       15.0 / 16.0,  7.0 / 16.0, 13.0 / 16.0,  5.0 / 16.0
    );
    
    return dither[index];
}

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec4 color = texture(diffuse, fragUV);
    
    float brightness = luma(color.rgb);
    float threshold = findDitherValue(fragCoord);
    
    // Compare brightness to dither threshold
    float finalValue = brightness > threshold ? 1.0 : 0.25;
    outColour = vec4(color.rgb * finalValue, 1.0);
}
