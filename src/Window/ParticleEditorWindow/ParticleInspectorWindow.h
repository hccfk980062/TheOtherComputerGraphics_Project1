#pragma once
#include <imgui.h>
#include "Scene/ParticleEditorScene.h"
#include "ParticleEffects/ConcreteEmitters.h"

namespace CG
{
    class ParticleInspectorWindow
    {
    public:
        ParticleInspectorWindow();
        ~ParticleInspectorWindow();

        bool Initialize();
        void Display();

        void SetScene(ParticleEditorScene* scene) { m_scene = scene; }

    private:
        ParticleEditorScene* m_scene = nullptr;

        // Panels drawn when an emitter is selected
        void DisplayEmitterPanel(EmitterBase* emitter);
        void DisplayShapePanel(EmitterBase* emitter);
        void DisplaySpawnPropsPanel(EmitterSpawnProps& props);
    };
}
