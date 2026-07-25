#include "nanovg_impl_slang_rhi.h"
#include <slang-rhi/shader-cursor.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

    enum SlangRHINVGcallType {
        NVG_NONE = 0,
        NVG_FILL,
        NVG_CONVEXFILL,
        NVG_TRIANGLES,
    };

    struct SlangRHINVGpath {
        int fillOffset = 0;
        int fillCount  = 0;
        int strokeOffset = 0;
        int strokeCount  = 0;
    };

    struct FragUniforms {
        float scmat[4];       // a, b, c, d
        float scoffset[2];    // e, f
        float scextent[2];    // sw, sh
        float paintmat[4];    // a, b, c, d
        float paintoffset[2]; // e, f
        float innerCol[4];    // r, g, b, a
        float outerCol[4];    // r, g, b, a
        float extent[2];      // w, h
        float radius;
        float feather;
        float strokeMult;
        float strokeThr;
        int   texType;    // 0: no tex, 1: RGBA, 2: Alpha
        int   shaderType; // 0: gradient, 1: texture, 2: simple
    };

    struct SlangRHINVGcall {
        int type = NVG_NONE;
        int image = 0;
        int pathOffset = 0;
        int pathCount  = 0;
        int triangleOffset = 0;
        int triangleCount  = 0;
        int uniformOffset  = 0;
    };

    struct TextureUpdate {
        int x = 0, y = 0, w = 0, h = 0;
        std::vector<uint8_t> data;
    };

    struct SlangRHINVGtexture {
        int id = 0;
        rhi::ComPtr<rhi::ITexture>     texture;
        rhi::ComPtr<rhi::ITextureView> view;
        int type   = 0;
        int width  = 0;
        int height = 0;
        int flags  = 0;
        bool external = false;

        std::vector<TextureUpdate> pendingUpdates;
    };

    struct SlangRHINVGcontext {
        rhi::IDevice* device = nullptr;

        rhi::ComPtr<rhi::IInputLayout>   inputLayout;
        rhi::ComPtr<rhi::IShaderProgram> shaderProgram;
        rhi::ComPtr<rhi::ISampler>       samplerLinear;
        rhi::ComPtr<rhi::ISampler>       samplerNearest;
        rhi::ComPtr<rhi::ITexture>       dummyTexture;
        rhi::ComPtr<rhi::ITextureView>   dummyTextureView;

        std::map<std::pair<rhi::Format, rhi::Format>, rhi::ComPtr<rhi::IRenderPipeline>> pipelines;

        int flags = 0;

        // Texture storage
        std::vector<SlangRHINVGtexture> textures;
        int nextTextureId = 1;

        // Current frame accumulated geometry
        std::vector<SlangRHINVGcall> calls;
        std::vector<SlangRHINVGpath> paths;
        std::vector<NVGvertex>       verts;
        std::vector<FragUniforms>    uniforms;

        // Render target viewport dimensions for active draw
        float viewWidth  = 0.0f;
        float viewHeight = 0.0f;
        float devicePixelRatio = 1.0f;

        // Dynamic vertex buffer
        static const int kMaxFramesInFlight = 3;
        rhi::ComPtr<rhi::IBuffer> vertexBuffers[kMaxFramesInFlight];
        uint64_t vertexBufferSizes[kMaxFramesInFlight] = {0};
        int frameIndex = 0;
    };

    const std::string slang_nvg_shader_src = R"(
struct VertexInput
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 pos2d    : TEXCOORD1;
    float2 uv       : TEXCOORD0;
};

uniform float2 uViewSize;

uniform float4 uScmat;
uniform float2 uScOffset;
uniform float2 uScExtent;

uniform float4 uPaintMat;
uniform float2 uPaintOffset;
uniform float4 uInnerCol;
uniform float4 uOuterCol;
uniform float2 uExtent;
uniform float  uRadius;
uniform float  uFeather;
uniform float  uStrokeMult;
uniform float  uStrokeThr;
uniform int    uTexType;
uniform int    uShaderType;

Texture2D uTexture;
SamplerState uSampler;

