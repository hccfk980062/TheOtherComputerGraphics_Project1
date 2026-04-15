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


		model_Gundam[0] = new Model("objModels/Gundam_OriginRepositioned/back.obj", false, false);
		model_Gundam[1] = new Model("objModels/Gundam_OriginRepositioned/body.obj", false, false);
		model_Gundam[2] = new Model("objModels/Gundam_OriginRepositioned/dbody.obj", false, false);
		model_Gundam[3] = new Model("objModels/Gundam_OriginRepositioned/dlefthand.obj", false, false);
		model_Gundam[4] = new Model("objModels/Gundam_OriginRepositioned/dleftleg.obj", false, false);
		model_Gundam[5] = new Model("objModels/Gundam_OriginRepositioned/drighthand.obj", false, false);
		model_Gundam[6] = new Model("objModels/Gundam_OriginRepositioned/drightleg.obj", false, false);
		model_Gundam[7] = new Model("objModels/Gundam_OriginRepositioned/head.obj", false, false);
		model_Gundam[8] = new Model("objModels/Gundam_OriginRepositioned/leftfoot.obj", false, false);
		model_Gundam[9] = new Model("objModels/Gundam_OriginRepositioned/lefthand.obj", false, false);
		model_Gundam[10] = new Model("objModels/Gundam_OriginRepositioned/lshouder.obj", false, false);
		model_Gundam[11] = new Model("objModels/Gundam_OriginRepositioned/rightfoot.obj", false, false);
		model_Gundam[12] = new Model("objModels/Gundam_OriginRepositioned/righthand.obj", false, false);
		model_Gundam[13] = new Model("objModels/Gundam_OriginRepositioned/rshouder.obj", false, false);
		model_Gundam[14] = new Model("objModels/Gundam_OriginRepositioned/ulefthand.obj", false, false);
		model_Gundam[15] = new Model("objModels/Gundam_OriginRepositioned/uleftleg.obj", false, false);
		model_Gundam[16] = new Model("objModels/Gundam_OriginRepositioned/urighthand.obj", false, false);
		model_Gundam[17] = new Model("objModels/Gundam_OriginRepositioned/urightleg.obj", false, false);

		//model_Hand[0] = new Model(5.0f, 3.0f, 1.0f);
		//model_Hand[1] = new Model(0.9f, 2.5f, 0.9f);

		model_photonBlade = new Model("objModels/PhotonBlade/untitled.fbx", false, true);
		// 初始化場景物件

		for (int i = 0; i < 4; i++)
		{
			std::string gundamSerialNum = "Gundam_" + std::to_string(i);

			SetupSceneObject(model_Gundam[0], gundamSerialNum , "Back", glm::vec3(0, 0, -3.6));//
			SetupSceneObject(model_Gundam[1], gundamSerialNum , "Body", glm::vec3(0, 0, 0));//
			SetupSceneObject(model_Gundam[2], gundamSerialNum, "DBody", glm::vec3(0, -4.95, -0.8));//
			SetupSceneObject(model_Gundam[3], gundamSerialNum, "DLeftHand", glm::vec3(4.3, -4.2, -0.67));//
			SetupSceneObject(model_Gundam[4], gundamSerialNum, "DLeftLeg", glm::vec3(1.93, -15.13, -1.93));//
			SetupSceneObject(model_Gundam[5], gundamSerialNum, "DRightHand", glm::vec3(-4.57, -4.25, -0.58));//
			SetupSceneObject(model_Gundam[6], gundamSerialNum, "DRightLeg", glm::vec3(-1.78, -15.13, -1.73));//
			SetupSceneObject(model_Gundam[7], gundamSerialNum, "Head", glm::vec3(0, 3.65, 0.58));//
			SetupSceneObject(model_Gundam[8], gundamSerialNum, "LeftFoot", glm::vec3(2, -18.32, -1.5));//
			SetupSceneObject(model_Gundam[9], gundamSerialNum, "LeftHand", glm::vec3(4.3, -8.4, -0.53));//
			SetupSceneObject(model_Gundam[10], gundamSerialNum, "LeftShouder", glm::vec3(4.22, 0.82, -0.55));//
			SetupSceneObject(model_Gundam[11], gundamSerialNum, "RightFoot", glm::vec3(-2, -18.32, -1.5));//
			SetupSceneObject(model_Gundam[12], gundamSerialNum, "RightHand", glm::vec3(-4.6, -8.76, -0.52));//
			SetupSceneObject(model_Gundam[13], gundamSerialNum, "RightShouder", glm::vec3(-4.22, 0.82, -0.55));//
			SetupSceneObject(model_Gundam[14], gundamSerialNum, "ULeftHand", glm::vec3(4, -0.75, -0.65));//
			SetupSceneObject(model_Gundam[15], gundamSerialNum, "ULeftLeg", glm::vec3(1.93, -8.27, -1.3));//
			SetupSceneObject(model_Gundam[16], gundamSerialNum, "URightHand", glm::vec3(-4.34, -0.62, -0.58));//
			SetupSceneObject(model_Gundam[17], gundamSerialNum, "URightLeg", glm::vec3(-1.93, -8.27, -1.3));//

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
			ReparentObject(FindObjectByName(gundamSerialNum + "_Back"), FindObjectByName(gundamSerialNum + "_Body"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_Head"), FindObjectByName(gundamSerialNum + "_Body"));

			ReparentObject(FindObjectByName(gundamSerialNum + "_URightLeg"), FindObjectByName(gundamSerialNum + "_DBody"));
			ReparentObject(FindObjectByName(gundamSerialNum + "_ULeftLeg"), FindObjectByName(gundamSerialNum + "_DBody"));
		}

		SetupSceneObject(model_photonBlade, "PhotonBlade", "PhotonBlade");
		photonBladeTrail.color = glm::vec3(1.0f, 0.0f, 0.0f);  // 對應自發光色
		photonBladeTrail.duration = 0.3f;


		//Hand
		/*
		{

			SetupSceneObject(model_Hand[0], "Hand", "Palm");
			SetupSceneObject(model_Hand[1], "Hand", "Finger0_D", glm::vec3(-4, 3, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger0_U", glm::vec3(-4, 6, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger1_D", glm::vec3(-2, 3, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger1_U", glm::vec3(-2, 7, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger2_D", glm::vec3(0, 3, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger2_U", glm::vec3(0, 8, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger3_D", glm::vec3(2, 3, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger3_U", glm::vec3(2, 7, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger4_D", glm::vec3(4, 3, 0));
			SetupSceneObject(model_Hand[1], "Hand", "Finger4_U", glm::vec3(4, 6, 0));

			ReparentObject(FindObjectByName("Hand_Finger0_U"), FindObjectByName("Hand_Finger0_D"));
			ReparentObject(FindObjectByName("Hand_Finger0_D"), FindObjectByName("Hand_Palm"));
			ReparentObject(FindObjectByName("Hand_Finger1_U"), FindObjectByName("Hand_Finger1_D"));
			ReparentObject(FindObjectByName("Hand_Finger1_D"), FindObjectByName("Hand_Palm"));
			ReparentObject(FindObjectByName("Hand_Finger2_U"), FindObjectByName("Hand_Finger2_D"));
			ReparentObject(FindObjectByName("Hand_Finger2_D"), FindObjectByName("Hand_Palm"));
			ReparentObject(FindObjectByName("Hand_Finger3_U"), FindObjectByName("Hand_Finger3_D"));
			ReparentObject(FindObjectByName("Hand_Finger3_D"), FindObjectByName("Hand_Palm"));
			ReparentObject(FindObjectByName("Hand_Finger4_U"), FindObjectByName("Hand_Finger4_D"));
			ReparentObject(FindObjectByName("Hand_Finger4_D"), FindObjectByName("Hand_Palm"));
		}
		*/
		return true;
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

		// 將 mat4 分解回 Transform / Rotat e/ Scale，寫入 local transform
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
		glm::vec3 worldEdgeA = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(10.0f, 0.0f, 0.0f, 1.0f));
		glm::vec3 worldEdgeB = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
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
		glm::vec3 worldEdgeA = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(0.5f, -0.15f, 0.0f, 1.0f));
		glm::vec3 worldEdgeB = glm::vec3(FindObjectByName("PhotonBlade_PhotonBlade")->GetWorldMatrix() * glm::vec4(10.0f, -0.15f, 0.0f, 1.0f));
		

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