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
		SceneObject rootObject;
		SceneObject* selectedObject = nullptr;

	private:
		Model* model_Train;
		Model* model_OtherTrain;


		std::stack<glm::mat4>modelMtrxStack;
		void RenderObjectRecursively(Shader* shader, SceneObject* obj);
	};
}

