struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

uniform float4x4 modelViewProj;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(modelViewProj, float4(input.position, 1.0));
    output.color = mul(modelViewProj, float4(input.normal, 1.0));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color.xyz, 1.0);
}
