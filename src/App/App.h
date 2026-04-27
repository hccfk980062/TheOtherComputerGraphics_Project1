#pragma once
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Window/InspectorWindow/InspectorWindow.h"
#include "Window/ViewportWindow/ViewportWindow.h"
#include "Window/HierarchyWindow/HierarchyWindow.h"
#include "Window/SequencerWindow/SequencerWindow.h"

#include "Scene/SceneRenderer.h"
#include "Scene/MainScene.h"

#include "Command/CommandStack.h"
#include "Event/EventBus.h"

namespace CG
{
    // 應用程式主類別：持有所有子系統與 UI 視窗，驅動「初始化 → 主迴圈 → 清理」流程
    class App
    {
    public:
        App();
        ~App();

        auto Initialize() -> bool;  // 初始化 GLFW / GLEW / ImGui 及所有子系統
        void Loop();                // 主迴圈：輪詢事件 → 固定更新 → 渲染 → ImGui 繪製
        void Terminate();           // 釋放所有 GPU 資源並關閉 GLFW 視窗

    private:
        void FixedUpdate(double fixedDt);   // 固定 60 Hz 邏輯更新（目前用於 IK 解算）
        void ProcessShortcuts();             // 全域快捷鍵：Ctrl+Z 復原 / Ctrl+Y 重做
        void BeginDockspace();               // 建立覆蓋全螢幕的 ImGui DockSpace 容器

    private:
        GLFWwindow* mainWindow;  // GLFW 主視窗控制代碼

        std::unique_ptr<InspectorWindow> inspectorWindow;  // 屬性面板（Transform / Camera / IK）
        std::unique_ptr<ViewportWindow>  viewportWindow;   // 3D 視口（含 Gizmo 操控）
        std::unique_ptr<HierarchyWindow> hierarchyWindow;  // 場景層級樹
        std::unique_ptr<SequencerWindow> sequencerWindow;  // 關鍵幀動畫序列器

        std::unique_ptr<SceneRenderer>   sceneRenderer;   // 負責渲染到離屏 FBO
        std::unique_ptr<MainScene>       mainScene;        // 場景資料（物件樹、模型、IK）

        CommandStack commandStack;  // 可撤銷 / 重做的命令堆疊（Undo / Redo）
        EventBus     eventBus;      // 跨視窗事件匯流排（物件選取事件等）

        double timeNow       = 0;
        double timeLast      = 0;
        double timeDelta     = 0;
        double m_accumulator = 0.0;  // 固定更新的時間累積量，達到 FIXED_DT 才觸發更新

        static constexpr double FIXED_DT = 1.0 / 60.0;  // 固定步距 ≈ 16.67 ms (60 Hz)

    public:
        MainScene* GetMainScene() const { return mainScene.get(); }
    };
}
