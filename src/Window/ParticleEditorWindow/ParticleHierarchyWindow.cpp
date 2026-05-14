#include <string>
#include <algorithm>
#include "Window/ParticleEditorWindow/ParticleHierarchyWindow.h"

namespace CG
{
    ParticleHierarchyWindow::ParticleHierarchyWindow()  {}
    ParticleHierarchyWindow::~ParticleHierarchyWindow() {}

    bool ParticleHierarchyWindow::Initialize() { return true; }

    void ParticleHierarchyWindow::Display()
    {
        if (!ImGui::Begin("Particle Hierarchy"))
        {
            ImGui::End();
            return;
        }

        if (!m_scene)
        {
            ImGui::TextDisabled("(no scene)");
            ImGui::End();
            return;
        }

        // "Add Root Emitter" button with popup
        if (ImGui::Button("+ Add Root Emitter"))
            ImGui::OpenPopup("AddRootPopup");

        if (ImGui::BeginPopup("AddRootPopup"))
        {
            if (ImGui::MenuItem("Point"))  m_scene->AddRootEmitter(CreateEmitter(EmitterType::Point));
            if (ImGui::MenuItem("Sphere")) m_scene->AddRootEmitter(CreateEmitter(EmitterType::Sphere));
            if (ImGui::MenuItem("Box"))    m_scene->AddRootEmitter(CreateEmitter(EmitterType::Box));
            if (ImGui::MenuItem("Ring"))   m_scene->AddRootEmitter(CreateEmitter(EmitterType::Ring));
            ImGui::EndPopup();
        }

        ImGui::Separator();

        for (auto& e : m_scene->m_rootEmitters)
            DrawEmitterNode(e.get(), nullptr);

        ImGui::End();
    }

    void ParticleHierarchyWindow::DrawEmitterNode(EmitterBase* emitter, EmitterBase* parent)
    {
        bool hasChildren = !emitter->m_children.empty();

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        if (m_scene->m_selectedEmitter == emitter)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Show alive particle count as a hint
        std::string label = emitter->m_name + " [" + std::to_string(emitter->AliveCount()) + "]";
        bool nodeOpen = ImGui::TreeNodeEx((void*)emitter, flags, "%s", label.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            m_scene->m_selectedEmitter = emitter;

        DrawContextMenu(emitter, parent);

        if (hasChildren && nodeOpen)
        {
            for (auto& child : emitter->m_children)
                DrawEmitterNode(child.get(), emitter);
            ImGui::TreePop();
        }
    }

    void ParticleHierarchyWindow::DrawContextMenu(EmitterBase* emitter, EmitterBase* parent)
    {
        if (!ImGui::BeginPopupContextItem()) return;

        if (ImGui::BeginMenu("Add Child"))
        {
            if (ImGui::MenuItem("Point"))  emitter->m_children.push_back(CreateEmitter(EmitterType::Point));
            if (ImGui::MenuItem("Sphere")) emitter->m_children.push_back(CreateEmitter(EmitterType::Sphere));
            if (ImGui::MenuItem("Box"))    emitter->m_children.push_back(CreateEmitter(EmitterType::Box));
            if (ImGui::MenuItem("Ring"))   emitter->m_children.push_back(CreateEmitter(EmitterType::Ring));
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Remove"))
            RemoveEmitter(emitter, parent);

        ImGui::EndPopup();
    }

    void ParticleHierarchyWindow::RemoveEmitter(EmitterBase* emitter, EmitterBase* parent)
    {
        if (m_scene->m_selectedEmitter == emitter)
            m_scene->m_selectedEmitter = nullptr;

        if (parent == nullptr)
        {
            m_scene->RemoveRootEmitter(emitter);
        }
        else
        {
            auto& ch = parent->m_children;
            ch.erase(std::remove_if(ch.begin(), ch.end(),
                [emitter](const std::unique_ptr<EmitterBase>& e) { return e.get() == emitter; }),
                ch.end());
        }
    }

    std::unique_ptr<EmitterBase> ParticleHierarchyWindow::CreateEmitter(EmitterType type)
    {
        switch (type)
        {
        case EmitterType::Point:  return std::make_unique<PointEmitter>();
        case EmitterType::Sphere: return std::make_unique<SphereEmitter>();
        case EmitterType::Box:    return std::make_unique<BoxEmitter>();
        case EmitterType::Ring:   return std::make_unique<RingEmitter>();
        }
        return std::make_unique<PointEmitter>();
    }
}
