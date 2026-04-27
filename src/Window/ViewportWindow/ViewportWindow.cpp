#include "Window/ViewportWindow/ViewportWindow.h"

namespace CG
{
    ViewportWindow::ViewportWindow()
        : m_lastViewportWidth(0)
        , m_lastViewportHeight(0)
        , m_rightMouseDown(false)
        , m_lastMouseX(0.0f)
        , m_lastMouseY(0.0f)
    {
    }

    ViewportWindow::~ViewportWindow() {}

    auto ViewportWindow::Initialize() -> bool { return true; }

    // 將 FBO 大小同步至上一幀記錄的視口尺寸
    // 必須在 RenderScene 之前呼叫，確保 FBO 與視窗一致（延遲一幀避免中途 resize）
    void ViewportWindow::SyncFramebufferSize(Framebuffer* framebuffer)
    {
        if (m_lastViewportWidth > 0 && m_lastViewportHeight > 0)
        {
            if (m_lastViewportWidth  != framebuffer->width ||
                m_lastViewportHeight != framebuffer->height)
            {
                framebuffer->ResizeFramebuffer(m_lastViewportWidth, m_lastViewportHeight);
            }
        }
    }

    void ViewportWindow::UpdateScreen(MainScene* scene, Framebuffer* framebuffer)
    {
        // 移除視窗內距，讓 FBO 貼圖填滿整個視窗
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport (ImGuizmo NMSL Edition)", nullptr,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();

        // ── 記錄視口尺寸（供下幀 SyncFramebufferSize 使用）─────────────────
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
        if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;
        m_lastViewportWidth  = (int)viewportSize.x;
        m_lastViewportHeight = (int)viewportSize.y;

        ImVec2 imageScreenPos = ImGui::GetCursorScreenPos();

        // ── 將 FBO 顏色貼圖以 ImGui DrawList 貼上（翻轉 V 軸：OpenGL UV 原點在左下）
        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)(intptr_t)framebuffer->colorTexture,
            imageScreenPos,
            ImVec2(imageScreenPos.x + viewportSize.x, imageScreenPos.y + viewportSize.y),
            ImVec2(0, 1), ImVec2(1, 0));  // UV 翻轉：v=1 → v=0

        // ── 攝影機滑鼠輸入（右鍵拖曳旋轉 + WASDQE 飛行）────────────────────
        if (ImGui::IsWindowHovered())
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseDown[ImGuiMouseButton_Right])
            {
                float dx =  io.MouseDelta.x;
                float dy = -io.MouseDelta.y;  // Y 軸翻轉：螢幕向下 = 攝影機向上仰
                if (dx != 0.0f || dy != 0.0f)
                    scene->freeViewCamera.ProcessMouseMovement(dx, dy);

                std::array<bool, 6> keys = {
                    ImGui::IsKeyDown(ImGuiKey_W), ImGui::IsKeyDown(ImGuiKey_S),
                    ImGui::IsKeyDown(ImGuiKey_A), ImGui::IsKeyDown(ImGuiKey_D),
                    ImGui::IsKeyDown(ImGuiKey_Q), ImGui::IsKeyDown(ImGuiKey_E)
                };
                scene->freeViewCamera.ProcessKeyboard(keys, 0.05);
            }
        }

        // ── 工具列：變換操作模式（T / R / S）與座標空間（Local / World）──────
        {
            constexpr float PAD       = 8.0f;
            constexpr float BTN_W     = 120.0f;
            constexpr float BTN_H     = 28.0f;
            const ImVec4    COL_ACT   = { 0.26f, 0.59f, 0.98f, 1.00f };  // 啟用中：藍色
            const ImVec4    COL_NORM  = { 0.20f, 0.20f, 0.20f, 0.75f };  // 未啟用：深灰
            const ImVec4    COL_HOVER = { 0.35f, 0.35f, 0.35f, 0.90f };  // Hover：灰

            ImGui::SetCursorScreenPos(ImVec2(imageScreenPos.x + PAD, imageScreenPos.y + PAD));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(4.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

            struct OpBtn { const char* label; ImGuizmo::OPERATION op; };
            constexpr OpBtn OPS[] = {
                { "[T]ranslate", ImGuizmo::TRANSLATE },
                { "[R]otate",    ImGuizmo::ROTATE    },
                { "[S]cale",     ImGuizmo::SCALE     },
            };
            for (const auto& btn : OPS)
            {
                bool active = (m_transformOp == btn.op);
                ImGui::PushStyleColor(ImGuiCol_Button,        active ? COL_ACT : COL_NORM);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? COL_ACT : COL_HOVER);
                if (ImGui::Button(btn.label, ImVec2(BTN_W, BTN_H)))
                    m_transformOp = btn.op;
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
            }

            struct ModeBtn { const char* label; ImGuizmo::MODE mode; };
            constexpr ModeBtn MODES[] = {
                { "[L]ocal", ImGuizmo::LOCAL },
                { "[W]orld", ImGuizmo::WORLD },
            };
            for (const auto& btn : MODES)
            {
                bool active = (m_transformMode == btn.mode);
                ImGui::PushStyleColor(ImGuiCol_Button,        active ? COL_ACT : COL_NORM);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? COL_ACT : COL_HOVER);
                if (ImGui::Button(btn.label, ImVec2(BTN_W, BTN_H)))
                    m_transformMode = btn.mode;
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
            }
            ImGui::PopStyleVar(2);
        }

        // ── 左鍵點擊：顏色拾取，取得點擊位置的 SceneObject ──────────────────
        // IsOver() 為 true 代表滑鼠在 Gizmo 上，此時不觸發拾取以免誤選
        if (ImGui::IsWindowHovered() && !ImGuizmo::IsOver() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_sceneRenderer)
        {
            ImVec2 mouse = ImGui::GetMousePos();
            int pickX = (int)(mouse.x - imageScreenPos.x);
            int pickY = (int)(mouse.y - imageScreenPos.y);

            if (pickX >= 0 && pickY >= 0 &&
                pickX < m_lastViewportWidth && pickY < m_lastViewportHeight)
            {
                SceneObject* picked = m_sceneRenderer->GetObjectAtPixel(scene, pickX, pickY);
                scene->selectedObject = picked;
                if (m_eventBus)
                    m_eventBus->Publish(ObjectSelectedEvent{ picked });
            }
        }

        // ── 設定 ImGuizmo 繪製區域（與 FBO 貼圖對齊）────────────────────────
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(imageScreenPos.x, imageScreenPos.y, viewportSize.x, viewportSize.y);

        glm::mat4 viewMat = scene->freeViewCamera.GetViewMatrix();
        glm::mat4 projMat = scene->freeViewCamera.GetProjectionMatrix();
        float viewFlat[16], projFlat[16];
        memcpy(viewFlat, glm::value_ptr(viewMat), sizeof(viewFlat));
        memcpy(projFlat, glm::value_ptr(projMat), sizeof(projFlat));

        // 無選取物件或場景為空時，不繪製 Gizmo
        if (scene->selectedObject == nullptr || scene->rootObject.children.empty())
        {
            ImGui::End();
            return;
        }

        // ── 根據選取物件決定顯示 IK 目標 Gizmo 或物件 Transform Gizmo ────────
        IKChain* ikChain = FindEndEffectorChain(scene, scene->selectedObject);

        if (ikChain)
        {
            // 選取的物件是某 IK 鏈的末端效應器：顯示 IK 目標點 Gizmo（僅位移、世界空間）
            glm::mat4 targetMatrix = glm::translate(glm::mat4(1.0f), ikChain->target);

            bool nowUsing = false;
            ImGuizmo::Manipulate(viewFlat, projFlat,
                ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
                glm::value_ptr(targetMatrix));
            nowUsing = ImGuizmo::IsUsing();

            // 拖曳開始：記錄起始目標座標（供 Undo 用）
            if (!m_wasUsingIKGizmo && nowUsing)
            {
                m_ikGizmoStartTarget = ikChain->target;
                m_activeIKChain      = ikChain;
            }
            // 拖曳結束：建立命令並壓入 CommandStack
            else if (m_wasUsingIKGizmo && !nowUsing && m_activeIKChain && m_cmdStack)
            {
                auto cmd = std::make_unique<IKTargetMoveCommand>(
                    m_activeIKChain, m_ikGizmoStartTarget, m_activeIKChain->target);
                m_cmdStack->Execute(std::move(cmd));
                m_activeIKChain = nullptr;
            }

            // 拖曳中：直接更新 IK 目標座標（從矩陣第四行取 translation）
            if (nowUsing)
                ikChain->target = glm::vec3(targetMatrix[3]);

            m_wasUsingIKGizmo = nowUsing;
        }
        else
        {
            // 一般物件：顯示 Transform Gizmo（平移/旋轉/縮放）
            glm::mat4 modelMatrix = scene->selectedObject->GetWorldMatrix();

            ImGuizmo::Manipulate(viewFlat, projFlat,
                m_transformOp, m_transformMode, glm::value_ptr(modelMatrix));

            bool nowUsing = ImGuizmo::IsUsing();

            // 拖曳開始：快照目前 transform 以供 Undo
            if (!m_wasUsingGizmo && nowUsing)
                m_gizmoStartTransform = scene->selectedObject->transform;
            // 拖曳結束：建立命令並壓入 CommandStack
            else if (m_wasUsingGizmo && !nowUsing && m_cmdStack)
            {
                auto cmd = std::make_unique<TransformCommand>(
                    scene->selectedObject,
                    m_gizmoStartTransform,
                    scene->selectedObject->transform);
                m_cmdStack->Execute(std::move(cmd));
            }

            // 拖曳中：將 Gizmo 的世界矩陣轉換回物件的 local transform
            if (nowUsing)
            {
                if (scene->selectedObject->parent != nullptr)
                {
                    // 有父節點：需先乘以父節點世界矩陣的逆，轉換至 local 空間
                    glm::mat4 parentWorld = scene->selectedObject->parent->GetWorldMatrix();
                    glm::mat4 newLocalMat = glm::inverse(parentWorld) * modelMatrix;
                    ApplyMatrixToTransform(scene->selectedObject->transform, newLocalMat);
                }
                else
                {
                    // 無父節點：世界矩陣即 local 矩陣
                    ApplyMatrixToTransform(scene->selectedObject->transform, modelMatrix);
                }
                scene->selectedObject->MarkDirty();
            }

            m_wasUsingGizmo = nowUsing;
        }

        ImGui::End();
    }

    // ─── 輔助函式 ─────────────────────────────────────────────────────────────

    // 使用 ImGuizmo::DecomposeMatrixToComponents 分解矩陣（角度單位為度）
    // 旋轉轉換：先轉為弧度再建立四元數，保持與 glm 的慣例一致
    void ViewportWindow::ApplyMatrixToTransform(Transform& transform, const glm::mat4& matrix)
    {
        float t[3], r[3], s[3];
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), t, r, s);
        transform.position = { t[0], t[1], t[2] };
        transform.rotation = glm::quat(glm::radians(glm::vec3(r[0], r[1], r[2])));
        transform.scale    = { s[0], s[1], s[2] };
    }

    // 若選取物件是某個啟用 IK 鏈的末端效應器（joints.back()），回傳該鏈
    // 優先顯示 IK Gizmo，讓使用者直接拖動目標點而非旋轉骨骼
    IKChain* ViewportWindow::FindEndEffectorChain(MainScene* scene, SceneObject* obj)
    {
        for (auto& chain : scene->ikChains)
        {
            if (chain.enabled && !chain.joints.empty() &&
                chain.joints.back() == obj)
            {
                return &chain;
            }
        }
        return nullptr;
    }

} // namespace CG
