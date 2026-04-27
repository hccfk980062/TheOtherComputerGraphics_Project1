#include <iostream>

#include "MainScene.h"

namespace CG
{
    MainScene::MainScene() {}
    MainScene::~MainScene() {}

    auto MainScene::Initialize() -> bool
    {
        rootObject.id = 0;

        // 攝影機初始位置在 X=-3，面朝 +X 軸方向
        freeViewCamera = Camera(glm::vec3(-3, 0, 0));
        freeViewCamera.configureLookAt(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));

        // ── 載入 Gundam 各部位模型（OBJ 格式，含貼圖）─────────────────────────
        //model_Gundam[0]  = std::make_unique<Model>("objModels/Bot/back.fbx",       false, false);
        model_Gundam[1]  = std::make_unique<Model>("objModels/Bot/body.obj",       false, true);
        model_Gundam[2]  = std::make_unique<Model>("objModels/Bot/dbody.obj",      false, true);
        model_Gundam[3]  = std::make_unique<Model>("objModels/Bot/dlefthand.obj",  false, true);
        model_Gundam[4]  = std::make_unique<Model>("objModels/Bot/dleftleg.obj",   false, true);
        model_Gundam[5]  = std::make_unique<Model>("objModels/Bot/drighthand.obj", false, true);
        model_Gundam[6]  = std::make_unique<Model>("objModels/Bot/drightleg.obj",  false, true);
        model_Gundam[7]  = std::make_unique<Model>("objModels/Bot/head.obj",       false, true);
        model_Gundam[8]  = std::make_unique<Model>("objModels/Bot/leftfoot.obj",   false, true);
        model_Gundam[9]  = std::make_unique<Model>("objModels/Bot/lefthand.obj",   false, true);
        model_Gundam[10] = std::make_unique<Model>("objModels/Bot/lshouder.obj",   false, true);
        model_Gundam[11] = std::make_unique<Model>("objModels/Bot/rightfoot.obj",  false, true);
        model_Gundam[12] = std::make_unique<Model>("objModels/Bot/righthand.obj",  false, true);
        model_Gundam[13] = std::make_unique<Model>("objModels/Bot/rshouder.obj",   false, true);
        model_Gundam[14] = std::make_unique<Model>("objModels/Bot/ulefthand.obj",  false, true);
        model_Gundam[15] = std::make_unique<Model>("objModels/Bot/uleftleg.obj",   false, true);
        model_Gundam[16] = std::make_unique<Model>("objModels/Bot/urighthand.obj", false, true);
        model_Gundam[17] = std::make_unique<Model>("objModels/Bot/urightleg.obj",  false, true);

        model_photonBlade = std::make_unique<Model>("objModels/PhotonBlade/untitled.fbx", false, true);

