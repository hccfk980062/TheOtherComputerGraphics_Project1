#include <imgui.h>
#include <ImNeoSequencer/imgui_neo_sequencer.h>
#include <ImNeoSequencer/imgui_neo_internal.h>

#include "Window/ParticleEditorWindow/ParticleSequencerWindow.h"

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

        // Recurse into child emitter templates
        for (auto& child : emitter->m_children)
            DrawEmitterTimeline(child.get());
    }
}
