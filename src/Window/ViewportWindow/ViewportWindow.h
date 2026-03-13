#pragma once

#include<imgui.h>
#include <ImGuizmo.h>          // ← 補上，讓 OPERATION 型別可用

#include<glm/gtc/type_ptr.hpp>
#include "Scene/MainScene.h"
#include "FrameBuffer/Framebuffer.h"
namespace CG
{
	class ViewportWindow
	{
	public:
		ViewportWindow();
		~ViewportWindow();

		auto Initialize() -> bool;
		void SyncFramebufferSize(Framebuffer* framebuffer);
		void UpdateScreen(MainScene *scene, Framebuffer *framebuffer);
	private:
		int   m_lastViewportWidth = 0, m_lastViewportHeight = 0;  // 記錄上一幀的 viewport 尺寸
		bool  m_rightMouseDown;
		float m_lastMouseX, m_lastMouseY;

		ImGuizmo::OPERATION objectTransformOperation = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE objectTransformMode = ImGuizmo::LOCAL;

		void ApplyMatrixToTransform(Transform& transform, const glm::mat4& matrix);
	};
}