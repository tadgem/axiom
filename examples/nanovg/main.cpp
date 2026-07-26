#include <cmath>
#include "Axiom.hpp"

extern "C" const unsigned char archivo_regular_ttf[1269520];

AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

namespace {

    void DrawGlassPanel(NVGcontext* nvg, float x, float y, float w, float h, const char* title) {
        NVGpaint shadowPaint
                = nvgBoxGradient(nvg, x, y + 2.0f, w, h, 8.0f, 10.0f, nvgRGBA(0, 0, 0, 120), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(nvg);
        nvgRect(nvg, x - 10.0f, y - 10.0f, w + 20.0f, h + 20.0f);
        nvgFillPaint(nvg, shadowPaint);
        nvgFill(nvg);

        NVGpaint bodyPaint = nvgLinearGradient(nvg, x, y, x, y + h, nvgRGBA(32, 38, 52, 220), nvgRGBA(22, 26, 36, 230));
        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, x, y, w, h, 8.0f);
        nvgFillPaint(nvg, bodyPaint);
        nvgFill(nvg);

        nvgStrokeColor(nvg, nvgRGBA(255, 255, 255, 30));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);

        if (title) {
            nvgFontSize(nvg, 14.0f);
            nvgFontFace(nvg, "sans");
            nvgFillColor(nvg, nvgRGBA(200, 215, 235, 255));
            nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgText(nvg, x + 16.0f, y + 14.0f, title, nullptr);

            nvgBeginPath(nvg);
            nvgMoveTo(nvg, x + 16.0f, y + 36.0f);
            nvgLineTo(nvg, x + w - 16.0f, y + 36.0f);
            nvgStrokeColor(nvg, nvgRGBA(255, 255, 255, 20));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);
        }
    }

    void DrawLineGraph(NVGcontext* nvg, float x, float y, float w, float h, float time) {
        float points[10];
        for (int i = 0; i < 10; i++) {
            points[i] = y + h * 0.5f + std::sin(time * 2.0f + i * 0.6f) * (h * 0.35f);
        }

        float    dx        = w / 9.0f;

        NVGpaint areaPaint = nvgLinearGradient(nvg, x, y, x, y + h, nvgRGBA(0, 200, 255, 100), nvgRGBA(0, 100, 255, 0));
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x, y + h);
        for (int i = 0; i < 10; i++) {
            float px = x + i * dx;
            float py = points[i];
            if (i == 0) {
                nvgLineTo(nvg, px, py);
            } else {
                float prevX = x + (i - 1) * dx;
                float prevY = points[i - 1];
                float cx1   = prevX + dx * 0.5f;
                float cx2   = px - dx * 0.5f;
                nvgBezierTo(nvg, cx1, prevY, cx2, py, px, py);
            }
        }
        nvgLineTo(nvg, x + w, y + h);
        nvgClosePath(nvg);
        nvgFillPaint(nvg, areaPaint);
        nvgFill(nvg);

        nvgBeginPath(nvg);
        for (int i = 0; i < 10; i++) {
            float px = x + i * dx;
            float py = points[i];
            if (i == 0) {
                nvgMoveTo(nvg, px, py);
            } else {
                float prevX = x + (i - 1) * dx;
                float prevY = points[i - 1];
                float cx1   = prevX + dx * 0.5f;
                float cx2   = px - dx * 0.5f;
                nvgBezierTo(nvg, cx1, prevY, cx2, py, px, py);
            }
        }
        nvgStrokeColor(nvg, nvgRGBA(0, 220, 255, 255));
        nvgStrokeWidth(nvg, 2.5f);
        nvgStroke(nvg);

        for (int i = 0; i < 10; i++) {
            float px = x + i * dx;
            float py = points[i];

            nvgBeginPath(nvg);
            nvgCircle(nvg, px, py, 4.0f);
            nvgFillColor(nvg, nvgRGBA(10, 15, 25, 255));
            nvgFill(nvg);

            nvgBeginPath(nvg);
            nvgCircle(nvg, px, py, 2.5f);
            nvgFillColor(nvg, nvgRGBA(0, 240, 255, 255));
            nvgFill(nvg);
        }
    }

    void DrawProgressRing(NVGcontext* nvg, float cx, float cy, float radius, float progress, const char* label) {
        nvgBeginPath(nvg);
        nvgCircle(nvg, cx, cy, radius);
        nvgStrokeColor(nvg, nvgRGBA(255, 255, 255, 20));
        nvgStrokeWidth(nvg, 8.0f);
        nvgStroke(nvg);

        float startAngle = -NVG_PI * 0.5f;
        float endAngle   = startAngle + progress * NVG_PI * 2.0f;

        nvgBeginPath(nvg);
        nvgArc(nvg, cx, cy, radius, startAngle, endAngle, NVG_CW);
        nvgStrokeColor(nvg, nvgRGBA(255, 80, 160, 255));
        nvgStrokeWidth(nvg, 8.0f);
        nvgLineCap(nvg, NVG_ROUND);
        nvgStroke(nvg);

        char percentStr[16];
        std::snprintf(percentStr, sizeof(percentStr), "%d%%", (int) (progress * 100.0f));
        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "sans");
        nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
        nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(nvg, cx, cy - 2.0f, percentStr, nullptr);

        if (label) {
            nvgFontSize(nvg, 11.0f);
            nvgFillColor(nvg, nvgRGBA(160, 175, 200, 255));
            nvgText(nvg, cx, cy + radius + 16.0f, label, nullptr);
        }
    }

    void RenderNanoVGDemoCanvas(NVGcontext* nvg, float width, float height, float animTime, float frameMs) {
        nvgBeginFrame(nvg, width, height, 1.0f);

        // Background Gradient
        NVGpaint bgPaint = nvgRadialGradient(nvg,
                                             width * 0.5f,
                                             height * 0.5f,
                                             width * 0.2f,
                                             width * 0.7f,
                                             nvgRGBA(25, 30, 44, 255),
                                             nvgRGBA(10, 12, 18, 255));
        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, width, height);
        nvgFillPaint(nvg, bgPaint);
        nvgFill(nvg);

        // Header Bar
        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, width, 50.0f);
        nvgFillColor(nvg, nvgRGBA(18, 22, 32, 230));
        nvgFill(nvg);

        nvgFontSize(nvg, 20.0f);
        nvgFontFace(nvg, "sans");
        nvgFillColor(nvg, nvgRGBA(0, 220, 255, 255));
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(nvg, 20.0f, 25.0f, "AXIOM", nullptr);

        nvgFontSize(nvg, 14.0f);
        nvgFillColor(nvg, nvgRGBA(220, 230, 245, 255));
        nvgText(nvg, 95.0f, 25.0f, "NanoVG 2D Vector Graphics (Swapchain Render Pass)", nullptr);

        char fpsStr[64];
        std::snprintf(
                fpsStr, sizeof(fpsStr), "%.2f ms (%.0f FPS)", frameMs, 1000.0f / (frameMs > 0.001f ? frameMs : 1.0f));
        nvgFontSize(nvg, 12.0f);
        nvgFillColor(nvg, nvgRGBA(0, 230, 180, 255));
        nvgTextAlign(nvg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(nvg, width - 20.0f, 25.0f, fpsStr, nullptr);

        NVGpaint linePaint
                = nvgLinearGradient(nvg, 0, 50, width, 50, nvgRGBA(0, 200, 255, 255), nvgRGBA(255, 80, 180, 255));
        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 48.0f, width, 2.0f);
        nvgFillPaint(nvg, linePaint);
        nvgFill(nvg);

        // Content Panels
        float p1X = 20.0f, p1Y = 70.0f, p1W = (width - 60.0f) * 0.6f, p1H = height - 90.0f;
        if (p1W > 100.0f && p1H > 100.0f) {
            DrawGlassPanel(nvg, p1X, p1Y, p1W, p1H, "TELEMETRY SPECTRUM");
            DrawLineGraph(nvg, p1X + 20.0f, p1Y + 50.0f, p1W - 40.0f, p1H - 70.0f, animTime);
        }

        float p2X = p1X + p1W + 20.0f, p2Y = 70.0f, p2W = (width - 60.0f) * 0.4f, p2H = height - 90.0f;
        if (p2W > 100.0f && p2H > 100.0f) {
            DrawGlassPanel(nvg, p2X, p2Y, p2W, p2H, "SYSTEM METRICS");
            float ring1Progress = 0.5f + std::sin(animTime * 1.5f) * 0.4f;
            float ring2Progress = 0.5f + std::cos(animTime * 2.0f) * 0.35f;
            float ringSpacing   = p2W / 3.0f;

            if (p2H > 160.0f) {
                DrawProgressRing(nvg, p2X + ringSpacing, p2Y + 120.0f, 36.0f, ring1Progress, "GPU");
                DrawProgressRing(nvg, p2X + ringSpacing * 2.0f, p2Y + 120.0f, 36.0f, ring2Progress, "RAM");
            }
        }

        nvgEndFrame(nvg);
    }

} // namespace

