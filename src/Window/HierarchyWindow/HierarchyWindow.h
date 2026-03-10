#pragma once
#pragma once

#include <Scene/MainScene.h>

namespace CG
{
	class HierarchyWindow
	{
	public:
		HierarchyWindow();
		~HierarchyWindow();

		auto Initialize() -> bool;
		void Display();

		void SetTargetScene(MainScene* scene)
		{
			targetScene = scene;
		}

	private:
		MainScene* targetScene = nullptr;
		void DrawNode(SceneObject* obj);
		void DrawContextMenu(SceneObject* obj);
		void CreateObject();
		void DeleteObject();
		void ReparentObject(SceneObject* obj, SceneObject* newParent);
		bool IsAncestor(SceneObject* potentialAncestor, SceneObject* node);
	};
}