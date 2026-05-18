#pragma once

#include <Scene/MainScene.h>

namespace CG
{
    // 場景層級樹視窗：以樹狀結構顯示所有 SceneObject，支援點選選取與拖放重設父節點
    class HierarchyWindow
    {
    public:
        HierarchyWindow();
        ~HierarchyWindow();

        auto Initialize() -> bool;
        void Display();  // 每幀呼叫：繪製整個 Hierarchy 視窗

        void SetTargetScene(MainScene* scene)
        {
            targetScene = scene;
        }

    private:
        MainScene* targetScene = nullptr;

        void DrawNode(SceneObject* obj);           // 遞迴繪製一個節點及其子節點
        void DrawContextMenu(SceneObject* obj);    // 右鍵選單（Create Child / Delete）
        void CreateObject();                       // 建立新空物件（尚未實作）
        void DeleteObject();                       // 刪除物件（尚未實作）

        // 重設父節點（委派至 MainScene::ReparentObject，保持世界座標不變）
        void ReparentObject(SceneObject* obj, SceneObject* newParent);

        // 判斷 potentialAncestor 是否為 node 的祖先節點（防止拖放成循環）
        bool IsAncestor(SceneObject* potentialAncestor, SceneObject* node);
    };
}
