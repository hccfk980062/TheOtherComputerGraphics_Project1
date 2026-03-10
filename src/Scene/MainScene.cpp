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

		modelMtrxStack.push(glm::mat4(1.0f));

		freeViewCamera = Camera(glm::vec3(-3, 0, 0));
		freeViewCamera.configureLookAt(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));

		model_Train = new Model("objModels/Train/Train.obj");
		model_OtherTrain = new Model("objModels/anotherTrain/BTTF Loco UV Mapping 3_6 Animated.obj");

		// 初始化場景物件
		auto trainObj = std::make_unique<SceneObject>();
		trainObj->id = 1;
		trainObj->name = "Train Model";
		trainObj->model = model_Train;
		trainObj->objectType = 1;
		trainObj->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		rootObject.children.push_back(std::move(trainObj));

		auto otherTrainObj = std::make_unique<SceneObject>();
		otherTrainObj->id = 2;
		otherTrainObj->name = "Other Train Model";
		otherTrainObj->model = model_OtherTrain;
		otherTrainObj->objectType = 1;
		otherTrainObj->transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
		rootObject.children.push_back(std::move(otherTrainObj));

		return true;
	}

	void MainScene::Render(Shader* shader)
	{
		// Start drawing
		shader->use();

		// 渲染所有場景物件
		RenderObjectRecursively(shader, &rootObject);
	}
	void MainScene::RenderObjectRecursively(Shader* shader, SceneObject* obj)
	{
		glm::mat4 viewMtrx = freeViewCamera.GetViewMatrix();
		glm::mat4 projectionMtrx = freeViewCamera.GetProjectionMatrix();

		if (obj->model != nullptr)
			{
				glm::mat4 model = obj->transform.GetModelMatrix();
				shader->setUnifMat4("model", modelMtrxStack.top() * model);
				shader->setUnifMat4("view", viewMtrx);
				shader->setUnifMat4("projection", projectionMtrx);
				obj->model->Draw(*shader);
			}

		for (int i = 0; i < obj->children.size(); i++)
		{
			modelMtrxStack.push(obj->model != nullptr ? obj->transform.GetModelMatrix() : glm::mat4(1.0f));
			RenderObjectRecursively(shader, obj->children[i].get());
			modelMtrxStack.pop();
		}
	}
}