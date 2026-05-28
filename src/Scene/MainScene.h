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

#include "Camera/Camera.h"
#include "IK/IKSolver.h"
#include "Lighting/Light.h"
#include "ParticleEffects/EmitterBase.h"

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
        void RenderObjects(Shader* worldObjectShader);
        void RenderObjects(Shader* worldObjectShader, const glm::mat4& overrideView);
        void RenderObjectsExceptCube(Shader* worldObjectShader);  // 排除 Cube，供環境貼圖 pass 使用
        void RenderCubeOnly(Shader* shader);                      // 僅繪製 model_Cube
        void RenderParticles(Shader* particleShader, Shader* trailShader);
        void RenderObjectsForPicking(Shader* pickingShader);

        // 動態模糊速度 Pass：對每個物件個別設定當前 + 前一幀 Model Matrix，渲染速度向量
        void RenderObjectsForVelocity(Shader* velocityShader,
            const std::unordered_map<uint32_t, glm::mat4>& prevMatrices);

        // ── 粒子特效（從 JSON 匯入，運行於 MainScene 中）────────────────────
        std::vector<std::unique_ptr<EmitterBase>> m_particleEmitters;
        std::vector<SceneObject*>                 m_emitterObjects;   // non-owning refs，供清除時追蹤
        void LoadParticleEffect(const std::string& path);   // 追加載入，同時建立對應 SceneObject
        void ClearParticleEffects();                         // 清除所有特效及其 SceneObject

        Camera    freeViewCamera;  // 自由飛行攝影機（WASD + 滑鼠右鍵）
        LightData light;           // 場景光源設定（Directional 或 Point）

        // ── 物件管理 ─────────────────────────────────────────────────────────
        std::vector<SceneObject*> ObjectList;  // 所有 SceneObject 的扁平列表（快速查找用）

        SceneObject  rootObject;              // 場景樹的虛擬根節點（不含模型）
        SceneObject* selectedObject = nullptr;  // 目前被選取的物件
        int objectCount = 0;  // 物件 ID 計數器（每次 SetupSceneObject 遞增）


        // ── 逆向動力學 ───────────────────────────────────────────────────────
        std::vector<IKChain> ikChains;  // 場景中所有 IK 鏈（每幀由 App::FixedUpdate 解算）
        void SolveIK();                 // 驅動所有啟用中的 IK 鏈進行 FABRIK 解算

        // ── 查找輔助函式 ─────────────────────────────────────────────────────
        SceneObject* FindObjectByName(std::string objectName);
        SceneObject* FindObjectById(uint32_t id);
        std::vector<SceneObject*> GetObjectsInAnimationGroup(std::string groupName);

        // 重設父節點，同時保持物件世界座標不變（分解並回寫 local transform）
        void ReparentObject(SceneObject* obj, SceneObject* newParent);

        // 直接重設父節點（不保持世界座標，載入場景時使用）
        void ReparentObjectDirect(SceneObject* obj, SceneObject* newParent);

        // 清除所有場景物件與狀態，為重新初始化做準備（LoadScene 前必須呼叫）
        void Reset();

    private:
        void InitIKChains();  // 為場景中的 Gundam 初始化四肢 IK 鏈

        // 遞迴執行顏色拾取渲染（深度優先走訪場景樹）
        void RenderObjectForPickingRecursive(SceneObject* obj, Shader* shader);

        // 遞迴渲染速度向量（動態模糊 pass 使用）
        void RenderObjectForVelocityRecursive(SceneObject* obj, Shader* shader,
            const std::unordered_map<uint32_t, glm::mat4>& prevMatrices);

        // 鋼彈各部位模型（index 0 預留，1~17 對應各零件）
        std::unique_ptr<Model> model_Gundam[18];
        std::unique_ptr<Model> model_photonBlade;
        std::unique_ptr<Model> model_Cube;
        std::unique_ptr<Model> model_Sphere;
        float lastTime = (float)glfwGetTime();

        // Particle timeline state
        int   m_particleFrame     = 0;
        float m_particleTimeAccum = 0.0f;

        // 遞迴收集場景樹中所有物件的世界矩陣，依 Model* 分組以支援 Instanced Rendering
        // excludeModel != nullptr 時跳過該 Model（供環境貼圖 pass 排除 Cube 使用）
        void CollectInstances(SceneObject* obj, std::unordered_map<Model*, std::vector<glm::mat4>>& outMap, Model* excludeModel = nullptr);

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
