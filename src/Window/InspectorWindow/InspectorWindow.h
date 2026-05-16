#pragma once

#include <Scene/MainScene.h>

namespace CG
{
    // 屬性面板視窗：以分頁顯示 Transform、Camera、IK 等資訊，並允許即時編輯
    class InspectorWindow
    {
    public:
        InspectorWindow();
        ~InspectorWindow();

        auto Initialize() -> bool;
        void Display();  // 每幀呼叫：繪製整個 Inspector 視窗

        void SetTargetScene(MainScene* scene)
        {
            targetScene = scene;
        }

    private:
        MainScene* targetScene = nullptr;

        // ── 分頁面板輔助函式 ─────────────────────────────────────────────────
        void DisplayTransformPanel();
        void DisplayCameraPanel();
        void DisplayIKPanel();
        void DisplayLightingPanel();
        void DisplayParticlesPanel();  // 匯入並管理在 MainScene 中運行的粒子特效
    };
}
