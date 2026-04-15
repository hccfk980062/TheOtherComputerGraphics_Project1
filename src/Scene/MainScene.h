#pragma once
#include <array>
#include <string>
#include <map>
#include<stack>
#include <vector>
#include <memory>
#include <unordered_map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Model/ModelLoader.h"
#include "Scene/Transform.h"

#include "ParticleEffects/ParticleRenderer.h"
#include "ParticleEffects/TrailRenderer.h"

#include "Camera/Camera.h"
#include "IK/IKSolver.h"
namespace CG
{
	class MainScene
	{
	public:
		MainScene();
		~MainScene();

		auto Initialize() -> bool;
		void RenderObjects(Shader* worldObjectShader);
		void RenderParticles(Shader* particleShader);
		void RenderTrails(Shader* trailShader);
		Camera freeViewCamera;
		//物件管理
		std::vector<SceneObject*>ObjectList;

		SceneObject rootObject;
		SceneObject* selectedObject = nullptr;
		int objectCount = 0;

		TrailRenderer photonBladeTrail;
		ParticleRenderer photonBladePlasma = ParticleRenderer(16);

		// ── Inverse Kinematics ────────────────────────────────────────────────
		std::vector<IKChain> ikChains;
		void SolveIK();

		SceneObject* FindObjectByName(std::string objectName);
		std::vector<SceneObject*> GetObjectsInAnimationGroup(std::string groupName);

		void ReparentObject(SceneObject* obj, SceneObject* newParent);

	private:
		void InitIKChains();

		std::unique_ptr<Model> model_Gundam[18];
		std::unique_ptr<Model> model_photonBlade;

		float lastTime = (float)glfwGetTime();
		//Model* model_Hand[2];

		void CollectInstances(SceneObject* obj,std::unordered_map<Model*, std::vector<glm::mat4>>& outMap);

		void SetupSceneObject(Model* model, std::string modelName, std::string animationSerializedName, glm::vec3 objectPosition = glm::vec3(0))
		{
			auto objUniquePtr = std::make_unique<SceneObject>();
			objUniquePtr->id = objectCount++;
			objUniquePtr->objectName = (modelName + "_" + animationSerializedName);
			objUniquePtr->animationGroupName = modelName;
			objUniquePtr->animationSerializedName = animationSerializedName;
			objUniquePtr->model = model;
			objUniquePtr->objectType = 1;
			objUniquePtr->transform.position = objectPosition;

			ObjectList.push_back(objUniquePtr.get());
			rootObject.children.push_back(std::move(objUniquePtr));
		}
	};
}

