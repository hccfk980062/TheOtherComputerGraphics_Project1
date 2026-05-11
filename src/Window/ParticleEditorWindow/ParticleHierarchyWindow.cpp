#include "Window/ParticleEditorWindow/ParticleHierarchyWindow.h"

namespace CG
{
    static void OpenAddEmitterPopup(const char* popupId, ParticleEditorScene* scene, EmitterBase* parent)
    {
        if (ImGui::BeginPopup(popupId))
        {
            if (ImGui::MenuItem("Point"))  scene->AddEmitter(EmitterType::Point,  parent);
            if (ImGui::MenuItem("Sphere")) scene->AddEmitter(EmitterType::Sphere, parent);
            if (ImGui::MenuItem("Box"))    scene->AddEmitter(EmitterType::Box,    parent);
            if (ImGui::MenuItem("Ring"))   scene->AddEmitter(EmitterType::Ring,   parent);
            if (ImGui::MenuItem("Ribbon")) scene->AddEmitter(EmitterType::Ribbon, parent);
            ImGui::EndPopup();
        }
    }

    void ParticleHierarchyWindow::Display(ParticleEditorScene* scene)
    {
        ImGui::Text("Hierarchy");
        ImGui::SameLine();
        if (ImGui::Button("+ Root"))
            ImGui::OpenPopup("##pe_add_root");
        OpenAddEmitterPopup("##pe_add_root", scene, nullptr);

        ImGui::Separator();

        m_pendingDelete = nullptr;

        for (auto& emitter : scene->rootEmitters)
            DrawEmitterNode(emitter.get(), scene);

        // 延遲刪除，確保不在 TreeNode 迴圈中修改容器
        if (m_pendingDelete)
            scene->RemoveEmitter(m_pendingDelete);
    }

    void ParticleHierarchyWindow::DrawEmitterNode(EmitterBase* emitter, ParticleEditorScene* scene)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow    |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_DefaultOpen;

        if (emitter->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;
        if (scene->selectedEmitter == emitter)
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::PushID(emitter);

        bool nodeOpen = ImGui::TreeNodeEx(emitter->name.c_str(), flags);

        if (ImGui::IsItemClicked())
            scene->selectedEmitter = emitter;

        // 右鍵選單：新增子 Emitter 或刪除
        char ctxId[32];
        snprintf(ctxId, sizeof(ctxId), "##ctx_%p", (void*)emitter);
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::BeginMenu("Add Child"))
            {
                if (ImGui::MenuItem("Point"))  scene->AddEmitter(EmitterType::Point,  emitter);
                if (ImGui::MenuItem("Sphere")) scene->AddEmitter(EmitterType::Sphere, emitter);
                if (ImGui::MenuItem("Box"))    scene->AddEmitter(EmitterType::Box,    emitter);
                if (ImGui::MenuItem("Ring"))   scene->AddEmitter(EmitterType::Ring,   emitter);
                if (ImGui::MenuItem("Ribbon")) scene->AddEmitter(EmitterType::Ribbon, emitter);
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Delete"))
                m_pendingDelete = emitter;

            ImGui::EndPopup();
        }

        if (nodeOpen)
        {
            // 避免刪除後繼續遍歷已失效指標
            for (auto& child : emitter->children)
            {
                if (m_pendingDelete) break;
                DrawEmitterNode(child.get(), scene);
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

} // namespace CG
