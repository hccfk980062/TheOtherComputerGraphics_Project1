#include <algorithm>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Scene/ParticleEditorScene.h"

namespace CG
{
    ParticleEditorScene::ParticleEditorScene()
        : m_camera(glm::vec3(0.0f, 2.0f, 5.0f))
    {}

    ParticleEditorScene::~ParticleEditorScene() {}

    bool ParticleEditorScene::Initialize(int width, int height)
    {
        m_framebuffer    = std::make_unique<Framebuffer>(width, height);
        m_billboardShader= std::make_unique<Shader>("ShaderPrograms/Particle_vertex.vert",
                                                    "ShaderPrograms/Particle_fragement.frag");
        m_trailShader    = std::make_unique<Shader>("ShaderPrograms/Trail_vertex.vert",
                                                    "ShaderPrograms/Trail_fragment.frag");

        m_camera.SetProjectionMatrix(width, height);
        m_camera.configureLookAt(glm::vec3(0, -0.4f, -1.0f), glm::vec3(0, 1, 0));

        return true;
    }

    void ParticleEditorScene::ResizeFBO(int w, int h)
    {
        if (w > 0 && h > 0 && m_framebuffer)
        {
            m_framebuffer->ResizeFramebuffer(w, h);
            m_camera.SetProjectionMatrix(w, h);
        }
    }

    void ParticleEditorScene::AddRootEmitter(std::unique_ptr<EmitterBase> emitter)
    {
        m_rootEmitters.push_back(std::move(emitter));
    }

    void ParticleEditorScene::RemoveRootEmitter(EmitterBase* emitter)
    {
        m_rootEmitters.erase(
            std::remove_if(m_rootEmitters.begin(), m_rootEmitters.end(),
                [emitter](const std::unique_ptr<EmitterBase>& e) { return e.get() == emitter; }),
            m_rootEmitters.end());

        if (m_selectedEmitter == emitter)
            m_selectedEmitter = nullptr;
    }

    // ── Update ────────────────────────────────────────────────────────────────

    void ParticleEditorScene::Update()
    {
        double now = ImGui::GetTime();
        float  dt  = glm::clamp((float)(now - m_lastTime), 0.0f, 0.1f);
        m_timeAccumulated += now - m_lastTime;
        m_lastTime = now;

        // Advance timeline at ~66 fps (15 ms per frame), same as AnimationGroup pattern
        if (m_isPlaying)
        {
            while (m_timeAccumulated >= 0.015)
            {
                m_timeAccumulated -= 0.015;
                ++m_currentFrame;
                if (m_currentFrame > m_endFrame)
                {
                    if (m_loopEnabled)
                        m_currentFrame = m_startFrame;
                    else
                    {
                        m_currentFrame = m_endFrame;
                        m_isPlaying    = false;
                        break;
                    }
                }
            }
        }
        else
        {
            m_timeAccumulated = 0.0;
        }

        for (auto& e : m_rootEmitters)
            e->Update(dt, m_currentFrame, glm::vec3(0));
    }

    // ── Render ────────────────────────────────────────────────────────────────

    void ParticleEditorScene::Render()
    {
        if (!m_framebuffer) return;

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer->fbo);
        glViewport(0, 0, m_framebuffer->width, m_framebuffer->height);
        glClearColor(0.05f, 0.05f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        glm::mat4 view = m_camera.GetViewMatrix();
        glm::mat4 proj = m_camera.GetProjectionMatrix();
        glm::vec3 camPos = m_camera.Position;

        for (auto& e : m_rootEmitters)
            e->Draw(m_billboardShader.get(), m_trailShader.get(), view, proj, camPos);

        // Ensure blend state is clean after all draws
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
