#include "Window/ParticleEditorWindow/ParticleEditorWindow.h"

namespace CG
{
    ParticleEditorWindow::ParticleEditorWindow()  {}
    ParticleEditorWindow::~ParticleEditorWindow() {}

    bool ParticleEditorWindow::Initialize()
    {
        m_scene = std::make_unique<ParticleEditorScene>();
        if (!m_scene->Initialize(800, 600)) return false;

        m_hierarchyWindow = std::make_unique<ParticleHierarchyWindow>();
        m_hierarchyWindow->Initialize();
        m_hierarchyWindow->SetScene(m_scene.get());

        m_inspectorWindow = std::make_unique<ParticleInspectorWindow>();
        m_inspectorWindow->Initialize();
        m_inspectorWindow->SetScene(m_scene.get());

        m_viewportWindow = std::make_unique<ParticleViewportWindow>();
        m_viewportWindow->Initialize();
        m_viewportWindow->SetScene(m_scene.get());

        m_sequencerWindow = std::make_unique<ParticleSequencerWindow>();
        m_sequencerWindow->Initialize();
        m_sequencerWindow->SetScene(m_scene.get());

        return true;
    }

    void ParticleEditorWindow::Display()
    {
        // 1. Update particle simulation (dt from ImGui::GetTime())
        m_scene->Update();

        // 2. Sync FBO to last-frame viewport size (one-frame delay, same as main ViewportWindow)
        m_viewportWindow->SyncFBO();

        // 3. Render emitters into the FBO
        m_scene->Render();

        // 4. Draw all sub-windows (they dock into the main DockSpace automatically)
        m_hierarchyWindow->Display();
        m_inspectorWindow->Display();
        m_viewportWindow->UpdateScreen();
        m_sequencerWindow->Display();
    }
}
