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
        void DisplayTransformPanel();  // 顯示並編輯選取物件的 Transform（位移/旋轉/縮放）
        void DisplayCameraPanel();     // 顯示並編輯自由攝影機參數
        void DisplayIKPanel();         // 顯示並編輯所有 IK 鏈的設定與狀態
    };
}
