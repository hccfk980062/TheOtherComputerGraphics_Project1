#include <imgui.h>
#include <ImNeoSequencer/imgui_neo_sequencer.h>
#include <ImNeoSequencer/imgui_neo_internal.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>

#include "Window/ParticleEditorWindow/ParticleSequencerWindow.h"
#include "ParticleEffects/ParticleSerializer.h"

namespace CG
{
    ParticleSequencerWindow::ParticleSequencerWindow()  {}
    ParticleSequencerWindow::~ParticleSequencerWindow() {}

    bool ParticleSequencerWindow::Initialize() { return true; }

    void ParticleSequencerWindow::Display()
    {
        if (!ImGui::Begin("Particle Sequencer"))
        {
            ImGui::End();
            return;
        }

        if (!m_scene)
        {
            ImGui::TextDisabled("(no scene)");
            ImGui::End();
            return;
        }

        // ── Export / Import ───────────────────────────────────────────────────
        if (ImGui::Button("Export"))
        {
            IGFD::FileDialogConfig cfg; cfg.path = "."; cfg.fileName = "particle_effect.json";
            ImGuiFileDialog::Instance()->OpenDialog("PE_Export", "Export Particle Effect", ".json", cfg);
        }
        ImGui::SameLine();
        if (ImGui::Button("Import"))
        {
            IGFD::FileDialogConfig cfg; cfg.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("PE_Import", "Import Particle Effect", ".json", cfg);
        }

        DoExport();
        DoImport();

        ImGui::Separator();

        // ── Playback controls ─────────────────────────────────────────────────
        if (ImGui::Button("|<"))
            m_scene->m_currentFrame = m_scene->m_startFrame;
        ImGui::SameLine();

        if (ImGui::Button(m_scene->m_isPlaying ? "Pause" : " Play "))
            m_scene->m_isPlaying = !m_scene->m_isPlaying;
        ImGui::SameLine();

        ImGui::Checkbox("Loop", &m_scene->m_loopEnabled);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(80.0f);
        ImGui::DragInt("End##seq", &m_scene->m_endFrame, 1, m_scene->m_startFrame + 1, 9999);
        ImGui::SameLine();
        ImGui::Text("Frame: %d", m_scene->m_currentFrame);

        ImGui::Separator();

        // ── NeoSequencer ─────────────────────────────────────────────────────
        ImGuiNeoSequencerFlags seqFlags =
            ImGuiNeoSequencerFlags_EnableSelection |
            ImGuiNeoSequencerFlags_Selection_EnableDragging;

        if (ImGui::BeginNeoSequencer("ParticleEffect",
            &m_scene->m_currentFrame,
            &m_scene->m_startFrame,
            &m_scene->m_endFrame,
            ImVec2(0, 0),
            seqFlags))
        {
            for (auto& emitter : m_scene->m_rootEmitters)
                DrawEmitterTimeline(emitter.get());

            ImGui::EndNeoSequencer();
        }

        ImGui::End();
    }

    void ParticleSequencerWindow::DrawEmitterTimeline(EmitterBase* emitter)
    {
        if (ImGui::BeginNeoTimelineEx(emitter->m_name.c_str(), &emitter->m_open))
        {
            ImGui::NeoKeyframe(&emitter->m_startFrame);
            ImGui::NeoKeyframe(&emitter->m_endFrame);
            ImGui::EndNeoTimeLine();
        }

        for (auto& child : emitter->m_children)
            DrawEmitterTimeline(child.get());
    }

    void ParticleSequencerWindow::DoExport()
    {
        if (ImGuiFileDialog::Instance()->Display("PE_Export", 32, ImVec2(400, 250)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && m_scene)
                SaveParticleEffect(ImGuiFileDialog::Instance()->GetFilePathName(),
                                   m_scene->m_rootEmitters);
            ImGuiFileDialog::Instance()->Close();
        }
    }

    void ParticleSequencerWindow::DoImport()
    {
        if (ImGuiFileDialog::Instance()->Display("PE_Import", 32, ImVec2(400, 250)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && m_scene)
            {
                auto loaded = LoadParticleEffectFromFile(
                    ImGuiFileDialog::Instance()->GetFilePathName());
                for (auto& e : loaded)
                    m_scene->m_rootEmitters.push_back(std::move(e));
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }
}
