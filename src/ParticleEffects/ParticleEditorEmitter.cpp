#include "ParticleEffects/ParticleEditorEmitter.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstdlib>
#include <cmath>

namespace CG
{
    static constexpr float PI = 3.14159265f;

    static const float s_quadVertices[8] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f,
    };

    // ── EmitterBase ──────────────────────────────────────────────────────────

    EmitterBase::~EmitterBase()
    {
        if (m_VAO)         glDeleteVertexArrays(1, &m_VAO);
        if (m_quadVBO)     glDeleteBuffers(1, &m_quadVBO);
        if (m_instanceVBO) glDeleteBuffers(1, &m_instanceVBO);
    }

    void EmitterBase::Initialize()
    {
        // RibbonEmitter 使用 TrailRenderer，不需要 billboard 粒子緩衝
        if (type == EmitterType::Ribbon) return;

        m_particles.assign(maxParticles, ParticleInstance{});

        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        // attrib 0：靜態 quad 頂點（vec2），所有粒子共用
        glGenBuffers(1, &m_quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(s_quadVertices), s_quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        // per-instance：pos(3) + color(4) + size(1) = 8 floats
        glGenBuffers(1, &m_instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * 8 * sizeof(float), nullptr, GL_STREAM_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glVertexAttribDivisor(1, 1);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribDivisor(2, 1);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
        glVertexAttribDivisor(3, 1);

        glBindVertexArray(0);
    }

    glm::vec3 EmitterBase::GetWorldPosition() const
    {
        if (parent)
        {
            glm::mat4 parentMat = glm::translate(glm::mat4(1.0f), parent->GetWorldPosition())
                                * glm::mat4_cast(parent->rotation);
            return glm::vec3(parentMat * glm::vec4(position, 1.0f));
        }
        return position;
    }

    void EmitterBase::Update(float dt, float currentTime)
    {
        // ── Emitter 自身運動 ──────────────────────────────────────────────────
        position += velocity * dt;
        velocity += acceleration * dt;
        if (glm::length(angularVelocity) > 0.0001f)
        {
            float angle = glm::length(angularVelocity) * dt;
            rotation = glm::normalize(glm::angleAxis(angle, glm::normalize(angularVelocity)) * rotation);
        }

        // ── 發射粒子（在活躍時間段內）────────────────────────────────────────
        bool active = (currentTime >= startTime && currentTime <= endTime);
        if (active)
        {
            m_emitTimer += dt;
            while (m_emitTimer >= emitInterval)
            {
                m_emitTimer -= emitInterval;
                Emit();
            }
        }

        // ── 更新所有存活粒子 ──────────────────────────────────────────────────
        for (auto& p : m_particles)
        {
            if (!p.alive) continue;
            p.life -= dt;
            if (p.life <= 0.0f) { p.alive = false; continue; }
            p.position += p.velocity * dt;
            p.velocity += p.acceleration * dt;
            p.color.a = p.life / p.maxLife;
        }

        // ── 更新子 Emitter ────────────────────────────────────────────────────
        for (auto& child : children)
            child->Update(dt, currentTime);
    }

    void EmitterBase::Draw(Shader* particleShader, Shader* ribbonShader,
                            const glm::mat4& view, const glm::mat4& projection)
    {
        if (m_VAO == 0) { // 非 billboard 類型（如 Ribbon）不走此路徑
            for (auto& child : children)
                child->Draw(particleShader, ribbonShader, view, projection);
            return;
        }

        std::vector<float> data;
        data.reserve(maxParticles * 8);
        int aliveCount = 0;
        for (auto& p : m_particles)
        {
            if (!p.alive) continue;
            data.push_back(p.position.x); data.push_back(p.position.y); data.push_back(p.position.z);
            data.push_back(p.color.r);    data.push_back(p.color.g);    data.push_back(p.color.b);
            data.push_back(p.color.a);
            data.push_back(p.size);
            ++aliveCount;
        }

        if (aliveCount > 0)
        {
            particleShader->use();
            particleShader->setUnifMat4("view", view);
            particleShader->setUnifMat4("projection", projection);

            glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), nullptr, GL_STREAM_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);

            glBindVertexArray(m_VAO);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, aliveCount);
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        for (auto& child : children)
            child->Draw(particleShader, ribbonShader, view, projection);
    }

    void EmitterBase::Emit()
    {
        glm::mat4 rotMat = glm::mat4_cast(rotation);
        glm::vec3 worldPos = GetWorldPosition();

        for (int i = 0; i < emitCount; ++i)
        {
            int idx = FirstDeadSlot();
            if (idx == -1) continue;

            auto& p = m_particles[idx];
            glm::vec3 offset = SampleSpawnOffset();
            glm::vec3 rotatedOffset = glm::vec3(rotMat * glm::vec4(offset, 0.0f));

            p.position = worldPos + rotatedOffset;
            // offset 傳給 ComputeParticleVelocity，讓子類可依生成位置決定初速度方向
            p.velocity = ComputeParticleVelocity(offset) + glm::vec3(
                ((rand() % 2000) / 1000.0f - 1.0f) * 0.1f,
                ((rand() % 2000) / 1000.0f - 1.0f) * 0.1f,
                ((rand() % 2000) / 1000.0f - 1.0f) * 0.1f);
            p.acceleration = particleAcceleration;
            p.life    = particleLife;
            p.maxLife = particleLife;
            p.size    = particleSize;
            p.color   = particleColor;
            p.alive   = true;
        }
    }

    int EmitterBase::FirstDeadSlot()
    {
        for (int i = 0; i < (int)m_particles.size(); ++i)
            if (!m_particles[i].alive) return i;
        return -1;
    }

    // ── SphereEmitter ────────────────────────────────────────────────────────

    glm::vec3 SphereEmitter::SampleSpawnOffset()
    {
        float theta = ((rand() % 1000) / 1000.0f) * 2.0f * PI;
        float phi   = acosf(1.0f - 2.0f * ((rand() % 1000) / 1000.0f));
        return glm::vec3(
            radius * sinf(phi) * cosf(theta),
            radius * sinf(phi) * sinf(theta),
            radius * cosf(phi));
    }

    glm::vec3 SphereEmitter::ComputeParticleVelocity(const glm::vec3& spawnOffset)
    {
        if (convergeToCenter && glm::length(spawnOffset) > 0.0001f)
            return glm::normalize(-spawnOffset) * convergeSpeed;  // 朝球心方向
        return particleInitVelocity;
    }

    // ── BoxEmitter ───────────────────────────────────────────────────────────

    glm::vec3 BoxEmitter::SampleSpawnOffset()
    {
        return glm::vec3(
            ((rand() % 2000) / 1000.0f - 1.0f) * extents.x * 0.5f,
            ((rand() % 2000) / 1000.0f - 1.0f) * extents.y * 0.5f,
            ((rand() % 2000) / 1000.0f - 1.0f) * extents.z * 0.5f);
    }

    // ── RingEmitter ──────────────────────────────────────────────────────────

    glm::vec3 RingEmitter::SampleSpawnOffset()
    {
        float angle = ((rand() % 1000) / 1000.0f) * 2.0f * PI;
        float r     = radius + ((rand() % 1000) / 1000.0f - 0.5f) * width;
        return glm::vec3(r * cosf(angle), 0.0f, r * sinf(angle));
    }

    // ── RibbonEmitter ────────────────────────────────────────────────────────

    RibbonEmitter::RibbonEmitter()
    {
        type = EmitterType::Ribbon;
        name = "RibbonEmitter";
        m_trail.color    = ribbonColor;
        m_trail.duration = 1.0f;
        m_trail.maxPoints = 128;
    }

    void RibbonEmitter::Update(float dt, float currentTime)
    {
        // Emitter 自身運動
        position += velocity * dt;
        velocity += acceleration * dt;
        if (glm::length(angularVelocity) > 0.0001f)
        {
            float angle = glm::length(angularVelocity) * dt;
            rotation = glm::normalize(glm::angleAxis(angle, glm::normalize(angularVelocity)) * rotation);
        }

        if (currentTime >= startTime && currentTime <= endTime)
        {
            glm::vec3 worldPos = GetWorldPosition();
            // 以速度方向決定 ribbon 平面，計算兩側邊緣點
            glm::vec3 dir = glm::length(velocity) > 0.001f ? glm::normalize(velocity) : glm::vec3(0, 1, 0);
            glm::vec3 up  = glm::vec3(0, 1, 0);
            if (std::abs(glm::dot(dir, up)) > 0.99f) up = glm::vec3(1, 0, 0);
            glm::vec3 right = glm::normalize(glm::cross(dir, up));

            m_trail.color = ribbonColor;
            m_trail.update(worldPos + right * (ribbonWidth * 0.5f),
                           worldPos - right * (ribbonWidth * 0.5f),
                           currentTime);
        }

        for (auto& child : children)
            child->Update(dt, currentTime);
    }

    void RibbonEmitter::Draw(Shader* particleShader, Shader* ribbonShader,
                              const glm::mat4& view, const glm::mat4& projection)
    {
        ribbonShader->use();
        ribbonShader->setUnifMat4("view", view);
        ribbonShader->setUnifMat4("projection", projection);
        m_trail.color = ribbonColor;
        m_trail.Draw(ribbonShader);

        for (auto& child : children)
            child->Draw(particleShader, ribbonShader, view, projection);
    }

} // namespace CG
