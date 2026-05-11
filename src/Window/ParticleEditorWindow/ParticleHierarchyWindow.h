#pragma once
#include <imgui.h>
#include "Scene/ParticleEditorScene.h"

namespace CG
{
    // Emitter 層級樹視窗：列出所有 Emitter 並支援新增/刪除/選取操作
    class ParticleHierarchyWindow
    {
    public:
        void Display(ParticleEditorScene* scene);

    private:
        void DrawEmitterNode(EmitterBase* emitter, ParticleEditorScene* scene);
        EmitterBase* m_pendingDelete = nullptr;  // 延遲刪除，避免在迴圈中修改容器
    };

} // namespace CG