float nvg_sdroundrect(float2 pt, float2 ext, float rad)
{
    float2 ext2 = ext - float2(rad, rad);
    float2 d = abs(pt) - ext2;
    return min(max(d.x, d.y), 0.0f) + length(max(d, float2(0.0f, 0.0f))) - rad;
}

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(2.0f * input.pos.x / uViewSize.x - 1.0f, 1.0f - 2.0f * input.pos.y / uViewSize.y, 0.0f, 1.0f);
    output.pos2d = input.pos;
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Scissor calculation
    float scissor_alpha = 1.0f;
    if (uScExtent.x > -0.5f && uScExtent.y > -0.5f)
    {
        float2 scpt = float2(
            uScmat.x * input.pos2d.x + uScmat.z * input.pos2d.y + uScOffset.x,
            uScmat.y * input.pos2d.x + uScmat.w * input.pos2d.y + uScOffset.y
        );
        float sd = nvg_sdroundrect(scpt, uScExtent, 0.0f);
        scissor_alpha = clamp(0.5f - sd, 0.0f, 1.0f);
    }

    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (uShaderType == 0) // Gradient / Box gradient
    {
        float2 pt = float2(
            uPaintMat.x * input.pos2d.x + uPaintMat.z * input.pos2d.y + uPaintOffset.x,
            uPaintMat.y * input.pos2d.x + uPaintMat.w * input.pos2d.y + uPaintOffset.y
        );
        float d = nvg_sdroundrect(pt, uExtent, uRadius);
        float factor = clamp((d + uFeather * 0.5f) / uFeather, 0.0f, 1.0f);
        color = lerp(uInnerCol, uOuterCol, factor);
    }
    else if (uShaderType == 1) // Texture
    {
        float2 pt = float2(
            uPaintMat.x * input.pos2d.x + uPaintMat.z * input.pos2d.y + uPaintOffset.x,
            uPaintMat.y * input.pos2d.x + uPaintMat.w * input.pos2d.y + uPaintOffset.y
        );
        float2 texCoord = pt / uExtent;
        float4 texColor = uTexture.Sample(uSampler, texCoord);

        if (uTexType == 2) // Alpha texture (fonts)
        {
            color = float4(1.0f, 1.0f, 1.0f, texColor.r) * uInnerCol;
        }
        else // RGBA texture
        {
            color = texColor * uInnerCol;
        }
    }
    else // Simple anti-aliased edge (shaderType == 2)
    {
        color = uInnerCol;
    }

    // Stroke / Edge anti-alias fringe
    float strokeAlpha = clamp(input.uv.y, 0.0f, 1.0f);
    color.a *= strokeAlpha * scissor_alpha;

    return color;
}
)";

    SlangRHINVGtexture* findTexture(SlangRHINVGcontext* slangCtx, int id) {
        for (auto& tex : slangCtx->textures) {
            if (tex.id == id) return &tex;
        }
        return nullptr;
    }

    int slangRHINVG_renderCreate(void* uptr) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        rhi::IDevice* device = slangCtx->device;

        std::string moduleName = "nanovg_shader_" + std::to_string((uintptr_t)slangCtx);
        std::string pathName   = moduleName + ".slang";
        rhi::ComPtr<slang::IBlob> diagnostics;
        slang::IModule* module = device->getSlangSession()->loadModuleFromSourceString(moduleName.c_str(), pathName.c_str(), slang_nvg_shader_src.c_str(), diagnostics.writeRef());
        if (diagnostics) {
            std::cout << "NanoVG Shader compilation log:\n" << (const char*)diagnostics->getBufferPointer() << std::endl;
        }
        if (!module) {
            std::cerr << "NanoVG backend: Failed to compile shader module" << std::endl;
            return 0;
        }

        slang::IEntryPoint* vertEntry = nullptr;
        slang::IEntryPoint* fragEntry = nullptr;
        module->findEntryPointByName("vertexMain", &vertEntry);
        module->findEntryPointByName("fragmentMain", &fragEntry);

        std::vector<slang::IComponentType*> entryPoints = { vertEntry, fragEntry };

        rhi::ShaderProgramDesc programDesc = {};
        programDesc.linkingStyle = rhi::LinkingStyle::SingleProgram;
        programDesc.slangEntryPoints = entryPoints.data();
        programDesc.slangEntryPointCount = (uint32_t)entryPoints.size();
        programDesc.slangGlobalScope = module;

        if (SLANG_FAILED(device->createShaderProgram(programDesc, slangCtx->shaderProgram.writeRef(), diagnostics.writeRef()))) {
            std::cerr << "NanoVG backend: Failed to create shader program" << std::endl;
            return 0;
        }

        // Create Input Layout
        rhi::VertexStreamDesc vertexStreams[] = {
            { sizeof(NVGvertex), rhi::InputSlotClass::PerVertex, 0 }
        };
        rhi::InputElementDesc inputElements[] = {
            { "POSITION", 0, rhi::Format::RG32Float, offsetof(NVGvertex, x), 0 },
            { "TEXCOORD", 0, rhi::Format::RG32Float, offsetof(NVGvertex, u), 0 },
        };
        rhi::InputLayoutDesc inputLayoutDesc = {};
        inputLayoutDesc.inputElements = inputElements;
        inputLayoutDesc.inputElementCount = 2;
        inputLayoutDesc.vertexStreams = vertexStreams;
        inputLayoutDesc.vertexStreamCount = 1;

        if (SLANG_FAILED(device->createInputLayout(inputLayoutDesc, slangCtx->inputLayout.writeRef()))) {
            std::cerr << "NanoVG backend: Failed to create input layout" << std::endl;
            return 0;
        }

        // Create Samplers
        rhi::SamplerDesc sampDesc = {};
        sampDesc.minFilter = rhi::TextureFilteringMode::Linear;
        sampDesc.magFilter = rhi::TextureFilteringMode::Linear;
        sampDesc.mipFilter = rhi::TextureFilteringMode::Linear;
        sampDesc.addressU = rhi::TextureAddressingMode::ClampToEdge;
        sampDesc.addressV = rhi::TextureAddressingMode::ClampToEdge;
        sampDesc.addressW = rhi::TextureAddressingMode::ClampToEdge;
        device->createSampler(sampDesc, slangCtx->samplerLinear.writeRef());

        sampDesc.minFilter = rhi::TextureFilteringMode::Point;
        sampDesc.magFilter = rhi::TextureFilteringMode::Point;
        sampDesc.mipFilter = rhi::TextureFilteringMode::Point;
        device->createSampler(sampDesc, slangCtx->samplerNearest.writeRef());

        // Create 1x1 Dummy White Texture
        rhi::TextureDesc dummyDesc = {};
        dummyDesc.type = rhi::TextureType::Texture2D;
        dummyDesc.size = { 1, 1, 1 };
        dummyDesc.arrayLength = 1;
        dummyDesc.mipCount = 1;
        dummyDesc.format = rhi::Format::RGBA8Unorm;
        dummyDesc.usage = rhi::TextureUsage::ShaderResource;
        dummyDesc.defaultState = rhi::ResourceState::ShaderResource;
        dummyDesc.label = "NanoVG Dummy White Texture";

        uint32_t whitePixel = 0xFFFFFFFF;
        rhi::SubresourceData initData = {};
        initData.data = &whitePixel;
        initData.rowPitch = 4;

        if (SLANG_SUCCEEDED(device->createTexture(dummyDesc, &initData, slangCtx->dummyTexture.writeRef()))) {
            slangCtx->dummyTextureView = slangCtx->dummyTexture->getDefaultView();
        }

        return 1;
    }

    int slangRHINVG_renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;

        SlangRHINVGtexture tex;
        tex.id = slangCtx->nextTextureId++;
        tex.type = type;
        tex.width = w;
        tex.height = h;
        tex.flags = imageFlags;
        tex.external = false;

        rhi::TextureDesc desc = {};
        desc.type = rhi::TextureType::Texture2D;
        desc.size = { (uint32_t)w, (uint32_t)h, 1 };
        desc.arrayLength = 1;
        desc.mipCount = 1;
        desc.format = (type == NVG_TEXTURE_ALPHA) ? rhi::Format::R8Unorm : rhi::Format::RGBA8Unorm;
        desc.usage = rhi::TextureUsage::ShaderResource;
        desc.defaultState = rhi::ResourceState::ShaderResource;
        desc.label = "NanoVG Texture";

        rhi::SubresourceData initData = {};
        initData.data = data;
        initData.rowPitch = (type == NVG_TEXTURE_ALPHA) ? (rhi::Size)w : (rhi::Size)w * 4;

        if (SLANG_FAILED(slangCtx->device->createTexture(desc, data ? &initData : nullptr, tex.texture.writeRef()))) {
            std::cerr << "NanoVG backend: Failed to create texture" << std::endl;
            return 0;
        }

        tex.view = tex.texture->getDefaultView();
        slangCtx->textures.push_back(tex);
        return tex.id;
    }

    int slangRHINVG_renderDeleteTexture(void* uptr, int image) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        for (auto it = slangCtx->textures.begin(); it != slangCtx->textures.end(); ++it) {
            if (it->id == image) {
                slangCtx->textures.erase(it);
                return 1;
            }
        }
        return 0;
    }

    int slangRHINVG_renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        SlangRHINVGtexture* tex = findTexture(slangCtx, image);
        if (!tex || !tex->texture || !data) return 0;

        size_t bpp = (tex->type == NVG_TEXTURE_ALPHA) ? 1 : 4;
        size_t dataSize = (size_t)w * (size_t)h * bpp;

        TextureUpdate update;
        update.x = x;
        update.y = y;
        update.w = w;
        update.h = h;
        update.data.assign(data, data + dataSize);

        tex->pendingUpdates.push_back(std::move(update));
        return 1;
    }

    int slangRHINVG_renderGetTextureSize(void* uptr, int image, int* w, int* h) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        SlangRHINVGtexture* tex = findTexture(slangCtx, image);
        if (!tex) return 0;
        if (w) *w = tex->width;
        if (h) *h = tex->height;
        return 1;
    }

    void slangRHINVG_renderViewport(void* uptr, float width, float height, float devicePixelRatio) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        slangCtx->viewWidth = width;
        slangCtx->viewHeight = height;
        slangCtx->devicePixelRatio = devicePixelRatio;
    }

    void slangRHINVG_renderCancel(void* uptr) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        slangCtx->calls.clear();
        slangCtx->paths.clear();
        slangCtx->verts.clear();
        slangCtx->uniforms.clear();
    }

    void convertPaint(FragUniforms* frag, NVGpaint* paint, NVGscissor* scissor, float width, float fringe, int image) {
        std::memset(frag, 0, sizeof(*frag));

        frag->innerCol[0] = paint->innerColor.r;
        frag->innerCol[1] = paint->innerColor.g;
        frag->innerCol[2] = paint->innerColor.b;
        frag->innerCol[3] = paint->innerColor.a;

        frag->outerCol[0] = paint->outerColor.r;
        frag->outerCol[1] = paint->outerColor.g;
        frag->outerCol[2] = paint->outerColor.b;
        frag->outerCol[3] = paint->outerColor.a;

        // Scissor transform
        if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
            frag->scextent[0] = -1.0f;
            frag->scextent[1] = -1.0f;
        } else {
            float inv[6];
            nvgTransformInverse(inv, scissor->xform);
            frag->scmat[0] = inv[0];
            frag->scmat[1] = inv[1];
            frag->scmat[2] = inv[2];
            frag->scmat[3] = inv[3];
            frag->scoffset[0] = inv[4];
            frag->scoffset[1] = inv[5];
            frag->scextent[0] = scissor->extent[0];
            frag->scextent[1] = scissor->extent[1];
        }

        frag->extent[0] = paint->extent[0];
        frag->extent[1] = paint->extent[1];
        frag->radius = paint->radius;
        frag->feather = paint->feather;
        frag->strokeMult = (width < 1.0f) ? 1.0f : (1.0f / fringe);
        frag->strokeThr = -1.0f;

        if (paint->image != 0) {
            float inv[6];
            nvgTransformInverse(inv, paint->xform);
            frag->paintmat[0] = inv[0];
            frag->paintmat[1] = inv[1];
            frag->paintmat[2] = inv[2];
            frag->paintmat[3] = inv[3];
            frag->paintoffset[0] = inv[4];
            frag->paintoffset[1] = inv[5];
            frag->shaderType = 1; // Texture
        } else if (paint->radius > 0.0f || paint->feather > 0.0f) {
            float inv[6];
            nvgTransformInverse(inv, paint->xform);
            frag->paintmat[0] = inv[0];
            frag->paintmat[1] = inv[1];
            frag->paintmat[2] = inv[2];
            frag->paintmat[3] = inv[3];
            frag->paintoffset[0] = inv[4];
            frag->paintoffset[1] = inv[5];
            frag->shaderType = 0; // Gradient / Box gradient
        } else {
            frag->shaderType = 2; // Simple anti-aliased edge
        }
    }

    void slangRHINVG_renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;

        SlangRHINVGcall call;
        call.type = NVG_TRIANGLES;
        call.image = paint->image;
        call.triangleOffset = (int)slangCtx->verts.size();
        call.uniformOffset = (int)slangCtx->uniforms.size();

        FragUniforms frag;
        convertPaint(&frag, paint, scissor, 1.0f, fringe, paint->image);
        if (paint->image != 0) {
            SlangRHINVGtexture* tex = findTexture(slangCtx, paint->image);
            if (tex) frag.texType = (tex->type == NVG_TEXTURE_ALPHA) ? 2 : 1;
        }
        slangCtx->uniforms.push_back(frag);

        int vertStart = (int)slangCtx->verts.size();

        for (int i = 0; i < npaths; i++) {
            // Convert TriangleFan fill to TriangleList
            if (paths[i].nfill >= 3) {
                const NVGvertex* fill = paths[i].fill;
                for (int j = 2; j < paths[i].nfill; j++) {
                    slangCtx->verts.push_back(fill[0]);
                    slangCtx->verts.push_back(fill[j - 1]);
                    slangCtx->verts.push_back(fill[j]);
                }
            }
            // Convert TriangleStrip stroke fringe to TriangleList
            if (paths[i].nstroke >= 3) {
                const NVGvertex* stroke = paths[i].stroke;
                for (int j = 2; j < paths[i].nstroke; j++) {
                    if (j % 2 == 0) {
                        slangCtx->verts.push_back(stroke[j - 2]);
                        slangCtx->verts.push_back(stroke[j - 1]);
                        slangCtx->verts.push_back(stroke[j]);
                    } else {
                        slangCtx->verts.push_back(stroke[j - 1]);
                        slangCtx->verts.push_back(stroke[j - 2]);
                        slangCtx->verts.push_back(stroke[j]);
                    }
                }
            }
        }

        call.triangleCount = (int)slangCtx->verts.size() - vertStart;
        if (call.triangleCount > 0) {
            slangCtx->calls.push_back(call);
        }
    }

    void slangRHINVG_renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;

        SlangRHINVGcall call;
        call.type = NVG_TRIANGLES;
        call.image = paint->image;
        call.triangleOffset = (int)slangCtx->verts.size();
        call.uniformOffset = (int)slangCtx->uniforms.size();

        FragUniforms frag;
        convertPaint(&frag, paint, scissor, strokeWidth, fringe, paint->image);
        if (paint->image != 0) {
            SlangRHINVGtexture* tex = findTexture(slangCtx, paint->image);
            if (tex) frag.texType = (tex->type == NVG_TEXTURE_ALPHA) ? 2 : 1;
        }
        slangCtx->uniforms.push_back(frag);

        int vertStart = (int)slangCtx->verts.size();

        for (int i = 0; i < npaths; i++) {
            // Convert TriangleStrip stroke to TriangleList
            if (paths[i].nstroke >= 3) {
                const NVGvertex* stroke = paths[i].stroke;
                for (int j = 2; j < paths[i].nstroke; j++) {
                    if (j % 2 == 0) {
                        slangCtx->verts.push_back(stroke[j - 2]);
                        slangCtx->verts.push_back(stroke[j - 1]);
                        slangCtx->verts.push_back(stroke[j]);
                    } else {
                        slangCtx->verts.push_back(stroke[j - 1]);
                        slangCtx->verts.push_back(stroke[j - 2]);
                        slangCtx->verts.push_back(stroke[j]);
                    }
                }
            }
        }

        call.triangleCount = (int)slangCtx->verts.size() - vertStart;
        if (call.triangleCount > 0) {
            slangCtx->calls.push_back(call);
        }
    }

    void slangRHINVG_renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;

        SlangRHINVGcall call;
        call.type = NVG_TRIANGLES;
        call.image = paint->image;
        call.triangleOffset = (int)slangCtx->verts.size();
        call.triangleCount = nverts;
        call.uniformOffset = (int)slangCtx->uniforms.size();

        FragUniforms frag;
        convertPaint(&frag, paint, scissor, 1.0f, fringe, paint->image);
        if (paint->image != 0) {
            SlangRHINVGtexture* tex = findTexture(slangCtx, paint->image);
            if (tex) frag.texType = (tex->type == NVG_TEXTURE_ALPHA) ? 2 : 1;
        }
        slangCtx->uniforms.push_back(frag);

        for (int i = 0; i < nverts; i++) {
            slangCtx->verts.push_back(verts[i]);
        }

        slangCtx->calls.push_back(call);
    }

    void slangRHINVG_renderFlush(void* uptr) {
        // Render commands are flushed and executed during nvgSlangRHIRender.
    }

    void slangRHINVG_renderDelete(void* uptr) {
        SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)uptr;
        if (!slangCtx) return;

        slangCtx->pipelines.clear();
        slangCtx->inputLayout.setNull();
        slangCtx->shaderProgram.setNull();
        slangCtx->samplerLinear.setNull();
        slangCtx->samplerNearest.setNull();
        slangCtx->dummyTexture.setNull();
        slangCtx->dummyTextureView.setNull();

        for (int i = 0; i < SlangRHINVGcontext::kMaxFramesInFlight; i++) {
            slangCtx->vertexBuffers[i].setNull();
        }

        delete slangCtx;
    }

} // namespace

