#include "Scene/ParticleEditorScene.h"
#include <iostream>

namespace CG
{
    bool ParticleEditorScene::Initialize(int width, int height)
    {
        m_framebuffer = std::make_unique<Framebuffer>(width, height);

        m_particleShader = std::make_unique<Shader>(
            "ShaderPrograms/Particle_vertex.vert",
            "ShaderPrograms/Particle_fragement.frag");  // 注意：原檔名有 typo

        m_ribbonShader = std::make_unique<Shader>(
            "ShaderPrograms/Trail_vertex.vert",
            "ShaderPrograms/Trail_fragment.frag");

        // 設定攝影機初始位置，俯視粒子場景
        camera.Position = glm::vec3(0.0f, 3.0f, 8.0f);
        camera.SetProjectionMatrix(width, height);
        camera.configureLookAt(glm::normalize(glm::vec3(0, -0.3f, -1)), glm::vec3(0, 1, 0));

        m_lastTime = ImGui::GetTime();
        return true;
    }

    void ParticleEditorScene::Update()
    {
        double now = ImGui::GetTime();
        float  dt  = (float)(now - m_lastTime);
        m_lastTime = now;
        if (dt > 0.1f) dt = 0.1f;  // 限制最大跳幀，防止長時間暫停後爆炸

        if (isPlaying)
        {
            currentTime += dt;
            if (currentTime > timelineEnd)
                currentTime = 0.0f;  // 循環播放
        }

        for (auto& e : rootEmitters)
            e->Update(dt, currentTime);
    }

    void ParticleEditorScene::Render()
    {
        Framebuffer* fbo = m_framebuffer.get();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
        glViewport(0, 0, fbo->width, fbo->height);
        glClearColor(0.05f, 0.05f, 0.15f, 1.0f);  // 深藍背景，粒子效果更顯眼
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = camera.GetProjectionMatrix();

        for (auto& e : rootEmitters)
            e->Draw(m_particleShader.get(), m_ribbonShader.get(), view, proj);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    EmitterBase* ParticleEditorScene::AddEmitter(EmitterType type, EmitterBase* parent)
    {
        std::unique_ptr<EmitterBase> emitter;
        switch (type)
        {
            case EmitterType::Point:  emitter = std::make_unique<PointEmitter>();  break;
            case EmitterType::Sphere: emitter = std::make_unique<SphereEmitter>(); break;
            case EmitterType::Box:    emitter = std::make_unique<BoxEmitter>();    break;
            case EmitterType::Ring:   emitter = std::make_unique<RingEmitter>();   break;
            case EmitterType::Ribbon: emitter = std::make_unique<RibbonEmitter>(); break;
            default: return nullptr;
        }
        emitter->Initialize();
        emitter->parent = parent;

        EmitterBase* raw = emitter.get();
        if (parent)
            parent->children.push_back(std::move(emitter));
        else
            rootEmitters.push_back(std::move(emitter));

        return raw;
    }

    void ParticleEditorScene::RemoveEmitter(EmitterBase* target)
    {
        if (!target) return;

        // 若被刪除的 Emitter（或其後代）是選取對象，清除選取
        if (selectedEmitter == target || IsDescendant(selectedEmitter, target))
            selectedEmitter = nullptr;

        // 從 parent 的 children 或 rootEmitters 中移除
        auto removeFrom = [&](std::vector<std::unique_ptr<EmitterBase>>& list)
        {
            for (auto it = list.begin(); it != list.end(); ++it)
            {
                if (it->get() == target)
                {
                    list.erase(it);
                    return true;
                }
            }
            return false;
        };

        if (target->parent)
            removeFrom(target->parent->children);
        else
            removeFrom(rootEmitters);
    }

    void ParticleEditorScene::CollectAll(std::vector<EmitterBase*>& out) const
    {
        for (auto& e : rootEmitters)
            CollectRecursive(e.get(), out);
    }

    bool ParticleEditorScene::IsDescendant(EmitterBase* possible, EmitterBase* ancestor) const
    {
        if (!possible) return false;
        EmitterBase* curr = possible->parent;
        while (curr)
        {
            if (curr == ancestor) return true;
            curr = curr->parent;
        }
        return false;
    }

    void ParticleEditorScene::CollectRecursive(EmitterBase* e, std::vector<EmitterBase*>& out)
    {
        out.push_back(e);
        for (auto& child : e->children)
            CollectRecursive(child.get(), out);
    }

} // namespace CG
