#include "HierarchyWindow.h"
#include <imgui.h>
#include "ParticleEffects/EmitterBase.h"

namespace CG
{
    HierarchyWindow::HierarchyWindow() : targetScene(nullptr) {}
    HierarchyWindow::~HierarchyWindow() {}

    auto HierarchyWindow::Initialize() -> bool { return true; }

    void HierarchyWindow::Display()
    {
        if (ImGui::Begin("Totally not Unity Hierarchy", nullptr))
        {
            if (targetScene)
            {
                // 右鍵點擊空白區域：跳出建立物件選單
                if (ImGui::BeginPopupContextWindow("##hierarchy_ctx",
                    ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
                {
                    if (ImGui::MenuItem("Create Empty"))
                        CreateObject();
                    ImGui::EndPopup();
                }

                // 遞迴繪製所有頂層節點（rootObject 的直接子節點）
                for (auto& child : targetScene->rootObject.children)
                {
                    if (child != nullptr)
                        DrawNode(child.get());
                }

                // 點擊視窗空白處取消選取
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
                    targetScene->selectedObject = nullptr;

                // ── 未附加的粒子發射器區段 ────────────────────────────────────
                bool hasUnattached = false;
                for (auto& e : targetScene->m_particleEmitters)
                    if (!e->m_parentSceneObject) { hasUnattached = true; break; }

                if (hasUnattached)
                {
                    ImGui::Separator();
                    ImGui::TextDisabled("Particle Emitters");
                    for (auto& e : targetScene->m_particleEmitters)
                    {
                        if (!e->m_parentSceneObject)
                            DrawEmitterNode(e.get());
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Scene Assigned!");
            }
        }
        ImGui::End();
    }

    // 遞迴繪製一個節點及其所有子節點
    // 葉節點不顯示展開箭頭，選取中節點以高亮顯示
    void HierarchyWindow::DrawNode(SceneObject* obj)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        // 葉節點（無子節點且無附加 emitter）不顯示展開圖示且不在樹狀堆疊中
        if (obj->children.empty() && obj->m_emitters.empty())
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (obj == targetScene->selectedObject)
            flags |= ImGuiTreeNodeFlags_Selected;

        // 以物件 id 作為 ImGui 節點識別碼，避免同名物件衝突
        bool opened = ImGui::TreeNodeEx((void*)(intptr_t)obj->id, flags, "%s", obj->objectName.c_str());

        // 點擊節點（非展開/收合箭頭）時選取此物件
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            targetScene->selectedObject = obj;

        DrawContextMenu(obj);

        // 拖曳來源：攜帶 SceneObject* 指標作為 payload
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("HIERARCHY_NODE", &obj, sizeof(SceneObject*));
            ImGui::Text("Moving: %s", obj->objectName.c_str());
            ImGui::EndDragDropSource();
        }

        // 放置目標：接收 SceneObject 拖曳（重設父節點）或 Emitter 拖曳（附加到此物件）
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_NODE"))
            {
                SceneObject* dragged = *(SceneObject**)payload->Data;
                if (dragged != obj && !IsAncestor(dragged, obj))
                    ReparentObject(dragged, obj);
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EMITTER_NODE"))
            {
                EmitterBase* emitter = *(EmitterBase**)payload->Data;
                targetScene->AttachEmitter(emitter, obj);
            }
            ImGui::EndDragDropTarget();
        }

        // 有子節點（或附加 emitter）且已展開時，遞迴繪製子節點與 emitter 葉節點
        if (opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        {
            for (auto& child : obj->children)
                DrawNode(child.get());
            for (EmitterBase* emitter : obj->m_emitters)
                DrawEmitterNode(emitter);
            ImGui::TreePop();
        }
    }

    // 右鍵選單：在每個節點上提供「建立子物件」與「刪除」選項
    void HierarchyWindow::DrawContextMenu(SceneObject* obj)
    {
        ImGui::PushID(obj->id);
        if (ImGui::BeginPopupContextItem("##node_ctx"))
        {
            if (ImGui::MenuItem("Create Child"))
                CreateObject();
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                DeleteObject();
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    // 繪製附加在 SceneObject 下的 emitter 葉節點
    // 支援拖曳來源（EMITTER_NODE payload）與右鍵解除附加
    void HierarchyWindow::DrawEmitterNode(EmitterBase* emitter)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        std::string label = "[P] " + emitter->m_name;
        ImGui::TreeNodeEx((void*)(uintptr_t)emitter, flags, "%s", label.c_str());

        // 右鍵選單：解除附加
        ImGui::PushID(emitter);
        if (ImGui::BeginPopupContextItem("##emitter_ctx"))
        {
            if (ImGui::MenuItem("Detach from Object"))
                targetScene->DetachEmitter(emitter);
            ImGui::EndPopup();
        }
        ImGui::PopID();

        // 拖曳來源：攜帶 EmitterBase* 指標作為 payload
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("EMITTER_NODE", &emitter, sizeof(EmitterBase*));
            ImGui::Text("[P] %s", emitter->m_name.c_str());
            ImGui::EndDragDropSource();
        }
    }

    void HierarchyWindow::CreateObject()
    {
        std::cout << "Not Implemented yet";
    }

    void HierarchyWindow::DeleteObject()
    {
        std::cout << "Not Implemented yet";
    }

    void HierarchyWindow::ReparentObject(SceneObject* obj, SceneObject* newParent)
    {
        targetScene->ReparentObject(obj, newParent);
    }

    // 向上遍歷 node 的 parent 鏈，若遇到 potentialAncestor 則回傳 true
    // 防止拖放操作造成場景樹的循環引用
    bool HierarchyWindow::IsAncestor(SceneObject* potentialAncestor, SceneObject* node)
    {
        SceneObject* cur = node->parent;
        while (cur)
        {
            if (cur == potentialAncestor) return true;
            cur = cur->parent;
        }
        return false;
    }
}
