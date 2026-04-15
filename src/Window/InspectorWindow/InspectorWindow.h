#pragma once

#include <Scene/MainScene.h>

namespace CG
{
	class InspectorWindow
	{
	public:
		InspectorWindow();
		~InspectorWindow();

		auto Initialize() -> bool;
		void Display();

		void SetTargetScene(MainScene* scene)
		{
			targetScene = scene;
		}

	private:
		MainScene* targetScene = nullptr;

		// UI helpers
		void DisplayTransformPanel();
		void DisplayCameraPanel();
		void DisplayIKPanel();
	};
}