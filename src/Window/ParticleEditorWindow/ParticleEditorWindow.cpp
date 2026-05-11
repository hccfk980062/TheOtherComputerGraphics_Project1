#include "Window/ParticleEditorWindow/ParticleEditorWindow.h"

namespace CG
{
    bool ParticleEditorWindow::Initialize()
    {
        m_scene = std::make_unique<ParticleEditorScene>();
        return m_scene->Initialize(800, 600);
    }

    void ParticleEditorWindow::Display()
    {
        if (!open) return;

        ImGui::SetNextWindowSize(ImVec2(1100, 700), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Particle Editor", &open, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImVec2 avail      = ImGui::GetContentRegionAvail();
        float  topHeight  = avail.y - m_sequencerHeight - ImGui::GetStyle().ItemSpacing.y;
        if (topHeight < 80.0f) topHeight = 80.0f;

        // ── 左側面板：Hierarchy（上半） + Inspector（下半） ───────────────────
        ImGui::BeginChild("##pe_left", ImVec2(m_leftPanelWidth, topHeight), false,
                          ImGuiWindowFlags_NoScrollbar);
        {
            float halfH = ImGui::GetContentRegionAvail().y * 0.45f;

            ImGui::BeginChild("##pe_hier", ImVec2(0, halfH), true);
            m_hierarchy.Display(m_scene.get());
            ImGui::EndChild();

            ImGui::BeginChild("##pe_insp", ImVec2(0, 0), true);
            m_inspector.Display(m_scene.get());
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // ── 右側：Viewport ────────────────────────────────────────────────────
        ImGui::BeginChild("##pe_viewport", ImVec2(0, topHeight), false);
        m_viewport.Display(m_scene.get());
        ImGui::EndChild();

        // ── 底部：Sequencer（全寬）────────────────────────────────────────────
        ImGui::BeginChild("##pe_seq_wrap", ImVec2(0, m_sequencerHeight), true);
        m_sequencer.Display(m_scene.get());
        ImGui::EndChild();

        ImGui::End();
    }

} // namespace CG
