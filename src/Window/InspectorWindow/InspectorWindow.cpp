#include "InspectorWindow.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace CG
{
	InspectorWindow::InspectorWindow()
		: targetScene(nullptr)
	{
	}

	InspectorWindow::~InspectorWindow()
	{
	}

	auto InspectorWindow::Initialize() -> bool
	{
		return true;
	}

	void InspectorWindow::Display()
	{
		if (ImGui::Begin("Totally not Unity Inspector", nullptr))
		{
			if (targetScene)
			{
				ImGui::BeginTabBar("InspectorTabs");
				{
					/*
					if (ImGui::BeginTabItem("Scene"))
					{
						DisplayScenePanel();
						ImGui::EndTabItem();
					}
					*/
					if (ImGui::BeginTabItem("Transform"))
					{
						DisplayTransformPanel();
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Camera"))
					{
						DisplayCameraPanel();
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("IK"))
					{
						DisplayIKPanel();
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Scene Assigned!");
			}
		}
		ImGui::End();
	}

	void InspectorWindow::DisplayCameraPanel()
	{
		ImGui::Text("Free View Camera");
		ImGui::Separator();

		if (!targetScene)
			return;

		Camera& cam = targetScene->freeViewCamera;

		ImGui::TextDisabled("Position");
		ImGui::DragFloat3("##CamPos", glm::value_ptr(cam.Position), 0.1f);

		ImGui::TextDisabled("Rotation");
		ImGui::SliderFloat("Yaw##Cam", &cam.Yaw, -180.0f, 180.0f);
		ImGui::SliderFloat("Pitch##Cam", &cam.Pitch, -89.0f, 89.0f);

		ImGui::TextDisabled("Settings");
		ImGui::DragFloat("Movement Speed##Cam", &cam.MovementSpeed, 0.1f, 0.1f, 50.0f);
		ImGui::DragFloat("Mouse Sensitivity##Cam", &cam.MouseSensitivity, 0.01f, 0.01f, 1.0f);
		ImGui::DragFloat("Zoom##Cam", &cam.Zoom, 0.1f, 10.0f, 120.0f);

		ImGui::Spacing();
		ImGui::TextDisabled("Camera Vectors");
		ImGui::Text("Front: (%.2f, %.2f, %.2f)", cam.Front.x, cam.Front.y, cam.Front.z);
		ImGui::Text("Up: (%.2f, %.2f, %.2f)", cam.Up.x, cam.Up.y, cam.Up.z);
		ImGui::Text("Right: (%.2f, %.2f, %.2f)", cam.Right.x, cam.Right.y, cam.Right.z);
	}

	void InspectorWindow::DisplayTransformPanel()
	{
		ImGui::Text("Transform Properties");
		ImGui::Separator();

		if (targetScene->selectedObject == nullptr)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Select an object in Hierarchy Window to edit its transform");
			return;
		}

		SceneObject* selectedObject = targetScene->selectedObject;

		// Directly edit object transform
		ImGui::TextDisabled("Position");
		if (ImGui::DragFloat3("##Position", glm::value_ptr(selectedObject->transform.position), 0.1f))
		{
			selectedObject->MarkDirty();
		}

		ImGui::TextDisabled("Rotation (Euler Angles)");
		{
			glm::vec3 euler = glm::degrees(glm::eulerAngles(selectedObject->transform.rotation));
			if (ImGui::DragFloat3("##Rotation", glm::value_ptr(euler), 0.5f))
			{
				selectedObject->transform.rotation = glm::quat(glm::radians(euler));
				selectedObject->MarkDirty();
			}
		}

		ImGui::TextDisabled("Scale");
		if(ImGui::DragFloat3("##Scale", glm::value_ptr(selectedObject->transform.scale), 0.1f, 0.01f, 100.0f))
		{
			selectedObject->MarkDirty();
		}

		ImGui::Spacing();
		if (ImGui::Button("Reset Transform", ImVec2(-1, 0)))
		{
			selectedObject->transform.position = glm::vec3(0.0f);
			selectedObject->transform.rotation = glm::quat();
			selectedObject->transform.scale = glm::vec3(1.0f);
			selectedObject->MarkDirty();
		}

		// Display transformation matrix preview
		ImGui::Spacing();
		ImGui::TextDisabled("Local Model Matrix (Preview)");
		glm::mat4 model = selectedObject->transform.GetLocalMatrix();

		ImGui::Text("Row 0: (%.2f, %.2f, %.2f, %.2f)", model[0][0], model[0][1], model[0][2], model[0][3]);
		ImGui::Text("Row 1: (%.2f, %.2f, %.2f, %.2f)", model[1][0], model[1][1], model[1][2], model[1][3]);
		ImGui::Text("Row 2: (%.2f, %.2f, %.2f, %.2f)", model[2][0], model[2][1], model[2][2], model[2][3]);
		ImGui::Text("Row 3: (%.2f, %.2f, %.2f, %.2f)", model[3][0], model[3][1], model[3][2], model[3][3]);
	}

	// ─── IK Panel ──────────────────────────────────────────────────────────────
	void InspectorWindow::DisplayIKPanel()
	{
		if (!targetScene)
			return;

		auto& chains = targetScene->ikChains;
		if (chains.empty())
		{
			ImGui::TextDisabled("No IK chains defined.");
			return;
		}

		ImGui::Text("Inverse Kinematics (FABRIK)");
		ImGui::Separator();

		for (int idx = 0; idx < (int)chains.size(); idx++)
		{
			IKChain& chain = chains[idx];

			// 折疊標題列，包含 enabled checkbox
			std::string headerLabel = chain.name + "##header" + std::to_string(idx);
			bool treeOpen = ImGui::TreeNodeEx(headerLabel.c_str(),
				ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth);

			// 右側顯示 Enable 開關（與 TreeNode 同列）
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.0f);
			std::string checkId = "##en" + std::to_string(idx);
			ImGui::Checkbox(checkId.c_str(), &chain.enabled);

			if (treeOpen)
			{
				ImGui::PushID(idx);

				// ── 目標座標 ───────────────────────────────────────────────────
				ImGui::TextDisabled("Target (World)");
				ImGui::DragFloat3("##Target", glm::value_ptr(chain.target), 0.05f);

				// 快速按鈕：將目標重設到末端效應器當前世界位置
				if (ImGui::Button("Snap to End Effector"))
				{
					if (!chain.joints.empty())
						chain.target = glm::vec3(chain.joints.back()->GetWorldMatrix()[3]);
				}

				ImGui::Spacing();

				// ── 解算參數 ───────────────────────────────────────────────────
				ImGui::TextDisabled("Solver Settings");
				ImGui::SliderInt("Max Iterations", &chain.maxIter, 1, 50);
				ImGui::DragFloat("Tolerance",      &chain.tolerance, 0.001f, 0.001f, 1.0f, "%.3f");

				// ── 骨骼資訊（唯讀） ───────────────────────────────────────────
				ImGui::Spacing();
				ImGui::TextDisabled("Chain Info");
				ImGui::Text("Joints : %d", (int)chain.joints.size());
				ImGui::Text("Reach  : %.2f", chain.totalLength);

				if (!chain.joints.empty())
				{
					glm::vec3 endWorld = glm::vec3(chain.joints.back()->GetWorldMatrix()[3]);
					float distToTarget = glm::length(chain.target - endWorld);
					ImGui::Text("Dist to target: %.2f", distToTarget);
				}

				ImGui::Spacing();
				ImGui::TextDisabled("Joint Chain");
				for (int j = 0; j < (int)chain.joints.size(); j++)
				{
					bool isEnd = (j == (int)chain.joints.size() - 1);
					ImGui::Text("  [%d] %s%s", j,
						chain.joints[j]->objectName.c_str(),
						isEnd ? "  <-- end effector" : "");
				}

				// ── 重新計算骨骼長度（如手動調整過 T-pose） ────────────────────
				ImGui::Spacing();
				if (ImGui::Button("Recompute Bone Lengths"))
					chain.ComputeBoneLengths();

				ImGui::PopID();
				ImGui::TreePop();
			}

			ImGui::Spacing();
		}
	}
}
