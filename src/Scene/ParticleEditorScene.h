#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "Camera/Camera.h"
#include "FrameBuffer/Framebuffer.h"
#include "Shader/Shader.h"
#include "ParticleEffects/EmitterBase.h"

namespace CG
{
    // Standalone particle editor scene: owns its own FBO, Camera, Shader pair,
    // root emitter tree, and timeline playback state.
    class ParticleEditorScene
    {
    public:
        ParticleEditorScene();
        ~ParticleEditorScene();

        bool Initialize(int width, int height);

        // Per-frame update: advances dt from ImGui::GetTime(), drives timeline and all emitters
        void Update();

        // Renders all emitters into the internal FBO
        void Render();

        // Called by ParticleViewportWindow when the viewport is resized
        void ResizeFBO(int w, int h);

        // ── Emitter tree ──────────────────────────────────────────────────────
        std::vector<std::unique_ptr<EmitterBase>> m_rootEmitters;
        EmitterBase* m_selectedEmitter = nullptr;

        void AddRootEmitter(std::unique_ptr<EmitterBase> emitter);
        void RemoveRootEmitter(EmitterBase* emitter);

        // ── Timeline playback state ───────────────────────────────────────────
        int  m_currentFrame = 0;
        int  m_startFrame   = 0;
        int  m_endFrame     = 240;
        bool m_isPlaying    = false;
        bool m_loopEnabled  = true;

        // ── Camera and FBO access ─────────────────────────────────────────────
        Camera       m_camera;
        Framebuffer* GetFramebuffer() { return m_framebuffer.get(); }

    private:
        std::unique_ptr<Framebuffer> m_framebuffer;
        std::unique_ptr<Shader>      m_billboardShader;
        std::unique_ptr<Shader>      m_trailShader;

        // Timeline clock (mirrors AnimationGroup::AdvancePlayback pattern)
        double m_lastTime        = 0.0;
        double m_timeAccumulated = 0.0;
    };
}
