#include "Window/ViewportWindow/ViewportWindow.h"

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

		{
			const float  PAD = 8.0f;   // 距邊框距離
			const float  BTN_WIDTH = 120.0f;
			const float  BTN_HEIGHT = 28.0f;
			const ImVec4 COL_ACTIVE = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // 藍色：選中
			const ImVec4 COL_NORMAL = ImVec4(0.20f, 0.20f, 0.20f, 0.75f); // 深灰：未選

			// 把 cursor 移到 viewport 內容區左上角 + padding
			ImGui::SetCursorScreenPos(ImVec2(imageScreenPos.x + PAD,
				imageScreenPos.y + PAD));

			// 讓三顆按鈕橫排緊靠
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
			// 圓角按鈕
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

			struct OperationBtn { const char* label; ImGuizmo::OPERATION op; };
			constexpr OperationBtn BUTTONS[] = {
				{ "[T]ranslate", ImGuizmo::TRANSLATE },
				{ "[R]otate", ImGuizmo::ROTATE    },
				{ "[S]cale", ImGuizmo::SCALE     },
			};

			for (const auto& btn : BUTTONS)
			{
				bool isActive = (objectTransformOperation == btn.op);
				ImGui::PushStyleColor(ImGuiCol_Button,
					isActive ? COL_ACTIVE : COL_NORMAL);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					isActive ? COL_ACTIVE : ImVec4(0.35f, 0.35f, 0.35f, 0.90f));

				if (ImGui::Button(btn.label, ImVec2(BTN_WIDTH, BTN_HEIGHT)))
					objectTransformOperation = btn.op;

				ImGui::PopStyleColor(2);
				ImGui::SameLine();
			}



			struct ModeBtn { const char* label; ImGuizmo::MODE mode; };
			constexpr ModeBtn MODEBUTTONS[] = {
				{ "[L]ocal", ImGuizmo::LOCAL },
				{ "[W]orld", ImGuizmo::WORLD    },
			};

			for (const auto& btn : MODEBUTTONS)
			{
				bool isActive = (objectTransformMode == btn.mode);
				ImGui::PushStyleColor(ImGuiCol_Button,
					isActive ? COL_ACTIVE : COL_NORMAL);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					isActive ? COL_ACTIVE : ImVec4(0.35f, 0.35f, 0.35f, 0.90f));

				if (ImGui::Button(btn.label, ImVec2(BTN_WIDTH, BTN_HEIGHT)))
					objectTransformMode = btn.mode;

				ImGui::PopStyleColor(2);
				ImGui::SameLine();
			}
			ImGui::PopStyleVar(2);   // ItemSpacing, FrameRounding
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
			glm::mat4 modelMatrix = scene->selectedObject->GetWorldMatrix();
			ImGuizmo::Manipulate(viewFlat, projFlat, objectTransformOperation, objectTransformMode, glm::value_ptr(modelMatrix));
			// ──  Gizmo 操作結果寫回 Transform ─────────────────────
			if (ImGuizmo::IsUsing())
			{
				if(scene->selectedObject->parent != nullptr)
				{
					glm::mat4 parentWorld = scene->selectedObject->parent->GetWorldMatrix();
					glm::mat4 newLocalMatrix = glm::inverse(parentWorld) * modelMatrix;
					ApplyMatrixToTransform(scene->selectedObject->transform, newLocalMatrix);
				}
				else
				{
					ApplyMatrixToTransform(scene->selectedObject->transform, modelMatrix);
				}
				scene->selectedObject->MarkDirty();
			}
		}
		ImGui::End();
	}

	void ViewportWindow::ApplyMatrixToTransform(Transform& transform, const glm::mat4& matrix)
	{
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];

		// ImGuizmo 內建的矩陣分解，比 glm::decompose 更穩定
		ImGuizmo::DecomposeMatrixToComponents(
			glm::value_ptr(matrix),
			matrixTranslation,
			matrixRotation,   // 回傳 Euler angles（degrees）
			matrixScale
		);

		glm::quat rotationQuat = glm::quat(glm::radians(glm::vec3(matrixRotation[0], matrixRotation[1], matrixRotation[2])));
		transform.position = { matrixTranslation[0], matrixTranslation[1], matrixTranslation[2] };
		transform.rotation = rotationQuat;
		transform.scale = { matrixScale[0],        matrixScale[1],       matrixScale[2] };
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