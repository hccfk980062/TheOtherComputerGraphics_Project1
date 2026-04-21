#include <iostream>

#include "MainScene.h"
namespace CG
{
	MainScene::MainScene()
	{
	}
	MainScene::~MainScene()
	{
	}

	auto MainScene::Initialize() -> bool
	{
		rootObject.id = 0;

		freeViewCamera = Camera(glm::vec3(-3, 0, 0));
		freeViewCamera.configureLookAt(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));


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

		model_photonBlade = std::make_unique<Model>("objModels/PhotonBlade/untitled.fbx", false, true);//
		// 初始化場景物件

		for (int i = 0; i < 4; i++)
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

			//R_Leg
			ReparentObject(FindObjectByName(gundamSerialNum + "_RightFoot"), FindObjectByName(gundamSerialNum + "_DRightLeg"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_DRightLeg"), FindObjectByName(gundamSerialNum + "_URightLeg"));

			//L_Leg
			ReparentObject(FindObjectByName(gundamSerialNum + "_LeftFoot"), FindObjectByName(gundamSerialNum + "_DLeftLeg"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_DLeftLeg"), FindObjectByName(gundamSerialNum + "_ULeftLeg"));

			//R_Hand
			ReparentObject(FindObjectByName(gundamSerialNum + "_RightHand"), FindObjectByName(gundamSerialNum + "_DRightHand"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_DRightHand"), FindObjectByName(gundamSerialNum + "_URightHand"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_URightHand"), FindObjectByName(gundamSerialNum + "_RightShouder"));
			//L_Hand
			ReparentObject(FindObjectByName(gundamSerialNum + "_LeftHand"), FindObjectByName(gundamSerialNum + "_DLeftHand"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_DLeftHand"), FindObjectByName(gundamSerialNum + "_ULeftHand"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_ULeftHand"), FindObjectByName(gundamSerialNum + "_LeftShouder"));

			//Body Assembly
			ReparentObject(FindObjectByName(gundamSerialNum + "_RightShouder"), FindObjectByName(gundamSerialNum + "_Body"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_LeftShouder"), FindObjectByName(gundamSerialNum + "_Body"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_DBody"), FindObjectByName(gundamSerialNum + "_Body"));
			//ReparentObject(FindObjectByName(gundamSerialNum + "_Back"), FindObjectByName(gundamSerialNum + "_Body"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_Head"), FindObjectByName(gundamSerialNum + "_Body"));

			ReparentObject(FindObjectByName(gundamSerialNum + "_URightLeg"), FindObjectByName(gundamSerialNum + "_DBody"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_ULeftLeg"), FindObjectByName(gundamSerialNum + "_DBody"));
		}

		SetupSceneObject(model_photonBlade.get(), "PhotonBlade", "PhotonBlade");
		photonBladeTrail.color = glm::vec3(1.0f, 0.0f, 0.0f);  // 對應自發光色
		photonBladeTrail.duration = 0.3f;

		InitIKChains();

		return true;
	}

	// ── IK 鏈初始化 ────────────────────────────────────────────────────────────
	// 為每個 Gundam（0~3）建立四條鏈：右臂、左臂、右腿、左腿
	void MainScene::InitIKChains()
	{
		ikChains.clear();

		// 輔助 lambda：依名稱串接關節，若任一節點不存在則跳過
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

	void MainScene::SolveIK()
	{
		for (auto& chain : ikChains)
			IKSolver::Solve(chain);
	}
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

	SceneObject* MainScene::FindObjectByName(std::string objectName)
	{
		for (int i = 0; i < ObjectList.size(); i++)
		{
			if (ObjectList[i]->objectName == objectName)
			{
				return ObjectList[i];
			}
		}

		return nullptr;
	}
	void MainScene::ReparentObject(SceneObject* obj, SceneObject* newParent)
	{
		//在改變層級之前，先記錄物件目前的世界矩陣
		glm::mat4 oldWorldMatrix = obj->GetWorldMatrix();

		// 從舊父節點取出節點
		SceneObject* oldParent = obj->parent ? obj->parent : &rootObject;
		auto& siblings = oldParent->children;
		auto it = std::find_if(siblings.begin(), siblings.end(),
			[obj](const auto& c) { return c.get() == obj; });
		if (it == siblings.end()) return;

		auto node = std::move(*it);
		siblings.erase(it);
		node->parent = newParent;
		newParent->children.push_back(std::move(node));

		// 用新父節點的世界矩陣反推，得到能保持世界位置不變的新 local matrix
		glm::mat4 newParentWorld = newParent->GetWorldMatrix();
		glm::mat4 newLocalMatrix = glm::inverse(newParentWorld) * oldWorldMatrix;

		// 將 mat4 分解回 Transform / Rotate / Scale，寫入 local transform
		// 提取 Translation
		obj->transform.position = glm::vec3(newLocalMatrix[3]);

		// 提取 Scale（各軸向量長度）
		glm::vec3 newScale;
		newScale.x = glm::length(glm::vec3(newLocalMatrix[0]));
		newScale.y = glm::length(glm::vec3(newLocalMatrix[1]));
		newScale.z = glm::length(glm::vec3(newLocalMatrix[2]));
		obj->transform.scale = newScale;

		// 消除 scale 後提取 Rotation
		glm::mat3 rotMat(
			glm::vec3(newLocalMatrix[0]) / newScale.x,
			glm::vec3(newLocalMatrix[1]) / newScale.y,
			glm::vec3(newLocalMatrix[2]) / newScale.z
		);
		obj->transform.rotation = glm::quat_cast(rotMat);

		obj->MarkDirty();
	}

	void MainScene::RenderObjects(Shader* worldObjectShader)
	{
		worldObjectShader->use();

		// view / projection 只需上傳一次
		worldObjectShader->setUnifMat4("view", freeViewCamera.GetViewMatrix());
		worldObjectShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

		//遍歷場景樹，依 Model* 分組收集 world matrix
		std::unordered_map<Model*, std::vector<glm::mat4>> instanceMap; 
		CollectInstances(&rootObject, instanceMap);

		// ★ Step 2：每個獨立 Model 批次繪製
		for (auto& [model, matrices] : instanceMap)
			model->DrawInstanced(*worldObjectShader, matrices);
	}

	void MainScene::RenderTrails(Shader* trailShader)
	{
		trailShader->use();

		// view / projection 只需上傳一次
		trailShader->setUnifMat4("view", freeViewCamera.GetViewMatrix());
		trailShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

		float currentTime = (float)glfwGetTime();
		glm::vec3 worldEdgeA = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(10.0f, -0.2f, 0.0f, 1.0f));
		glm::vec3 worldEdgeB = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(0.5f, -0.2f, 0.0f, 1.0f));
		photonBladeTrail.update(worldEdgeA, worldEdgeB, currentTime);

		photonBladeTrail.Draw(trailShader);  // ← 在同一個 FBO 內
	}

	void MainScene::RenderParticles(Shader* particleShader)
	{
		particleShader->use();

		// view / projection 只需上傳一次
		particleShader->setUnifMat4("view", freeViewCamera.GetViewMatrix());
		particleShader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

		float currentTime = (float)glfwGetTime();
		float dt = currentTime - lastTime;

		lastTime = currentTime;
		glm::vec3 worldEdgeA = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(0.5f, -0.2f, 0.0f, 1.0f));
		glm::vec3 worldEdgeB = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(10.0f, -0.2f, 0.0f, 1.0f));
		

		glm::vec3 worldPos = glm::mix(worldEdgeA, worldEdgeB, (rand() % 100) / 100.f);
		
		photonBladePlasma.emit(worldPos,3);
		photonBladePlasma.update(dt);

		photonBladePlasma.Draw();
	}

	void MainScene::CollectInstances(SceneObject* obj, std::unordered_map<Model*, std::vector<glm::mat4>>& outMap)
	{
		if (obj->model != nullptr)
			outMap[obj->model].push_back(obj->GetWorldMatrix());

		for (auto& child : obj->children)
			CollectInstances(child.get(), outMap);
	}
}