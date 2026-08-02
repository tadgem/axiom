struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 uv :UV;
};

uniform float4x4 modelViewProj;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(modelViewProj, float4(input.position, 1.0));
    output.normal = input.normal;
    output.uv = input.uv;
    // output.color = mul(modelViewProj, float4(input.normal, 1.0));

    return output;
}

struct FragmentOutput
{
    float4 colour;
    float2 normal;
    float2 uv;
};

Texture2D       diffuse;
SamplerState    diffuseSampler;

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput input) : SV_Target
{
    FragmentOutput output;
    float3 normalized = normalize(input.normal);
    output.colour = diffuse.Sample(diffuseSampler, input.uv);
    output.normal = float2(normalized.x, normalized.y);
    output.uv = input.uv;

    return output;
}
