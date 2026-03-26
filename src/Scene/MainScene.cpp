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
		// 初始化場景物件

		SetupSceneObject(model_Gundam[0], "Gundam_Back", glm::vec3(0,0,-3.6));//
		SetupSceneObject(model_Gundam[1], "Gundam_Body", glm::vec3(0, 0, 0));//
		SetupSceneObject(model_Gundam[2], "Gundam_DBody", glm::vec3(0, -4.95, -0.8));//
		SetupSceneObject(model_Gundam[3], "Gundam_DLeftHand", glm::vec3(4.3, -1.2, -0.67));//
		SetupSceneObject(model_Gundam[4], "Gundam_DLeftLeg", glm::vec3(1.93, -15.13, -1.93));//
		SetupSceneObject(model_Gundam[5], "Gundam_DRightHand", glm::vec3(-4.57, -1.25, -0.58));//
		SetupSceneObject(model_Gundam[6], "Gundam_DRightLeg", glm::vec3(-1.78, -15.13, -1.73));//
		SetupSceneObject(model_Gundam[7], "Gundam_Head", glm::vec3(0, 3.65, 0.58));//
		SetupSceneObject(model_Gundam[8], "Gundam_LeftFoot", glm::vec3(2, -18.32, -1.5));//
		SetupSceneObject(model_Gundam[9], "Gundam_LeftHand", glm::vec3(4.3, -5.4, -0.53));//
		SetupSceneObject(model_Gundam[10], "Gundam_LeftShouder", glm::vec3(4.22, 0.82, -0.55));//
		SetupSceneObject(model_Gundam[11], "Gundam_RightFoot", glm::vec3(-2, -18.32, -1.5));//
		SetupSceneObject(model_Gundam[12], "Gundam_RightHand", glm::vec3(-4.6, -5.76, -0.52));//
		SetupSceneObject(model_Gundam[13], "Gundam_RightShouder", glm::vec3(-4.22, 0.82, -0.55));//
		SetupSceneObject(model_Gundam[14], "Gundam_ULeftHand", glm::vec3(4, -0.75, -0.65));//
		SetupSceneObject(model_Gundam[15], "Gundam_ULeftLeg", glm::vec3(1.93, -8.27, -1.3));//
		SetupSceneObject(model_Gundam[16], "Gundam_URightHand", glm::vec3(-4.34, -0.62, -0.58));//
		SetupSceneObject(model_Gundam[17], "Gundam_URightLeg", glm::vec3(-1.93, -8.27, -1.3));//
		SetupSceneObject(model_Gundam[17], "Another_Gundam_URightLeg", glm::vec3(-1.93, -8.27, -1.3));//
		//R_Leg
		ReparentObject(FindObjectByName("Gundam_RightFoot"), FindObjectByName("Gundam_DRightLeg"));
		ReparentObject(FindObjectByName("Gundam_DRightLeg"), FindObjectByName("Gundam_URightLeg"));
		
		//L_Leg
		ReparentObject(FindObjectByName("Gundam_LeftFoot"), FindObjectByName("Gundam_DLeftLeg"));
		ReparentObject(FindObjectByName("Gundam_DLeftLeg"), FindObjectByName("Gundam_ULeftLeg"));

		//R_Hand
		ReparentObject(FindObjectByName("Gundam_RightHand"), FindObjectByName("Gundam_DRightHand"));
		ReparentObject(FindObjectByName("Gundam_DRightHand"), FindObjectByName("Gundam_URightHand"));
		ReparentObject(FindObjectByName("Gundam_URightHand"), FindObjectByName("Gundam_RightShouder"));
		//L_Hand
		ReparentObject(FindObjectByName("Gundam_LeftHand"), FindObjectByName("Gundam_DLeftHand"));
		ReparentObject(FindObjectByName("Gundam_DLeftHand"), FindObjectByName("Gundam_ULeftHand"));
		ReparentObject(FindObjectByName("Gundam_ULeftHand"), FindObjectByName("Gundam_LeftShouder"));
		
		//Body Assembly
		ReparentObject(FindObjectByName("Gundam_RightShouder"), FindObjectByName("Gundam_Body"));
		ReparentObject(FindObjectByName("Gundam_LeftShouder"), FindObjectByName("Gundam_Body"));
		ReparentObject(FindObjectByName("Gundam_DBody"), FindObjectByName("Gundam_Body"));
		ReparentObject(FindObjectByName("Gundam_Back"), FindObjectByName("Gundam_Body"));
		ReparentObject(FindObjectByName("Gundam_Head"), FindObjectByName("Gundam_Body"));

		ReparentObject(FindObjectByName("Gundam_URightLeg"), FindObjectByName("Gundam_DBody"));
		ReparentObject(FindObjectByName("Gundam_ULeftLeg"), FindObjectByName("Gundam_DBody"));
		return true;
	}

	SceneObject* MainScene::FindObjectByName(char* objectName)
	{
		for (int i = 0; i < ObjectList.size(); i++)
		{
			if (ObjectList[i]->name == objectName)
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

	void MainScene::Render(Shader* shader)
	{
		shader->use();

		// view / projection 只需上傳一次
		shader->setUnifMat4("view", freeViewCamera.GetViewMatrix());
		shader->setUnifMat4("projection", freeViewCamera.GetProjectionMatrix());

		//遍歷場景樹，依 Model* 分組收集 world matrix
		std::unordered_map<Model*, std::vector<glm::mat4>> instanceMap; 
		CollectInstances(&rootObject, instanceMap);

		// ★ Step 2：每個獨立 Model 批次繪製
		for (auto& [model, matrices] : instanceMap)
			model->DrawInstanced(*shader, matrices);
	}

	void MainScene::CollectInstances(SceneObject* obj, std::unordered_map<Model*, std::vector<glm::mat4>>& outMap)
	{
		if (obj->model != nullptr)
			outMap[obj->model].push_back(obj->GetWorldMatrix());

		for (auto& child : obj->children)
			CollectInstances(child.get(), outMap);
	}
}