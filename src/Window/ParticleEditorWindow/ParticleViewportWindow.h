#pragma once
#include <imgui.h>
#include "Scene/ParticleEditorScene.h"

namespace CG
{
    // 粒子編輯器的 3D 視口：同步 FBO 尺寸、渲染場景並顯示結果
    class ParticleViewportWindow
    {
    public:
        void Display(ParticleEditorScene* scene);

    private:
        void SyncFBOSize(ParticleEditorScene* scene, int w, int h);
        void HandleCameraInput(ParticleEditorScene* scene);

        bool  m_rightMouseDown = false;
        float m_lastMouseX     = 0.0f;
        float m_lastMouseY     = 0.0f;
    };

} // namespace CG