NVGcontext* nvgCreateSlangRHI(rhi::IDevice* device, int flags) {
    if (!device) return nullptr;

    SlangRHINVGcontext* slangCtx = new SlangRHINVGcontext();
    slangCtx->device = device;
    slangCtx->flags  = flags;

    NVGparams params;
    std::memset(&params, 0, sizeof(params));
    params.userPtr               = slangCtx;
    params.edgeAntiAlias        = (flags & NVG_ANTIALIAS) ? 1 : 0;
    params.renderCreate         = slangRHINVG_renderCreate;
    params.renderCreateTexture  = slangRHINVG_renderCreateTexture;
    params.renderDeleteTexture  = slangRHINVG_renderDeleteTexture;
    params.renderUpdateTexture  = slangRHINVG_renderUpdateTexture;
    params.renderGetTextureSize = slangRHINVG_renderGetTextureSize;
    params.renderViewport       = slangRHINVG_renderViewport;
    params.renderCancel         = slangRHINVG_renderCancel;
    params.renderFlush          = slangRHINVG_renderFlush;
    params.renderFill           = slangRHINVG_renderFill;
    params.renderStroke         = slangRHINVG_renderStroke;
    params.renderTriangles      = slangRHINVG_renderTriangles;
    params.renderDelete         = slangRHINVG_renderDelete;

    NVGcontext* ctx = nvgCreateInternal(&params);
    if (!ctx) {
        delete slangCtx;
        return nullptr;
    }

    return ctx;
}