int main() {
    AxiomEngine init = engine::Init();
    AXM_ASSERT(init.m_OK, "Failed to initialize Axiom Engine");

    rhi::Format swapchainFormat = init.m_GPU.m_Surface->getInfo().preferredFormat;
    rhi::Format depthFormat     = rhi::Format::D32Float;

    // Create NanoVG Slang-RHI Context
    NVGcontext* nvg = axm::nanovg::CreateContext(init.m_GPU.m_Device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    AXM_ASSERT(nvg != nullptr, "Failed to create NanoVG context");

    // Pre-create Slang-RHI render pipeline for swapchain format
    bool pipelineCreated = axm::nanovg::CreatePipeline(nvg, swapchainFormat, depthFormat);
    AXM_ASSERT(pipelineCreated, "Failed to create NanoVG Slang-RHI render pipeline");

    // Load font from embedded memory array
    int fontSans = nvgCreateFontMem(nvg, "sans", (unsigned char*) archivo_regular_ttf, sizeof(archivo_regular_ttf), 0);
    AXM_ASSERT(fontSans != -1, "Failed to load font into NanoVG");

    AXM_LOG("NanoVG & ImGui Harmonious Swapchain Demo Initialized");

    float animTime = 0.0f;

    while (init.m_Running) {
        if (engine::PreFrame(init)) {
            animTime += (float) init.m_DeltaTime * 0.001f;
            auto  viewport = viewports::GetFullscreenViewport(init.m_Window.m_Window);
            float width    = viewport.m_Size.x;
            float height   = viewport.m_Size.y;

            // 1. Accumulate NanoVG 2D vector draw commands
            RenderNanoVGDemoCanvas(nvg, width, height, animTime, (float) init.m_DeltaTime);

            // 2. Encode command buffer
            auto commandEncoder = init.m_GPU.m_Queue->createCommandEncoder();

            // 3. Upload pending font/image textures outside render pass
            axm::nanovg::UpdateTextures(nvg, commandEncoder);

            // 4. SWAPCHAIN PASS 1: Clear swapchain (color & depth) and render NanoVG 2D vector graphics
            auto passEncoder = render_pass::BeginSwapChainRenderPass(init,
                                                                     commandEncoder,
                                                                     rhi::LoadOp::Clear,
                                                                     rhi::LoadOp::Clear,
                                                                     true,
                                                                     aml::Vec4(0.06f, 0.07f, 0.10f, 1.0f));

            axm::nanovg::Render(nvg, commandEncoder, passEncoder, width, height);

            passEncoder->end();
            init.m_GPU.m_Queue->submit(commandEncoder->finish());
        }

        // --- ImGui Floating UI Overlay ---
        // Style ImGui window with translucent background so NanoVG vector graphics below render in harmony
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.15f, 0.22f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.45f, 0.65f, 0.50f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        if (ImGui::Begin("NanoVG & ImGui Swapchain Info")) {
            ImGui::Text("Swapchain Rendering Operations:");
            ImGui::BulletText("Pass 1 (NanoVG): LoadOp::Clear (Clears swapchain & draws 2D vector canvas)");
            ImGui::BulletText("Pass 2 (ImGui) : LoadOp::Load  (Preserves NanoVG swapchain image & overlays UI)");
            ImGui::Separator();
            ImGui::Text("Animation Time: %.2f s", animTime);
        }
        ImGui::End();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // 5. SWAPCHAIN PASS 2 (In engine::PostFrame): Opens swapchain pass with LoadOp::Load to draw ImGui over NanoVG
        engine::PostFrame(init);
    }

    axm::nanovg::DestroyContext(nvg);
    engine::Quit(init);
    return 0;
}
