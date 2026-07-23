// generate_mips.slang
// Compute shader for generating texture mipmaps in Axiom Engine

struct MipParams
{
    float2 srcTexelSize;  // Texel size of source mip (1.0 / srcWidth, 1.0 / srcHeight)
    uint2  dstSize;       // Dimensions of target mip level (width, height)
    uint   srcMipLevel;   // Source mipmap level index
};

uniform MipParams params;

Texture2D<float4>   srcTexture;
SamplerState        srcSampler;
RWTexture2D<float4> dstTexture;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= params.dstSize.x || dispatchThreadID.y >= params.dstSize.y)
        return;

    // Calculate normalized UV coordinates centered on the destination texel
    float2 uv = (float2(dispatchThreadID.xy) + 0.5) / float2(params.dstSize);

    // Sample from the source mip level using bilinear filtering to downsample
    float4 color = srcTexture.SampleLevel(srcSampler, uv, 0);

    // Write downsampled texel color to target mip level
    dstTexture[dispatchThreadID.xy] = color;
}
