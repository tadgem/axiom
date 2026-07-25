#pragma once

#include <im3d.h>
#include <slang-rhi.h>

bool Im3d_ImplSlangRHI_Init(rhi::IDevice* device,
                            rhi::Format defaultRenderTargetFormat = rhi::Format::Undefined,
                            rhi::Format defaultDepthFormat        = rhi::Format::Undefined);

void Im3d_ImplSlangRHI_Shutdown();

void Im3d_ImplSlangRHI_SetLineWidthScale(float scale);
float Im3d_ImplSlangRHI_GetLineWidthScale();

void Im3d_ImplSlangRHI_NewFrame(float        deltaTime,
                                float        viewportWidth,
                                float        viewportHeight,
                                const float* viewMatrix4x4,
                                const float* projMatrix4x4,
                                const float* camPos3 = nullptr,
                                const float* camDir3 = nullptr,
                                float        fovY    = 0.785398f);

void Im3d_ImplSlangRHI_RenderDrawData(rhi::ICommandEncoder*    commandEncoder,
                                      rhi::IRenderPassEncoder* renderPassEncoder,
                                      float                    viewportWidth,
                                      float                    viewportHeight,
                                      const float*             viewProjMatrix4x4 = nullptr,
                                      rhi::Format              colorFormat       = rhi::Format::Undefined,
                                      rhi::Format              depthFormat       = rhi::Format::Undefined,
                                      float                    lineWidthScale    = 0.0f);
