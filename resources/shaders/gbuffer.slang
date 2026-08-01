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
    output.colour = diffuse.Sample(diffuseSampler, input.uv);
    output.normal = float2(input.normal.x, input.normal.y);
    output.uv = input.uv;
    //float4 col = float4( input.uv, 0.0, 1.0 );

    return output;
}