void nvgDeleteSlangRHI(NVGcontext* ctx) {
    if (ctx) {
        nvgDeleteInternal(ctx);
    }
}

bool nvgSlangRHICreatePipeline(NVGcontext* ctx, rhi::Format colorFormat, rhi::Format depthFormat) {
    if (!ctx) return false;
    NVGparams* params = nvgInternalParams(ctx);
    if (!params || !params->userPtr) return false;
    SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)params->userPtr;

    auto key = std::make_pair(colorFormat, depthFormat);
    if (slangCtx->pipelines.find(key) != slangCtx->pipelines.end()) {
        return true; // Pipeline already cached
    }

    rhi::ColorTargetDesc colorTarget = {};
    colorTarget.format = colorFormat;
    colorTarget.enableBlend = true;
    colorTarget.color.srcFactor = rhi::BlendFactor::SrcAlpha;
    colorTarget.color.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    colorTarget.color.op = rhi::BlendOp::Add;
    colorTarget.alpha.srcFactor = rhi::BlendFactor::Zero;
    colorTarget.alpha.dstFactor = rhi::BlendFactor::One;
    colorTarget.alpha.op = rhi::BlendOp::Add;

    rhi::DepthStencilDesc depthStencilDesc = {};
    depthStencilDesc.depthTestEnable = false;
    depthStencilDesc.depthWriteEnable = false;
    if (depthFormat != rhi::Format::Undefined) {
        depthStencilDesc.format = depthFormat;
    }

    rhi::RasterizerDesc rasterizerDesc = {};
    rasterizerDesc.fillMode = rhi::FillMode::Solid;
    rasterizerDesc.cullMode = rhi::CullMode::None;
    rasterizerDesc.scissorEnable = false;

    rhi::RenderPipelineDesc pipelineDesc = {};
    pipelineDesc.program = slangCtx->shaderProgram;
    pipelineDesc.inputLayout = slangCtx->inputLayout;
    pipelineDesc.targets = &colorTarget;
    pipelineDesc.targetCount = 1;
    pipelineDesc.depthStencil = depthStencilDesc;
    pipelineDesc.rasterizer = rasterizerDesc;
    pipelineDesc.primitiveTopology = rhi::PrimitiveTopology::TriangleList;

    rhi::ComPtr<rhi::IRenderPipeline> pipeline;
    if (SLANG_FAILED(slangCtx->device->createRenderPipeline(pipelineDesc, pipeline.writeRef()))) {
        std::cerr << "NanoVG backend: Failed to create render pipeline for requested formats" << std::endl;
        return false;
    }

    slangCtx->pipelines[key] = pipeline;
    return true;
}