        // ── 建立 5 個 Gundam 實體，各部位偏移值為 T-pose 的相對位置 ────────────
        for (int i = 0; i < 5; i++)
        {
            std::string gundamSerialNum = "Gundam_" + std::to_string(i);

            //SetupSceneObject(model_Gundam[0].get(),  gundamSerialNum, "Back",        glm::vec3(0, -1, -3.6));
            SetupSceneObject(model_Gundam[1].get(),  gundamSerialNum, "Body",        glm::vec3(0, 2.5, 0));
            SetupSceneObject(model_Gundam[2].get(),  gundamSerialNum, "DBody",       glm::vec3(0, 0, 0));
            SetupSceneObject(model_Gundam[3].get(),  gundamSerialNum, "DLeftHand",   glm::vec3(3, 1.2, 0));
            SetupSceneObject(model_Gundam[4].get(),  gundamSerialNum, "DLeftLeg",    glm::vec3(1.4, -4.5, 0.3));
            SetupSceneObject(model_Gundam[5].get(),  gundamSerialNum, "DRightHand",  glm::vec3(-3, 1.2, 0));
            SetupSceneObject(model_Gundam[6].get(),  gundamSerialNum, "DRightLeg",   glm::vec3(-1.4, -4.5, 0.3));
            SetupSceneObject(model_Gundam[7].get(),  gundamSerialNum, "Head",        glm::vec3(0, 4.4, 0.4));
            SetupSceneObject(model_Gundam[8].get(),  gundamSerialNum, "LeftFoot",    glm::vec3(1.4, -7.8, 0));
            SetupSceneObject(model_Gundam[9].get(),  gundamSerialNum, "LeftHand",    glm::vec3(4.3, -0.8, 0.2));
            SetupSceneObject(model_Gundam[10].get(), gundamSerialNum, "LeftShouder", glm::vec3(1.6, 3, -0.1));
            SetupSceneObject(model_Gundam[11].get(), gundamSerialNum, "RightFoot",   glm::vec3(-1.4, -7.8, 0));
            SetupSceneObject(model_Gundam[12].get(), gundamSerialNum, "RightHand",   glm::vec3(-4.3, -0.8, 0.2));
            SetupSceneObject(model_Gundam[13].get(), gundamSerialNum, "RightShouder",glm::vec3(-1.6, 3, -0.1));
            SetupSceneObject(model_Gundam[14].get(), gundamSerialNum, "ULeftHand",   glm::vec3(1.6, 3, -0.1));
            SetupSceneObject(model_Gundam[15].get(), gundamSerialNum, "ULeftLeg",    glm::vec3(1.4, -1, 0.3));
            SetupSceneObject(model_Gundam[16].get(), gundamSerialNum, "URightHand",  glm::vec3(-1.6, 3, -0.1));
            SetupSceneObject(model_Gundam[17].get(), gundamSerialNum, "URightLeg",   glm::vec3(-1.4, -1, 0.3));

            // ── 建立骨骼層級（腿部鏈）────────────────────────────────────────
            // 右腿：RightFoot → DRightLeg → URightLeg
            ReparentObject(FindObjectByName(gundamSerialNum + "_RightFoot"), FindObjectByName(gundamSerialNum + "_DRightLeg"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_DRightLeg"), FindObjectByName(gundamSerialNum + "_URightLeg"));

            // 左腿：LeftFoot → DLeftLeg → ULeftLeg
            ReparentObject(FindObjectByName(gundamSerialNum + "_LeftFoot"), FindObjectByName(gundamSerialNum + "_DLeftLeg"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_DLeftLeg"), FindObjectByName(gundamSerialNum + "_ULeftLeg"));

            // 右臂：RightHand → DRightHand → URightHand → RightShouder
            ReparentObject(FindObjectByName(gundamSerialNum + "_RightHand"),  FindObjectByName(gundamSerialNum + "_DRightHand"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_DRightHand"), FindObjectByName(gundamSerialNum + "_URightHand"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_URightHand"), FindObjectByName(gundamSerialNum + "_RightShouder"));

            // 左臂：LeftHand → DLeftHand → ULeftHand → LeftShouder
            ReparentObject(FindObjectByName(gundamSerialNum + "_LeftHand"),  FindObjectByName(gundamSerialNum + "_DLeftHand"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_DLeftHand"), FindObjectByName(gundamSerialNum + "_ULeftHand"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_ULeftHand"), FindObjectByName(gundamSerialNum + "_LeftShouder"));

            // ── 組裝軀幹 ─────────────────────────────────────────────────────
            ReparentObject(FindObjectByName(gundamSerialNum + "_RightShouder"), FindObjectByName(gundamSerialNum + "_Body"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_LeftShouder"),  FindObjectByName(gundamSerialNum + "_Body"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_DBody"),        FindObjectByName(gundamSerialNum + "_Body"));
            //ReparentObject(FindObjectByName(gundamSerialNum + "_Back"),       FindObjectByName(gundamSerialNum + "_Body"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_Head"),         FindObjectByName(gundamSerialNum + "_Body"));

            // 腿接至下半身
            ReparentObject(FindObjectByName(gundamSerialNum + "_URightLeg"), FindObjectByName(gundamSerialNum + "_DBody"));
            ReparentObject(FindObjectByName(gundamSerialNum + "_ULeftLeg"),  FindObjectByName(gundamSerialNum + "_DBody"));
        }

        // ── 光子刀設定 ────────────────────────────────────────────────────────
        SetupSceneObject(model_photonBlade.get(), "PhotonBlade", "PhotonBlade");
        photonBladeTrail.color    = glm::vec3(1.0f, 0.0f, 0.0f);  // 紅色拖尾（對應自發光色）
        photonBladeTrail.duration = 0.3f;

        InitIKChains();

        return true;
    }

    // 為每個 Gundam（索引 0~3）建立四條 IK 鏈：右臂、左臂、右腿、左腿
    void MainScene::InitIKChains()
    {
        ikChains.clear();

        // 輔助 lambda：依名稱串接關節，若任一節點不存在則跳過整條鏈
        auto MakeChain = [&](const std::string& chainName,
                             const std::vector<std::string>& names) -> bool
        {
            IKChain chain;
            chain.name = chainName;
            for (const auto& n : names)
            {
                SceneObject* obj = FindObjectByName(n);
                if (!obj) { return false; }
                chain.joints.push_back(obj);
            }
            // 以當前 T-pose 計算骨骼長度，並將目標初始化到末端效應器世界座標
            chain.ComputeBoneLengths();
            chain.target = glm::vec3(chain.joints.back()->GetWorldMatrix()[3]);
            ikChains.push_back(std::move(chain));
            return true;
        };

        for (int i = 0; i < 4; i++)
        {
            std::string g = "Gundam_" + std::to_string(i) + "_";

            // 右臂：RightShouder → URightHand → DRightHand → RightHand
            MakeChain("Gundam_" + std::to_string(i) + " R_Arm",
            {
                g + "RightShouder",
                g + "URightHand",
                g + "DRightHand",
                g + "RightHand"
            });

            // 左臂：LeftShouder → ULeftHand → DLeftHand → LeftHand
            MakeChain("Gundam_" + std::to_string(i) + " L_Arm",
            {
                g + "LeftShouder",
                g + "ULeftHand",
                g + "DLeftHand",
                g + "LeftHand"
            });

            // 右腿：URightLeg → DRightLeg → RightFoot
            MakeChain("Gundam_" + std::to_string(i) + " R_Leg",
            {
                g + "URightLeg",
                g + "DRightLeg",
                g + "RightFoot"
            });

            // 左腿：ULeftLeg → DLeftLeg → LeftFoot
            MakeChain("Gundam_" + std::to_string(i) + " L_Leg",
            {
                g + "ULeftLeg",
                g + "DLeftLeg",
                g + "LeftFoot"
            });
        }
    }

    // 驅動所有啟用中的 IK 鏈進行 FABRIK 解算（由 App::FixedUpdate 每 60Hz 呼叫）
    void MainScene::SolveIK()
    {
        for (auto& chain : ikChains)
            IKSolver::Solve(chain);
    }

    // 依 animationGroupName 篩選並回傳所有符合的 SceneObject
    std::vector<SceneObject*> MainScene::GetObjectsInAnimationGroup(std::string groupName)
    {
        std::vector<SceneObject*> result;
        for (auto& obj : ObjectList)
        {
            if (obj->animationGroupName == groupName)
                result.push_back(obj);
        }
        return result;
    }

    // 線性搜尋 ObjectList 以名稱查找物件
    SceneObject* MainScene::FindObjectByName(std::string objectName)
    {
        for (auto* obj : ObjectList)
            if (obj->objectName == objectName)
                return obj;
        return nullptr;
    }

    // 線性搜尋 ObjectList 以 ID 查找物件（用於顏色拾取解碼）
    SceneObject* MainScene::FindObjectById(uint32_t id)
    {
        for (auto* obj : ObjectList)
            if (obj->id == id)
                return obj;
        return nullptr;
    }

    // 顏色拾取 Pass：以物件 ID 編碼顏色，對整個場景樹進行渲染
    void MainScene::RenderObjectsForPicking(Shader* pickingShader)
    {
        pickingShader->use();
        pickingShader->setUnifMat4("view",       freeViewCamera.GetViewMatrix());
        pickingShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());
        RenderObjectForPickingRecursive(&rootObject, pickingShader);
    }

    // 遞迴走訪場景樹，為每個有模型的物件設定 objectID uniform 並繪製
    void MainScene::RenderObjectForPickingRecursive(SceneObject* obj, Shader* shader)
    {
        if (obj->model != nullptr)
        {
            shader->setUnifInt("objectID", static_cast<int>(obj->id));
            obj->model->DrawInstanced(*shader, { obj->GetWorldMatrix() });
        }
        for (auto& child : obj->children)
            RenderObjectForPickingRecursive(child.get(), shader);
    }

    // 重設父節點：保持物件世界座標不變
    // 步驟：記錄舊世界矩陣 → 從舊父節點摘除 → 掛入新父節點 → 分解新 local matrix
    void MainScene::ReparentObject(SceneObject* obj, SceneObject* newParent)
    {
        // 先記錄當前世界矩陣，之後用於反推新的 local transform
        glm::mat4 oldWorldMatrix = obj->GetWorldMatrix();

        // 從舊父節點的 children 中找出並取出此節點
        SceneObject* oldParent = obj->parent ? obj->parent : &rootObject;
        auto& siblings = oldParent->children;
        auto it = std::find_if(siblings.begin(), siblings.end(),
            [obj](const auto& c) { return c.get() == obj; });
        if (it == siblings.end()) return;

        auto node    = std::move(*it);
        siblings.erase(it);
        node->parent = newParent;
        newParent->children.push_back(std::move(node));

        // 用新父節點的世界矩陣反推 local matrix，保持世界位置不變
        glm::mat4 newParentWorld = newParent->GetWorldMatrix();
        glm::mat4 newLocalMatrix = glm::inverse(newParentWorld) * oldWorldMatrix;

        // 將 mat4 分解回 Translation / Scale / Rotation 並寫入 local transform
        obj->transform.position = glm::vec3(newLocalMatrix[3]);

        // 各軸向量長度即為縮放值
        glm::vec3 newScale;
        newScale.x = glm::length(glm::vec3(newLocalMatrix[0]));
        newScale.y = glm::length(glm::vec3(newLocalMatrix[1]));
        newScale.z = glm::length(glm::vec3(newLocalMatrix[2]));
        obj->transform.scale = newScale;

        // 移除縮放影響後提取旋轉矩陣，轉換為四元數
        glm::mat3 rotMat(
            glm::vec3(newLocalMatrix[0]) / newScale.x,
            glm::vec3(newLocalMatrix[1]) / newScale.y,
            glm::vec3(newLocalMatrix[2]) / newScale.z
        );
        obj->transform.rotation = glm::quat_cast(rotMat);

        obj->MarkDirty();
    }

    // 一般場景渲染：收集各 Model 的世界矩陣並以 Instanced Rendering 批次繪製
    void MainScene::RenderObjects(Shader* worldObjectShader)
    {
        worldObjectShader->use();

        // view / projection 只需上傳一次，所有 mesh 共用
        worldObjectShader->setUnifMat4("view",       freeViewCamera.GetViewMatrix());
        worldObjectShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

        // 遍歷場景樹，依 Model* 分組收集 world matrix（相同 Model 合併為一個 Draw Call）
        std::unordered_map<Model*, std::vector<glm::mat4>> instanceMap;
        CollectInstances(&rootObject, instanceMap);

        // 每個不同的 Model 發出一次 glDrawElementsInstanced
        for (auto& [model, matrices] : instanceMap)
            model->DrawInstanced(*worldObjectShader, matrices);
    }

    // 刀光拖尾渲染：每幀採樣光子刀刃兩端世界座標，更新拖尾並繪製
    void MainScene::RenderTrails(Shader* trailShader)
    {
        trailShader->use();
        trailShader->setUnifMat4("view",       freeViewCamera.GetViewMatrix());
        trailShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

        float currentTime = (float)glfwGetTime();

        // 取光子刀物件的模型矩陣，將刀刃兩端 local 座標轉換至世界空間
        glm::vec3 worldEdgeA = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(10.0f, -0.2f, 0.0f, 1.0f));
        glm::vec3 worldEdgeB = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(0.5f,  -0.2f, 0.0f, 1.0f));
        photonBladeTrail.update(worldEdgeA, worldEdgeB, currentTime);
        photonBladeTrail.Draw(trailShader);
    }

