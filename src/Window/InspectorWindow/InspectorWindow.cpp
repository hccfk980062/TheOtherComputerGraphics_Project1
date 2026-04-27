#include "InspectorWindow.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace CG
{
    InspectorWindow::InspectorWindow() : targetScene(nullptr) {}
    InspectorWindow::~InspectorWindow() {}

    auto InspectorWindow::Initialize() -> bool { return true; }

    void InspectorWindow::Display()
    {
        if (ImGui::Begin("Totally not Unity Inspector", nullptr))
        {
            if (targetScene)
            {
                // 以分頁列（TabBar）區分不同屬性面板
                ImGui::BeginTabBar("InspectorTabs");
                {
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

    // 攝影機屬性面板：直接編輯 freeViewCamera 的成員變數
    void InspectorWindow::DisplayCameraPanel()
    {
        ImGui::Text("Free View Camera");
        ImGui::Separator();

        if (!targetScene) return;

        Camera& cam = targetScene->freeViewCamera;

        ImGui::TextDisabled("Position");
        ImGui::DragFloat3("##CamPos", glm::value_ptr(cam.Position), 0.1f);

        ImGui::TextDisabled("Rotation");
        ImGui::SliderFloat("Yaw##Cam",   &cam.Yaw,   -180.0f, 180.0f);
        ImGui::SliderFloat("Pitch##Cam", &cam.Pitch, -89.0f, 89.0f);

        ImGui::TextDisabled("Settings");
        ImGui::DragFloat("Movement Speed##Cam",    &cam.MovementSpeed,    0.1f, 0.1f, 50.0f);
        ImGui::DragFloat("Mouse Sensitivity##Cam", &cam.MouseSensitivity, 0.01f, 0.01f, 1.0f);
        ImGui::DragFloat("Zoom##Cam",              &cam.Zoom,             0.1f, 10.0f, 120.0f);

        // 即時顯示攝影機三軸向量（唯讀，由 updateCameraVectors 自動計算）
        ImGui::Spacing();
        ImGui::TextDisabled("Camera Vectors");
        ImGui::Text("Front: (%.2f, %.2f, %.2f)", cam.Front.x, cam.Front.y, cam.Front.z);
        ImGui::Text("Up: (%.2f, %.2f, %.2f)",    cam.Up.x,    cam.Up.y,    cam.Up.z);
        ImGui::Text("Right: (%.2f, %.2f, %.2f)", cam.Right.x, cam.Right.y, cam.Right.z);
    }

    // Transform 屬性面板：直接編輯選取物件的 local transform
    // DragFloat3 改變後需呼叫 MarkDirty() 使世界矩陣快取失效
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

        ImGui::TextDisabled("Position");
        if (ImGui::DragFloat3("##Position", glm::value_ptr(selectedObject->transform.position), 0.1f))
            selectedObject->MarkDirty();

        // 旋轉以尤拉角顯示（較直覺），但內部以四元數儲存
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
        if (ImGui::DragFloat3("##Scale", glm::value_ptr(selectedObject->transform.scale), 0.1f, 0.01f, 100.0f))
            selectedObject->MarkDirty();

        // 一鍵重設：清除所有 Transform 回初始狀態
        ImGui::Spacing();
        if (ImGui::Button("Reset Transform", ImVec2(-1, 0)))
        {
            selectedObject->transform.position = glm::vec3(0.0f);
            selectedObject->transform.rotation = glm::quat();
            selectedObject->transform.scale    = glm::vec3(1.0f);
            selectedObject->MarkDirty();
        }

        // 唯讀顯示 Local Model Matrix（輸出欄位，不可編輯）
        ImGui::Spacing();
        ImGui::TextDisabled("Local Model Matrix (Preview)");
        glm::mat4 model = selectedObject->transform.GetLocalMatrix();

        // glm 使用列主序，model[col][row]，此處依習慣以「列」排列顯示
        ImGui::Text("Row 0: (%.2f, %.2f, %.2f, %.2f)", model[0][0], model[0][1], model[0][2], model[0][3]);
        ImGui::Text("Row 1: (%.2f, %.2f, %.2f, %.2f)", model[1][0], model[1][1], model[1][2], model[1][3]);
        ImGui::Text("Row 2: (%.2f, %.2f, %.2f, %.2f)", model[2][0], model[2][1], model[2][2], model[2][3]);
        ImGui::Text("Row 3: (%.2f, %.2f, %.2f, %.2f)", model[3][0], model[3][1], model[3][2], model[3][3]);
    }

    // IK 屬性面板：列出所有 IK 鏈，提供啟用開關、目標設定與解算參數調整
    void InspectorWindow::DisplayIKPanel()
    {
        if (!targetScene) return;

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

            // 可折疊標題，右側顯示啟用/停用 Checkbox（AllowItemOverlap 使兩者共存同列）
            std::string headerLabel = chain.name + "##header" + std::to_string(idx);
            bool treeOpen = ImGui::TreeNodeEx(headerLabel.c_str(),
                ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth);

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.0f);
            std::string checkId = "##en" + std::to_string(idx);
            ImGui::Checkbox(checkId.c_str(), &chain.enabled);

            if (treeOpen)
            {
                ImGui::PushID(idx);

                // ── 目標座標（世界空間）───────────────────────────────────────
                ImGui::TextDisabled("Target (World)");
                ImGui::DragFloat3("##Target", glm::value_ptr(chain.target), 0.05f);

                // 快速將目標重設到末端效應器當前世界位置（joints.back 的 translation）
                if (ImGui::Button("Snap to End Effector"))
                {
                    if (!chain.joints.empty())
                        chain.target = glm::vec3(chain.joints.back()->GetWorldMatrix()[3]);
                }

                ImGui::Spacing();

                // ── FABRIK 解算參數 ───────────────────────────────────────────
                ImGui::TextDisabled("Solver Settings");
                ImGui::SliderInt("Max Iterations", &chain.maxIter,    1, 50);
                ImGui::DragFloat("Tolerance",      &chain.tolerance, 0.001f, 0.001f, 1.0f, "%.3f");

                // ── 鏈資訊（唯讀）───────────────────────────────────────────
                ImGui::Spacing();
                ImGui::TextDisabled("Chain Info");
                ImGui::Text("Joints : %d",    (int)chain.joints.size());
                ImGui::Text("Reach  : %.2f",  chain.totalLength);

                if (!chain.joints.empty())
                {
                    glm::vec3 endWorld     = glm::vec3(chain.joints.back()->GetWorldMatrix()[3]);
                    float     distToTarget = glm::length(chain.target - endWorld);
                    ImGui::Text("Dist to target: %.2f", distToTarget);
                }

                // 逐關節列表（末端效應器特別標記）
                ImGui::Spacing();
                ImGui::TextDisabled("Joint Chain");
                for (int j = 0; j < (int)chain.joints.size(); j++)
                {
                    bool isEnd = (j == (int)chain.joints.size() - 1);
                    ImGui::Text("  [%d] %s%s", j,
                        chain.joints[j]->objectName.c_str(),
                        isEnd ? "  <-- end effector" : "");
                }

                // 手動調整 T-pose 後，重新計算骨骼靜止長度
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
