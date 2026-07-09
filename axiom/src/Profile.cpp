#include "Core/Profile.hpp"
#include "Core/Engine.hpp"

#include "imgui.h"
namespace axm {
    static HashMap<const char*, profiler::ProfilerItem> g_ProfilerItems = { };
}
axm::profiler::ScopedTimer::ScopedTimer(const char* label) : m_Label(label) { }
// axm::profiler::ScopedTimer::ScopedTimer(std::source_location loc) : m_Label(loc.function_name()) { }

axm::profiler::ScopedTimer::~ScopedTimer() {

    auto duration = m_Timer.ElapsedNanosecondsF();

    if (!g_ProfilerItems.contains(m_Label)) {
        g_ProfilerItems[m_Label] = ProfilerItem { };
    }

    g_ProfilerItems[m_Label].m_MeanDuration += duration;
    g_ProfilerItems[m_Label].m_MeanDuration /= 2;

    if (g_ProfilerItems[m_Label].m_MinDuration > duration) {
        g_ProfilerItems[m_Label].m_MinDuration = duration;
    }

    if (g_ProfilerItems[m_Label].m_MaxDuration < duration) {
        g_ProfilerItems[m_Label].m_MaxDuration = duration;
    }

    // find bin to put the function in.
}

static axm::Vector<const char*> g_ProfilerItemLabelsSorted;

// Helper struct to allow profiler items to be sorted:
struct CompareItemsFromMap
{
    const axm::HashMap<const char*, axm::profiler::ProfilerItem>& Map;
    const ImGuiTableSortSpecs*                                    Specs;

    CompareItemsFromMap(const axm::HashMap<const char*, axm::profiler::ProfilerItem>& map,
                        const ImGuiTableSortSpecs* specs) : Map(map), Specs(specs) { }
    bool operator()(const char* key_a, const char* key_b) const {
        using namespace axm;
        const profiler::ProfilerItem& a = Map.at(key_a);
        const profiler::ProfilerItem& b = Map.at(key_b);
        for (int n = 0; n < Specs->SpecsCount; n++) {
            const ImGuiTableColumnSortSpecs* spec  = &Specs->Specs[n];

            f64                              delta = 0;
            switch (spec->ColumnIndex) {
                case 1: // Mean Duration
                    delta = (a.m_MeanDuration - b.m_MeanDuration);
                    break;
                case 2: // Min Duration
                    delta = a.m_MinDuration - b.m_MinDuration;
                    break;
                case 3: // Max Duration
                    delta = b.m_MaxDuration > a.m_MaxDuration ? 1.0f : -1.0f;
                    break;
            }
            if (delta != 0) {
                if (spec->SortDirection == ImGuiSortDirection_Ascending) {
                    return delta < 0.0f;
                } else {
                    return delta > 0.0f;
                }
            }
        }
        // Fallback for stable sort
        return a.m_MeanDuration < b.m_MeanDuration;
    }
};


void axm::profiler::ProfilerImGuiWindow(const AppState& e) {

    constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter
                                      | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable
                                      | ImGuiTableFlags_Sortable;

    if (g_ProfilerItemLabelsSorted.size() != g_ProfilerItems.size()) {
        g_ProfilerItemLabelsSorted.clear();
        for (const auto& pair: g_ProfilerItems) {
            g_ProfilerItemLabelsSorted.push_back(pair.first);
        }
        // Force a resort since the keys vector was rebuilt
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            sort_specs->SpecsDirty = true;
        }
    }

    ImGui::Begin("Axiom Profiler Stats");
    ImGui::Text("Frame Time : %.2f, FPS : %.2f", e.m_DeltaTime, 1000.0 / e.m_DeltaTime);
    if (ImGui::BeginTable("Axiom Profiler Stats", 4, flags)) {

        ImGui::TableSetupColumn("Label");
        ImGui::TableSetupColumn("Mean Duration (ms)");
        ImGui::TableSetupColumn("Min Duration (ms)");
        ImGui::TableSetupColumn("Max Duration (ms)");

        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty) {
                std::sort(g_ProfilerItemLabelsSorted.begin(),
                          g_ProfilerItemLabelsSorted.end(),
                          CompareItemsFromMap(g_ProfilerItems, sort_specs));
                sort_specs->SpecsDirty = false; // Mark dirty flag as resolved
            }
        }

        ImGui::TableHeadersRow();
        for (const auto& label: g_ProfilerItemLabelsSorted) {

            const auto& stats = g_ProfilerItems[label];
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f ms", stats.m_MeanDuration / 1000000.0);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f ms", stats.m_MinDuration / 1000000.0);
            ImGui::TableNextColumn();
            ImGui::Text("%.4f ms", stats.m_MaxDuration / 1000000.0);
            ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
