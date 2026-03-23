#pragma once
#include <array>
#include <string>
#include <map>
#include<stack>
#include <vector>
#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Model/ModelLoader.h"
#include "Scene/Transform.h"

#include "Camera/Camera.h"
namespace CG
{
	class MainScene
	{
	public:
		MainScene();
		~MainScene();

		auto Initialize() -> bool;
		void Render(Shader* shader);
		
		Camera freeViewCamera;

		//物件管理
		std::vector<SceneObject*>ObjectList;

		SceneObject rootObject;
		SceneObject* selectedObject = nullptr;
		int objectCount = 0;

		SceneObject* MainScene::FindObjectByName(char* objectName);
		void MainScene::ReparentObject(SceneObject* obj, SceneObject* newParent);

	private:
		Model* model_Gundam[18];

		void RenderObjectRecursively(Shader* shader, SceneObject* obj);

		void SetupSceneObject(Model* model, std::string modelName, glm::vec3 objectPosition = glm::vec3(0))
		{
			auto objUniquePtr = std::make_unique<SceneObject>();
			objUniquePtr->id = objectCount++;
			objUniquePtr->name = modelName;
			objUniquePtr->model = model;
			objUniquePtr->objectType = 1;
			objUniquePtr->transform.position = objectPosition;

			ObjectList.push_back(objUniquePtr.get());
			rootObject.children.push_back(std::move(objUniquePtr));
		}
	};
}

