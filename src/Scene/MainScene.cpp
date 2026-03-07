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
		freeViewCamera = Camera(glm::vec3(-3, 0, 0));
		freeViewCamera.configureLookAt(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));

		model_Train = new Model("objModels/Train/Train.obj");

		// 初始化場景物件
		SceneObject trainObj;
		trainObj.name = "Train Model";
		trainObj.model = model_Train;
		trainObj.objectType = 1;
		trainObj.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		sceneObjects.push_back(trainObj);

		return true;
	}

	SceneObject* MainScene::GetObjectByIndex(int index)
	{
		if (index >= 0 && index < sceneObjects.size())
			return &sceneObjects[index];
		return nullptr;
	}

	void MainScene::Render(Shader* shader, glm::mat4 projectionMtrx)
	{
		// Start drawing
		shader->use();

		glm::mat4 view = freeViewCamera.GetViewMatrix();
		// 渲染所有場景物件
		for (const auto& obj : sceneObjects)
		{
			if (obj.model)
			{
				glm::mat4 model = obj.transform.GetModelMatrix();
				shader->setUnifMat4("model", model);
				shader->setUnifMat4("view", view);
				shader->setUnifMat4("projection", projectionMtrx);
				obj.model->Draw(*shader);
			}
		}
	}
}