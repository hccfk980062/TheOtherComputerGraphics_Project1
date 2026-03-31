#include "HierarchyWindow.h"
#include<imgui.h>
namespace CG
{
	HierarchyWindow::HierarchyWindow(): targetScene(nullptr)
	{
	}

	HierarchyWindow::~HierarchyWindow()
	{
	}

	auto HierarchyWindow::Initialize() -> bool
	{
		return true;
	}

	void HierarchyWindow::Display()
	{
		if (ImGui::Begin("Totally not Unity Hierarchy", nullptr))
		{
			if (targetScene)
			{
				// Right-click on blank area → create root object
				if (ImGui::BeginPopupContextWindow("##hierarchy_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
					if (ImGui::MenuItem("Create Empty"))
						CreateObject();
					ImGui::EndPopup();
				}

				// Draw all top-level objects
                for (auto& child : targetScene->rootObject.children)
                {
                    if (child != nullptr)
                    {
                        DrawNode(child.get());
                    }
                }

				// Click on empty space to deselect
				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
					targetScene->selectedObject = nullptr;
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Scene Assigned!");
			}
		}
		ImGui::End();
	}

    void HierarchyWindow::DrawNode(SceneObject* obj) 
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (obj->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (obj == targetScene->selectedObject)
            flags |= ImGuiTreeNodeFlags_Selected;

        bool opened = ImGui::TreeNodeEx((void*)(intptr_t)obj->id, flags, "%s", obj->objectName.c_str());

        // Selection
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            targetScene->selectedObject = obj;

        // Right-click context menu per node
        DrawContextMenu(obj);

        // Drag source
        if (ImGui::BeginDragDropSource()) 
        {
            ImGui::SetDragDropPayload("HIERARCHY_NODE", &obj, sizeof(SceneObject*));
            ImGui::Text("Moving: %s", obj->objectName.c_str());
            ImGui::EndDragDropSource();
        }

        // Drop target — reparent onto this node
        if (ImGui::BeginDragDropTarget()) 
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_NODE")) {
                SceneObject* dragged = *(SceneObject**)payload->Data;
                if (dragged != obj && !IsAncestor(dragged, obj))
                    ReparentObject(dragged, obj);
            }
            ImGui::EndDragDropTarget();
        }

        if (opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) 
        {
            for (auto& child : obj->children)
                DrawNode(child.get());
            ImGui::TreePop();
        }
    }
    void HierarchyWindow::DrawContextMenu(SceneObject* obj) {
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
    bool HierarchyWindow::IsAncestor(SceneObject* potentialAncestor, SceneObject* node) {
        SceneObject* cur = node->parent;
        while (cur) {
            if (cur == potentialAncestor) return true;
            cur = cur->parent;
        }
        return false;
    }
}
