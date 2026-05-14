#pragma once
#include <imgui.h>
#include "Scene/ParticleEditorScene.h"
#include "ParticleEffects/ConcreteEmitters.h"

namespace CG
{
    class ParticleHierarchyWindow
    {
    public:
        ParticleHierarchyWindow();
        ~ParticleHierarchyWindow();

        bool Initialize();
        void Display();

        void SetScene(ParticleEditorScene* scene) { m_scene = scene; }

    private:
        ParticleEditorScene* m_scene = nullptr;

        // Draws one emitter node and its children recursively
        // parent == nullptr means this is a root emitter
        void DrawEmitterNode(EmitterBase* emitter, EmitterBase* parent);

        // Right-click context menu for an emitter node
        void DrawContextMenu(EmitterBase* emitter, EmitterBase* parent);

        // Factory: creates a concrete emitter of the given type
        std::unique_ptr<EmitterBase> CreateEmitter(EmitterType type);

        // Removes child from parent->m_children, or from scene root if parent==nullptr
        void RemoveEmitter(EmitterBase* emitter, EmitterBase* parent);
    };
}
