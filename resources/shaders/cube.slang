#pragma pack_matrix(column_major)

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv :UV;
};

uniform float4x4 modelViewProj;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(modelViewProj, float4(input.position, 1.0));
    output.uv = input.uv;
    // output.color = mul(modelViewProj, float4(input.normal, 1.0));

    return output;
}

Texture2D       diffuse;
SamplerState    diffuseSampler;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 col = diffuse.Sample(diffuseSampler, input.uv);

    return col;
}
