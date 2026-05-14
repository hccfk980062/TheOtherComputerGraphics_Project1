#include <array>
#include <imgui.h>

#include "Window/ParticleEditorWindow/ParticleViewportWindow.h"

namespace CG
{
    ParticleViewportWindow::ParticleViewportWindow()  {}
    ParticleViewportWindow::~ParticleViewportWindow() {}

    bool ParticleViewportWindow::Initialize() { return true; }

    void ParticleViewportWindow::SyncFBO()
    {
        if (!m_scene) return;
        if (m_lastWidth > 0 && m_lastHeight > 0)
            m_scene->ResizeFBO(m_lastWidth, m_lastHeight);
    }

    void ParticleViewportWindow::UpdateScreen()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        bool visible = ImGui::Begin("Particle Viewport");
        ImGui::PopStyleVar();

        if (!visible || !m_scene)
        {
            ImGui::End();
            return;
        }

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
        if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;

        // Record size for next frame's FBO sync
        m_lastWidth  = (int)viewportSize.x;
        m_lastHeight = (int)viewportSize.y;

        Framebuffer* fbo = m_scene->GetFramebuffer();
        if (fbo)
        {
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)(intptr_t)fbo->colorTexture,
                screenPos,
                ImVec2(screenPos.x + viewportSize.x, screenPos.y + viewportSize.y),
                ImVec2(0, 1), ImVec2(1, 0)  // V-flip for OpenGL coordinate system
            );
        }

        // Camera keyboard input (WASDQE when window is focused)
        bool focused = ImGui::IsWindowFocused();
        if (focused)
        {
            std::array<bool, 6> keys = {
                ImGui::IsKeyDown(ImGuiKey_W),
                ImGui::IsKeyDown(ImGuiKey_S),
                ImGui::IsKeyDown(ImGuiKey_A),
                ImGui::IsKeyDown(ImGuiKey_D),
                ImGui::IsKeyDown(ImGuiKey_Q),
                ImGui::IsKeyDown(ImGuiKey_E)
            };
            float dt = ImGui::GetIO().DeltaTime;
            m_scene->m_camera.ProcessKeyboard(keys, dt);
        }

        // Camera mouse rotation (right-mouse drag)
        bool hovered = ImGui::IsWindowHovered();
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered)
        {
            m_rightMouseDown = true;
            ImVec2 mp = ImGui::GetMousePos();
            m_lastMouseX = mp.x;
            m_lastMouseY = mp.y;
        }
        if (m_rightMouseDown)
        {
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                m_rightMouseDown = false;
            }
            else
            {
                ImVec2 mp = ImGui::GetMousePos();
                float dx =  (mp.x - m_lastMouseX);
                float dy = -(mp.y - m_lastMouseY);  // invert Y for look-up
                m_scene->m_camera.ProcessMouseMovement(dx, dy);
                m_lastMouseX = mp.x;
                m_lastMouseY = mp.y;
            }
        }

        ImGui::End();
    }
}
