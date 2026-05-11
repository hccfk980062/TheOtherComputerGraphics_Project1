#include "Window/ParticleEditorWindow/ParticleViewportWindow.h"

namespace CG
{
    void ParticleViewportWindow::Display(ParticleEditorScene* scene)
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        int w = (int)size.x;
        int h = (int)size.y;

        if (w <= 0 || h <= 0)
        {
            ImGui::TextDisabled("(Viewport too small)");
            return;
        }

        // 同步 FBO 尺寸 → 更新場景 → 渲染到 FBO
        SyncFBOSize(scene, w, h);
        scene->Update();
        scene->Render();

        // 以 ImGui::Image 顯示 FBO 色彩貼圖（Y 軸翻轉：OpenGL 原點在左下）
        Framebuffer* fbo = scene->GetFramebuffer();
        ImGui::Image(
            (ImTextureID)(intptr_t)fbo->colorTexture,
            size,
            ImVec2(0, 1), ImVec2(1, 0));

        HandleCameraInput(scene);
    }

    void ParticleViewportWindow::SyncFBOSize(ParticleEditorScene* scene, int w, int h)
    {
        Framebuffer* fbo = scene->GetFramebuffer();
        if (fbo->width != w || fbo->height != h)
        {
            fbo->ResizeFramebuffer(w, h);
            scene->camera.SetProjectionMatrix(w, h);
        }
    }

    void ParticleViewportWindow::HandleCameraInput(ParticleEditorScene* scene)
    {
        if (!ImGui::IsItemHovered()) return;

        ImGuiIO& io = ImGui::GetIO();

        // 右鍵拖曳：旋轉攝影機
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            if (!m_rightMouseDown)
            {
                m_rightMouseDown = true;
                m_lastMouseX = io.MousePos.x;
                m_lastMouseY = io.MousePos.y;
            }
            else
            {
                float dx = io.MousePos.x - m_lastMouseX;
                float dy = io.MousePos.y - m_lastMouseY;
                m_lastMouseX = io.MousePos.x;
                m_lastMouseY = io.MousePos.y;
                scene->camera.ProcessMouseMovement(dx, -dy);

                std::array<bool, 6> keys = {
                   ImGui::IsKeyDown(ImGuiKey_W), ImGui::IsKeyDown(ImGuiKey_S),
                   ImGui::IsKeyDown(ImGuiKey_A), ImGui::IsKeyDown(ImGuiKey_D),
                   ImGui::IsKeyDown(ImGuiKey_Q), ImGui::IsKeyDown(ImGuiKey_E)
                };
                scene->camera.ProcessKeyboard(keys, 0.05);
            }
        }
        else
        {
            m_rightMouseDown = false;
        }

        // 滾輪：前後縮放
        if (io.MouseWheel != 0.0f)
        {
            glm::vec3 dir = scene->camera.Front * io.MouseWheel * 0.5f;
            scene->camera.Position += dir;
        }
    }

} // namespace CG
