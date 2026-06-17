#pragma once

#include <imgui.h>
#include <slang-rhi.h>

bool ImGui_ImplSlangRHI_Init(rhi::IDevice* device, rhi::Format renderTargetFormat);
void ImGui_ImplSlangRHI_Shutdown();
void ImGui_ImplSlangRHI_NewFrame();
void ImGui_ImplSlangRHI_RenderDrawData(ImDrawData* drawData, rhi::ICommandEncoder* commandEncoder, rhi::IRenderPassEncoder* renderPassEncoder);