    // 粒子特效渲染：在光子刀刃隨機位置持續噴出電漿粒子
    void MainScene::RenderParticles(Shader* particleShader)
    {
        particleShader->use();
        particleShader->setUnifMat4("view",       freeViewCamera.GetViewMatrix());
        particleShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

        float currentTime = (float)glfwGetTime();
        float dt          = currentTime - lastTime;
        lastTime = currentTime;

        // 在刀刃兩端之間隨機選取發射位置
        glm::vec3 worldEdgeA = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(0.5f,  -0.2f, 0.0f, 1.0f));
        glm::vec3 worldEdgeB = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(10.0f, -0.2f, 0.0f, 1.0f));

        glm::vec3 worldPos = glm::mix(worldEdgeA, worldEdgeB, (rand() % 100) / 100.f);

        photonBladePlasma.emit(worldPos, 3);
        photonBladePlasma.update(dt);
        photonBladePlasma.Draw();
    }

    // 遞迴走訪場景樹，依 Model* 指標將世界矩陣分組（Instanced Rendering 前置收集）
    void MainScene::CollectInstances(SceneObject* obj, std::unordered_map<Model*, std::vector<glm::mat4>>& outMap)
    {
        if (obj->model != nullptr)
            outMap[obj->model].push_back(obj->GetWorldMatrix());

        for (auto& child : obj->children)
            CollectInstances(child.get(), outMap);
    }
}
