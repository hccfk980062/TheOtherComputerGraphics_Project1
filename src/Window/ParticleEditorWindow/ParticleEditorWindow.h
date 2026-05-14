#pragma once
#include <memory>

#include "Scene/ParticleEditorScene.h"
#include "Window/ParticleEditorWindow/ParticleHierarchyWindow.h"
#include "Window/ParticleEditorWindow/ParticleInspectorWindow.h"
#include "Window/ParticleEditorWindow/ParticleViewportWindow.h"
#include "Window/ParticleEditorWindow/ParticleSequencerWindow.h"

namespace CG
{
    // Top-level particle editor container.
    // Owns the scene and four sub-windows; App calls Display() each frame
    // when the editor is visible.
    class ParticleEditorWindow
    {
    public:
        ParticleEditorWindow();
        ~ParticleEditorWindow();

        bool Initialize();

        // Drive scene update + render, then display all sub-windows
        void Display();

    private:
        std::unique_ptr<ParticleEditorScene>     m_scene;
        std::unique_ptr<ParticleHierarchyWindow> m_hierarchyWindow;
        std::unique_ptr<ParticleInspectorWindow> m_inspectorWindow;
        std::unique_ptr<ParticleViewportWindow>  m_viewportWindow;
        std::unique_ptr<ParticleSequencerWindow> m_sequencerWindow;
    };
}
