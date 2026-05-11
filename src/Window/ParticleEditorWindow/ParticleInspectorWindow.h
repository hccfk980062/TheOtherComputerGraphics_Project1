#pragma once
#include <imgui.h>
#include "Scene/ParticleEditorScene.h"

namespace CG
{
    // 屬性面板：顯示並編輯選取 Emitter 的所有參數
    class ParticleInspectorWindow
    {
    public:
        void Display(ParticleEditorScene* scene);

    private:
        void DrawTypeSpecificProps(EmitterBase* e);
        void DrawEmitterMotionProps(EmitterBase* e);
        void DrawEmissionProps(EmitterBase* e);
        void DrawTimelineProps(EmitterBase* e);
        void DrawParticleProps(EmitterBase* e);
    };

} // namespace CG
