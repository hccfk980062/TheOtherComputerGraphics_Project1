#pragma once
#include <imgui.h>
#include "Scene/ParticleEditorScene.h"

namespace CG
{
    // Displays the particle editor's FBO and handles camera input.
    // Follows the same one-frame-delay FBO size sync pattern as ViewportWindow.
    class ParticleViewportWindow
    {
    public:
        ParticleViewportWindow();
        ~ParticleViewportWindow();

        bool Initialize();

        // Syncs the FBO to the viewport size recorded last frame (call before scene->Render())
        void SyncFBO();

        // Displays FBO texture and handles camera mouse/keyboard input
        void UpdateScreen();

        void SetScene(ParticleEditorScene* scene) { m_scene = scene; }

    private:
        ParticleEditorScene* m_scene = nullptr;

        int   m_lastWidth  = 0;
        int   m_lastHeight = 0;

        bool  m_rightMouseDown = false;
        float m_lastMouseX     = 0.0f;
        float m_lastMouseY     = 0.0f;
    };
}
