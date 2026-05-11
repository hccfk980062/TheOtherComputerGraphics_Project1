#pragma once
#include <memory>
#include <imgui.h>

#include "Scene/ParticleEditorScene.h"
#include "Window/ParticleEditorWindow/ParticleHierarchyWindow.h"
#include "Window/ParticleEditorWindow/ParticleInspectorWindow.h"
#include "Window/ParticleEditorWindow/ParticleViewportWindow.h"
#include "Window/ParticleEditorWindow/ParticleSequencerWindow.h"

namespace CG
{
    // 粒子特效編輯器的頂層視窗
    // 持有獨立場景（ParticleEditorScene）與四個子視窗：
    //   Hierarchy、Inspector、Viewport（含 FBO）、Sequencer（NeoSequencer）
    class ParticleEditorWindow
    {
    public:
        bool open = true;

        bool Initialize();
        void Display();  // 每幀從 App::Loop() 呼叫

    private:
        std::unique_ptr<ParticleEditorScene> m_scene;

        ParticleHierarchyWindow m_hierarchy;
        ParticleInspectorWindow m_inspector;
        ParticleViewportWindow  m_viewport;
        ParticleSequencerWindow m_sequencer;

        float m_sequencerHeight = 180.0f;
        float m_leftPanelWidth  = 400.0f;
    };

} // namespace CG
