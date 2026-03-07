#pragma once
#include <array>
#include <string>
#include <map>
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
		void Render(Shader* shader, glm::mat4 projectionMtrx);
		
		Camera freeViewCamera;

		//物件管理
		std::vector<SceneObject> sceneObjects;
		SceneObject* GetObjectByIndex(int index);
		int GetObjectCount() const { return sceneObjects.size(); }

	private:
		Model* model_Train;
	};
}

