#include<imgui.h>
#include<ImGuizmo.h>

#include<Window/ViewportWindow/ViewportWindow.h>

namespace CG
{
	ViewportWindow::ViewportWindow() : 
		m_lastViewportWidth(0),   // ← 關鍵修正：避免垃圾值觸發 ResizeFramebuffer
		m_lastViewportHeight(0),
		m_rightMouseDown(false),
		m_lastMouseX(0.0f),
		m_lastMouseY(0.0f)
	{
	}

	ViewportWindow::~ViewportWindow()
	{
	}

	auto ViewportWindow::Initialize() -> bool
	{
		return true;
	}

	void ViewportWindow::SyncFramebufferSize(Framebuffer* framebuffer)
	{
		if (m_lastViewportWidth > 0 && m_lastViewportHeight > 0)
		{
			if (m_lastViewportWidth != framebuffer->width ||
				m_lastViewportHeight != framebuffer->height)
			{
				framebuffer->ResizeFramebuffer(m_lastViewportWidth, m_lastViewportHeight);
			}
		}
	}

	void ViewportWindow::UpdateScreen(MainScene* scene, Framebuffer* framebuffer)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport (ImGuizmo NMSL Edition)", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();

		// ── 記錄本幀的 Viewport 尺寸，供下一幀 SyncFramebufferSize() 使用 ──────
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		// 防止視窗被縮到 0 造成 OpenGL 錯誤
		if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
		if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;
		m_lastViewportWidth = (int)viewportSize.x;
		m_lastViewportHeight = (int)viewportSize.y;

		// ── [Bug Fix #2] 在 Image() 之前取得 content 起點座標 ──────────────────
		// 原本用 GetWindowPos()，那個點包含 Title Bar，導致 ImGuizmo rect 偏移
		ImVec2 imageScreenPos = ImGui::GetCursorScreenPos();

		// ── 顯示 FBO texture ───────────────────────────────────────────────────
		ImGui::GetWindowDrawList()->AddImage(
			(ImTextureID)(intptr_t)framebuffer->colorTexture,
			imageScreenPos,
			ImVec2(imageScreenPos.x + viewportSize.x, imageScreenPos.y + viewportSize.y),
			ImVec2(0, 1), ImVec2(1, 0)  // flip Y
		);

		// 用 IsWindowHovered() 偵測輸入
		if (ImGui::IsWindowHovered())
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.MouseDown[ImGuiMouseButton_Right])
			{
				float dx = io.MouseDelta.x;
				float dy = -io.MouseDelta.y;
				if (dx != 0.0f || dy != 0.0f)
					scene->freeViewCamera.ProcessMouseMovement(dx, dy);

				std::array<bool, 4> pressedKey = {
					ImGui::IsKeyDown(ImGuiKey_W),
					ImGui::IsKeyDown(ImGuiKey_S),
					ImGui::IsKeyDown(ImGuiKey_A),
					ImGui::IsKeyDown(ImGuiKey_D)
				};
				scene->freeViewCamera.ProcessKeyboard(pressedKey, 0.05);
			}
		}
		
		// ── ImGuizmo overlay ───────────────────────────────────────────────────
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();  // 畫到當前視窗的 drawlist

		// [Bug Fix #2] 使用 imageScreenPos（content 起點）而非 GetWindowPos()
		ImGuizmo::SetRect(imageScreenPos.x, imageScreenPos.y, viewportSize.x, viewportSize.y);

		glm::mat4 viewMatrix = scene->freeViewCamera.GetViewMatrix();
		glm::mat4 projMatrix = scene->freeViewCamera.GetProjectionMatrix();

		glm::mat4 identityMatrix = glm::mat4(1.0f);

		float viewFlat[16], projFlat[16], identityFlat[16];
		memcpy(viewFlat, glm::value_ptr(viewMatrix), sizeof(viewFlat));
		memcpy(projFlat, glm::value_ptr(projMatrix), sizeof(projFlat));
		memcpy(identityFlat, glm::value_ptr(identityMatrix), sizeof(identityFlat));
		
		if (!scene->rootObject.children.empty() && scene->selectedObject != nullptr)
		{
			glm::mat4 modelMatrix = scene->selectedObject->transform.GetModelMatrix();
			float modelFlat[16];
			memcpy(modelFlat, glm::value_ptr(modelMatrix), sizeof(modelFlat));

			ImGuizmo::Manipulate(viewFlat, projFlat,ImGuizmo::TRANSLATE,ImGuizmo::LOCAL,modelFlat);
			// ── [Bug Fix #3] Gizmo 操作結果寫回 Transform ─────────────────────
			// 原本的 memcpy 被 comment 掉，且目標是 local variable，完全沒有作用
			if (ImGuizmo::IsUsing())
			{
				float t[3], r[3], s[3];
				ImGuizmo::DecomposeMatrixToComponents(modelFlat, t, r, s);

				scene->selectedObject->transform.position = glm::vec3(t[0], t[1], t[2]);

				// 若 Transform 有 rotation / scale 欄位，一起更新：
				// scene->sceneObjects[0].transform.rotation = glm::vec3(r[0], r[1], r[2]);
				// scene->sceneObjects[0].transform.scale    = glm::vec3(s[0], s[1], s[2]);
			}
		}
		

		ImGui::End();
	}
	/*
	void ViewportWindow::OnResize(int width, int height)
	{
		std::cout << "MainScene Resize: " << width << " " << height << std::endl;
	}

	void ViewportWindow::OnKeyboard(int key, int action)
	{
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case GLFW_KEY_W:
				pressedButton[0] = true;
				break;
			case GLFW_KEY_S:
				pressedButton[1] = true;
				break;
			case GLFW_KEY_A:
				pressedButton[2] = true;
				break;
			case GLFW_KEY_D:
				pressedButton[3] = true;
				break;
			default:
				break;
			}
		}
		if (action == GLFW_RELEASE)
		{
			switch (key)
			{
			case GLFW_KEY_W:
				pressedButton[0] = false;
				break;
			case GLFW_KEY_S:
				pressedButton[1] = false;
				break;
			case GLFW_KEY_A:
				pressedButton[2] = false;
				break;
			case GLFW_KEY_D:
				pressedButton[3] = false;
				break;
			default:
				break;
			}
		}
	}
	void ViewportWindow::OnMouseClick(int button, int action)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS) {
				flag_RightButtonDown = true;
			}
			else if (action == GLFW_RELEASE) {
				flag_RightButtonDown = false;
			}
		}
	}
	void ViewportWindow::OnMouseDrag(double xPos, double yPos)
	{
		if (!flag_RightButtonDown)
		{
			last_Xpos = xPos;
			last_Ypos = yPos;
		}

		if (flag_RightButtonDown) {
			// Calculate the change in position
			double delta_x = xPos - last_Xpos;
			double delta_y = last_Ypos - yPos; // Note: y-coordinates are top-to-bottom by default

			// Perform your drag operations here (e.g., move an object, rotate camera)

			freeViewCamera.ProcessMouseMovement(delta_x, delta_y);
			// Update the last position for the next callback
			last_Xpos = xPos;
			last_Ypos = yPos;
		}
	}
	*/
}