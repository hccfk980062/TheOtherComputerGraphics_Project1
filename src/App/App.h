#pragma once
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Window/InspectorWindow/InspectorWindow.h"
#include "Window/ViewportWindow/ViewportWindow.h"
#include "Window/HierarchyWindow/HierarchyWindow.h"
#include "Window/SequencerWindow/SequencerWindow.h"

#include "Scene/SceneRenderer.h"
#include "Scene/MainScene.h"

namespace CG
{
	class App
	{
	public:
		App();
		~App();

		auto Initialize() -> bool;
		void Loop();
		void Terminate();

	private:
		void Update(double dt);
		void BeginDockspace();
	private:
		GLFWwindow* mainWindow;

		std::unique_ptr<InspectorWindow> inspectorWindow;
		std::unique_ptr<ViewportWindow>  viewportWindow;
		std::unique_ptr<HierarchyWindow> hierarchyWindow;
		std::unique_ptr<SequencerWindow> sequencerWindow;

		std::unique_ptr<SceneRenderer> sceneRenderer;
		std::unique_ptr<MainScene>     mainScene;

		double timeNow = 0;
		double timeLast = 0;
		double timeDelta = 0;

	public:
		MainScene* GetMainScene() const
		{
			return mainScene.get();
		}
	};
}

