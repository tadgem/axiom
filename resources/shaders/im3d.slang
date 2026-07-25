struct VertexInput
{
    float4 positionSize : POSITION;
    uint color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float size : SIZE;
};

struct GeometryOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

uniform float4x4 uViewProj;
uniform float2 uViewport;
uniform float uLineWidthScale;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(uViewProj, float4(input.positionSize.xyz, 1.0));

    uint c = input.color;
    output.color = float4(
        float((c >> 24u) & 0xFFu),
        float((c >> 16u) & 0xFFu),
        float((c >> 8u)  & 0xFFu),
        float(c & 0xFFu)
    ) / 255.0;

    output.size = max(1.0, input.positionSize.w * uLineWidthScale);
    return output;
}

[maxvertexcount(4)]
[shader("geometry")]
void linesGeom(line VertexOutput input[2], inout TriangleStream<GeometryOutput> outputStream)
{
    float4 pos0 = input[0].position;
    float4 pos1 = input[1].position;

    // Convert NDC to screen space coordinates
    float2 screen0 = (pos0.xy / pos0.w) * uViewport * 0.5;
    float2 screen1 = (pos1.xy / pos1.w) * uViewport * 0.5;

    float2 lineVector = screen1 - screen0;
    float len = length(lineVector);
    
    float2 lineNormal = (len > 0.0001) ? float2(-lineVector.y, lineVector.x) / len : float2(0.0, 1.0);

    float halfWidth0 = input[0].size * 0.5;
    float halfWidth1 = input[1].size * 0.5;

    // Convert pixel offset back to clip space delta
    float2 delta0 = (lineNormal * halfWidth0 / (uViewport * 0.5)) * pos0.w;
    float2 delta1 = (lineNormal * halfWidth1 / (uViewport * 0.5)) * pos1.w;

    GeometryOutput output;

    output.position = float4(pos0.xy + delta0, pos0.zw);
    output.color = input[0].color;
    outputStream.Append(output);

    output.position = float4(pos0.xy - delta0, pos0.zw);
    output.color = input[0].color;
    outputStream.Append(output);

    output.position = float4(pos1.xy + delta1, pos1.zw);
    output.color = input[1].color;
    outputStream.Append(output);

    output.position = float4(pos1.xy - delta1, pos1.zw);
    output.color = input[1].color;
    outputStream.Append(output);

    outputStream.RestartStrip();
}

[shader("fragment")]
float4 fragmentMain(GeometryOutput input) : SV_Target
{
    return input.color;
}