void nvgSlangRHIUpdateTextures(NVGcontext* ctx, rhi::ICommandEncoder* commandEncoder) {
    if (!ctx || !commandEncoder) return;
    NVGparams* params = nvgInternalParams(ctx);
    if (!params || !params->userPtr) return;
    SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)params->userPtr;

    for (auto& tex : slangCtx->textures) {
        if (!tex.pendingUpdates.empty() && tex.texture) {
            for (const auto& update : tex.pendingUpdates) {
                rhi::SubresourceData subData = {};
                subData.data = update.data.data();
                subData.rowPitch = (tex.type == NVG_TEXTURE_ALPHA) ? (rhi::Size)update.w : (rhi::Size)update.w * 4;

                commandEncoder->uploadTextureData(
                    tex.texture,
                    rhi::SubresourceRange{ 0, 1, 0, 1 },
                    { (uint32_t)update.x, (uint32_t)update.y, 0 },
                    { (uint32_t)update.w, (uint32_t)update.h, 1 },
                    &subData,
                    1
                );
            }
            tex.pendingUpdates.clear();
        }
    }
}

void nvgSlangRHIRender(NVGcontext*             ctx,
                       rhi::ICommandEncoder*    commandEncoder,
                       rhi::IRenderPassEncoder* renderPassEncoder,
                       float                    width,
                       float                    height) {
    if (!ctx || !commandEncoder || !renderPassEncoder) return;

    NVGparams* params = nvgInternalParams(ctx);
    if (!params || !params->userPtr) return;
    SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)params->userPtr;

    if (slangCtx->verts.empty() || slangCtx->calls.empty()) {
        slangCtx->calls.clear();
        slangCtx->paths.clear();
        slangCtx->verts.clear();
        slangCtx->uniforms.clear();
        return;
    }

    slangCtx->frameIndex = (slangCtx->frameIndex + 1) % SlangRHINVGcontext::kMaxFramesInFlight;

    // Ensure vertex buffer size
    uint64_t vertexSizeNeeded = slangCtx->verts.size() * sizeof(NVGvertex);
    rhi::ComPtr<rhi::IBuffer>& vertexBuffer = slangCtx->vertexBuffers[slangCtx->frameIndex];
    uint64_t& vertexBufferSize = slangCtx->vertexBufferSizes[slangCtx->frameIndex];

    if (!vertexBuffer || vertexBufferSize < vertexSizeNeeded) {
        vertexBuffer.setNull();
        vertexBufferSize = vertexSizeNeeded + 1000 * sizeof(NVGvertex);
        rhi::BufferDesc desc = {};
        desc.size = vertexBufferSize;
        desc.usage = rhi::BufferUsage::VertexBuffer;
        desc.defaultState = rhi::ResourceState::VertexBuffer;
        desc.memoryType = rhi::MemoryType::Upload;
        desc.label = "NanoVG Vertex Buffer";

        if (SLANG_FAILED(slangCtx->device->createBuffer(desc, nullptr, vertexBuffer.writeRef()))) {
            std::cerr << "NanoVG backend: Failed to allocate vertex buffer" << std::endl;
            return;
        }
    }

    // Map vertex buffer
    void* mappedData = nullptr;
    if (SLANG_SUCCEEDED(slangCtx->device->mapBuffer(vertexBuffer, rhi::CpuAccessMode::Write, &mappedData))) {
        std::memcpy(mappedData, slangCtx->verts.data(), vertexSizeNeeded);
        slangCtx->device->unmapBuffer(vertexBuffer);
    } else {
        std::cerr << "NanoVG backend: Failed to map vertex buffer" << std::endl;
        return;
    }

    if (slangCtx->pipelines.empty()) {
        std::cerr << "NanoVG backend: No render pipelines created! Call nvgSlangRHICreatePipeline first." << std::endl;
        return;
    }

    rhi::IRenderPipeline* pipeline = slangCtx->pipelines.begin()->second.get();

    float viewSize[2] = { width, height };

    // Setup Render State
    rhi::RenderState renderState = {};
    renderState.viewports[0] = rhi::Viewport::fromSize(width, height);
    renderState.viewportCount = 1;
    renderState.scissorRects[0] = rhi::ScissorRect{ 0, 0, (uint32_t)width, (uint32_t)height };
    renderState.scissorRectCount = 1;
    renderState.vertexBuffers[0].buffer = vertexBuffer;
    renderState.vertexBuffers[0].offset = 0;
    renderState.vertexBufferCount = 1;

    renderPassEncoder->setRenderState(renderState);

    // Issue draw calls
    for (const auto& call : slangCtx->calls) {
        rhi::ITextureView* texView = slangCtx->dummyTextureView.get();
        rhi::ISampler*     sampler = slangCtx->samplerLinear.get();

        if (call.image != 0) {
            SlangRHINVGtexture* tex = findTexture(slangCtx, call.image);
            if (tex && tex->view) {
                texView = tex->view.get();
                if (tex->flags & NVG_IMAGE_NEAREST) {
                    sampler = slangCtx->samplerNearest.get();
                }
            }
        }

        const FragUniforms& frag = slangCtx->uniforms[call.uniformOffset];

        rhi::ShaderCursor cursor(renderPassEncoder->bindPipeline(pipeline));
        cursor["uViewSize"].setData(viewSize, sizeof(viewSize));
        cursor["uScmat"].setData(frag.scmat, sizeof(frag.scmat));
        cursor["uScOffset"].setData(frag.scoffset, sizeof(frag.scoffset));
        cursor["uScExtent"].setData(frag.scextent, sizeof(frag.scextent));
        cursor["uPaintMat"].setData(frag.paintmat, sizeof(frag.paintmat));
        cursor["uPaintOffset"].setData(frag.paintoffset, sizeof(frag.paintoffset));
        cursor["uInnerCol"].setData(frag.innerCol, sizeof(frag.innerCol));
        cursor["uOuterCol"].setData(frag.outerCol, sizeof(frag.outerCol));
        cursor["uExtent"].setData(frag.extent, sizeof(frag.extent));
        cursor["uRadius"].setData(&frag.radius, sizeof(frag.radius));
        cursor["uFeather"].setData(&frag.feather, sizeof(frag.feather));
        cursor["uStrokeMult"].setData(&frag.strokeMult, sizeof(frag.strokeMult));
        cursor["uStrokeThr"].setData(&frag.strokeThr, sizeof(frag.strokeThr));
        cursor["uTexType"].setData(&frag.texType, sizeof(frag.texType));
        cursor["uShaderType"].setData(&frag.shaderType, sizeof(frag.shaderType));

        cursor["uTexture"].setBinding(texView);
        cursor["uSampler"].setBinding(sampler);

        if (call.triangleCount > 0) {
            rhi::DrawArguments drawArgs = {};
            drawArgs.vertexCount = call.triangleCount;
            drawArgs.startVertexLocation = call.triangleOffset;
            renderPassEncoder->draw(drawArgs);
        }
    }

    // Reset frame geometry collections
    slangCtx->calls.clear();
    slangCtx->paths.clear();
    slangCtx->verts.clear();
    slangCtx->uniforms.clear();
}

int nvgSlangRHICreateImage(NVGcontext* ctx, rhi::ITextureView* textureView) {
    if (!ctx || !textureView) return 0;
    NVGparams* params = nvgInternalParams(ctx);
    if (!params || !params->userPtr) return 0;
    SlangRHINVGcontext* slangCtx = (SlangRHINVGcontext*)params->userPtr;

    SlangRHINVGtexture tex;
    tex.id = slangCtx->nextTextureId++;
    tex.view = textureView;
    tex.type = NVG_TEXTURE_RGBA;
    tex.external = true;

    slangCtx->textures.push_back(tex);
    return tex.id;
}

void nvgSlangRHIDeleteImage(NVGcontext* ctx, int image) {
    if (!ctx || image <= 0) return;
    slangRHINVG_renderDeleteTexture(nvgInternalParams(ctx)->userPtr, image);
}
