#pragma once
#include <vector>
#include <memory>

#include <GL/glew.h>
#include <imgui.h>

#include "Camera/Camera.h"
#include "FrameBuffer/Framebuffer.h"
#include "Shader/Shader.h"
#include "ParticleEffects/ParticleEditorEmitter.h"

namespace CG
{
    // 粒子編輯器專用場景：持有 Emitter 樹、攝影機、FBO 與渲染著色器
    class ParticleEditorScene
    {
    public:
        // ── Emitter 層級 ───────────────────────────────────────────────────────
        std::vector<std::unique_ptr<EmitterBase>> rootEmitters;
        EmitterBase* selectedEmitter = nullptr;

        // ── 時間軸狀態 ────────────────────────────────────────────────────────
        float currentTime  = 0.0f;
        float timelineEnd  = 10.0f;
        bool  isPlaying    = false;

        // ── 攝影機（at global scope, not in CG namespace） ─────────────────────
        Camera camera;

        bool Initialize(int width, int height);
        void Update();   // 使用 ImGui::GetTime() 計算 dt，更新播放與所有 Emitter

        // 渲染所有 Emitter 至內部 FBO
        void Render();

        // ── Emitter 管理 ───────────────────────────────────────────────────────
        EmitterBase* AddEmitter(EmitterType type, EmitterBase* parent = nullptr);
        void         RemoveEmitter(EmitterBase* target);
        void         CollectAll(std::vector<EmitterBase*>& out) const;

        Framebuffer* GetFramebuffer() const { return m_framebuffer.get(); }

    private:
        std::unique_ptr<Framebuffer> m_framebuffer;
        std::unique_ptr<Shader>      m_particleShader;
        std::unique_ptr<Shader>      m_ribbonShader;

        double m_lastTime = 0.0;

        bool IsDescendant(EmitterBase* possible, EmitterBase* ancestor) const;
        static void CollectRecursive(EmitterBase* e, std::vector<EmitterBase*>& out);
    };

} // namespace CG
