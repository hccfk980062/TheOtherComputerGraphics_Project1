#pragma once
#include <array>
#include <string>
#include <map>
#include<stack>
#include <vector>
#include <memory>
#include <unordered_map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Model/ModelLoader.h"
#include "Scene/Transform.h"

#include "ParticleEffects/ParticleRenderer.h"
#include "ParticleEffects/TrailRenderer.h"

#include "Camera/Camera.h"
#include "IK/IKSolver.h"

namespace CG
{
    // 場景資料中心：管理所有 SceneObject、模型、IK 鏈、特效與相機
    class MainScene
    {
    public:
        MainScene();
        ~MainScene();

        auto Initialize() -> bool;  // 載入模型、建立場景樹、初始化 IK 鏈

        // ── 渲染介面（由 SceneRenderer 呼叫）────────────────────────────────
        void RenderObjects(Shader* worldObjectShader);           // 渲染所有 SceneObject（Instanced）
        void RenderParticles(Shader* particleShader);            // 渲染光子刀粒子效果
        void RenderTrails(Shader* trailShader);                  // 渲染光子刀拖尾色帶
        void RenderObjectsForPicking(Shader* pickingShader);     // 顏色拾取 Pass（以 ID 編碼顏色）

        Camera freeViewCamera;  // 自由飛行攝影機（WASD + 滑鼠右鍵）

        // ── 物件管理 ─────────────────────────────────────────────────────────
        std::vector<SceneObject*> ObjectList;  // 所有 SceneObject 的扁平列表（快速查找用）

        SceneObject  rootObject;              // 場景樹的虛擬根節點（不含模型）
        SceneObject* selectedObject = nullptr;  // 目前被選取的物件
        int objectCount = 0;  // 物件 ID 計數器（每次 SetupSceneObject 遞增）

        // ── 光子刀特效 ───────────────────────────────────────────────────────
        TrailRenderer    photonBladeTrail;                          // 刀光拖尾
        ParticleRenderer photonBladePlasma = ParticleRenderer(16); // 電漿粒子效果

        // ── 逆向動力學 ───────────────────────────────────────────────────────
        std::vector<IKChain> ikChains;  // 場景中所有 IK 鏈（每幀由 App::FixedUpdate 解算）
        void SolveIK();                 // 驅動所有啟用中的 IK 鏈進行 FABRIK 解算

        // ── 查找輔助函式 ─────────────────────────────────────────────────────
        SceneObject* FindObjectByName(std::string objectName);
        SceneObject* FindObjectById(uint32_t id);
        std::vector<SceneObject*> GetObjectsInAnimationGroup(std::string groupName);

        // 重設父節點，同時保持物件世界座標不變（分解並回寫 local transform）
        void ReparentObject(SceneObject* obj, SceneObject* newParent);

    private:
        void InitIKChains();  // 為場景中的 Gundam 初始化四肢 IK 鏈

        // 遞迴執行顏色拾取渲染（深度優先走訪場景樹）
        void RenderObjectForPickingRecursive(SceneObject* obj, Shader* shader);

        // 鋼彈各部位模型（index 0 預留，1~17 對應各零件）
        std::unique_ptr<Model> model_Gundam[18];
        std::unique_ptr<Model> model_photonBlade;

        float lastTime = (float)glfwGetTime();  // 上幀時間，用於粒子 dt 計算

        // 遞迴收集場景樹中所有物件的世界矩陣，依 Model* 分組以支援 Instanced Rendering
        void CollectInstances(SceneObject* obj, std::unordered_map<Model*, std::vector<glm::mat4>>& outMap);

        // 建立並加入一個 SceneObject 至場景樹根節點
        void SetupSceneObject(Model* model, std::string modelName, std::string animationSerializedName, glm::vec3 objectPosition = glm::vec3(0))
        {
            auto objUniquePtr = std::make_unique<SceneObject>();
            objUniquePtr->id                         = objectCount++;
            objUniquePtr->objectName                 = (modelName + "_" + animationSerializedName);
            objUniquePtr->animationGroupName         = modelName;
            objUniquePtr->animationSerializedName    = animationSerializedName;
            objUniquePtr->model                      = model;
            objUniquePtr->objectType                 = 1;
            objUniquePtr->transform.position         = objectPosition;

            ObjectList.push_back(objUniquePtr.get());
            rootObject.children.push_back(std::move(objUniquePtr));
        }
    };
}
