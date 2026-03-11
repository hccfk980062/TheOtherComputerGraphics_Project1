#include "InspectorWindow.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

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
		if (!selectedObject)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid object");
			return;
		}

		// Directly edit object transform
		ImGui::TextDisabled("Position");
		ImGui::DragFloat3("##Position", glm::value_ptr(selectedObject->transform.position), 0.1f);

		ImGui::TextDisabled("Rotation (Euler Angles)");
		ImGui::InputFloat3("##Rotation", glm::value_ptr(selectedObject->transform.rotation));

		ImGui::TextDisabled("Scale");
		ImGui::DragFloat3("##Scale", glm::value_ptr(selectedObject->transform.scale), 0.1f, 0.01f, 100.0f);

		ImGui::Spacing();
		if (ImGui::Button("Reset Transform", ImVec2(-1, 0)))
		{
			selectedObject->transform.position = glm::vec3(0.0f);
			selectedObject->transform.rotation = glm::vec3(0.0f);
			selectedObject->transform.scale = glm::vec3(1.0f);
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
}
