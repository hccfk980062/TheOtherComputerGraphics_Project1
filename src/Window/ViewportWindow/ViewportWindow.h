#pragma once

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#include "Scene/MainScene.h"
#include "Scene/SceneRenderer.h"
#include "FrameBuffer/Framebuffer.h"
#include "Command/CommandStack.h"
#include "Event/EventBus.h"

namespace CG
{
    // 3D 視口視窗：顯示 FBO 渲染結果、處理攝影機輸入、並整合 ImGuizmo 變換控制器
    class ViewportWindow
    {
    public:
        ViewportWindow();
        ~ViewportWindow();

        auto Initialize() -> bool;

        // 由 App 注入外部依賴（不在建構子中傳入，保持建立順序彈性）
        void SetCommandStack(CommandStack* stack)     { m_cmdStack      = stack;    }
        void SetEventBus(EventBus* bus)               { m_eventBus      = bus;      }
        void SetSceneRenderer(SceneRenderer* renderer){ m_sceneRenderer = renderer; }

        // 同步 FBO 大小至上一幀記錄的視口尺寸（需在 RenderScene 之前呼叫）
        void SyncFramebufferSize(Framebuffer* framebuffer);

        // 每幀更新：顯示 FBO 貼圖、處理滑鼠輸入、繪製 Gizmo
        void UpdateScreen(MainScene* scene, Framebuffer* framebuffer);

    private:
        // ── 視口尺寸追蹤（上幀記錄，下幀同步至 FBO）────────────────────────
        int m_lastViewportWidth  = 0;
        int m_lastViewportHeight = 0;

        // ── 攝影機滑鼠輸入狀態 ───────────────────────────────────────────────
        bool  m_rightMouseDown = false;
        float m_lastMouseX     = 0.0f;
        float m_lastMouseY     = 0.0f;

        // ── Gizmo 操作模式 ───────────────────────────────────────────────────
        ImGuizmo::OPERATION m_transformOp   = ImGuizmo::TRANSLATE;  // T / R / S
        ImGuizmo::MODE      m_transformMode = ImGuizmo::LOCAL;       // Local / World

        // ── 物件 Transform Gizmo 的撤銷追蹤 ─────────────────────────────────
        bool      m_wasUsingGizmo      = false;
        Transform m_gizmoStartTransform;         // 拖曳開始時捕捉的 Transform 快照

        // ── IK 目標 Gizmo 的撤銷追蹤 ────────────────────────────────────────
        bool      m_wasUsingIKGizmo    = false;
        glm::vec3 m_ikGizmoStartTarget = glm::vec3(0.0f);  // 拖曳開始時的目標世界座標
        IKChain*  m_activeIKChain      = nullptr;           // 正在拖曳的 IK 鏈

        // ── 外部依賴（由 App::Initialize 注入）──────────────────────────────
        CommandStack*  m_cmdStack      = nullptr;
        EventBus*      m_eventBus      = nullptr;
        SceneRenderer* m_sceneRenderer = nullptr;

        // ── 輔助函式 ─────────────────────────────────────────────────────────

        // 將 ImGuizmo 分解出的矩陣寫回 Transform（位移 / 旋轉 / 縮放）
        void ApplyMatrixToTransform(Transform& transform, const glm::mat4& matrix);

        // 回傳 obj 是否為某個啟用 IK 鏈的末端效應器；若是則回傳該鏈，否則回傳 nullptr
        IKChain* FindEndEffectorChain(MainScene* scene, SceneObject* obj);
    };
}
