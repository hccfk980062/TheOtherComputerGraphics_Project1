#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CG
{
	struct Transform
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);  // 歐拉角度 (度)
		glm::vec3 scale = glm::vec3(1.0f);

		// 計算模型矩陣
		glm::mat4 GetModelMatrix() const
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, position);
			model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
			model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
			model = glm::scale(model, scale);
			return model;
		}
	};

	struct SceneObject
	{
		uint32_t id;

		std::string name;
		Transform transform;
		Model* model = nullptr;
		int objectType; // 0=Camera, 1=Model, 2=Light

		SceneObject* parent = nullptr;
		std::vector<std::unique_ptr<SceneObject>> children;
	};
}