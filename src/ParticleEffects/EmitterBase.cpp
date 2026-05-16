#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

#include "ParticleEffects/EmitterBase.h"

namespace CG
{
    // ── Helpers ───────────────────────────────────────────────────────────────

    static float randF(float base, float amp, std::mt19937& rng)
    {
        if (amp <= 0.0f) return base;
        std::uniform_real_distribution<float> d(-amp, amp);
        return base + d(rng);
    }

    static glm::vec3 randV3(glm::vec3 base, glm::vec3 amp, std::mt19937& rng)
    {
        return {
            randF(base.x, amp.x, rng),
            randF(base.y, amp.y, rng),
            randF(base.z, amp.z, rng)
        };
    }

    // ── EmitterBase ───────────────────────────────────────────────────────────

    EmitterBase::EmitterBase()
        : m_rng(std::random_device{}())
    {}

    EmitterBase::~EmitterBase()
    {
        if (m_gpuInitialized)
        {
            glDeleteVertexArrays(1, &m_quadVAO);
            glDeleteBuffers(1, &m_quadVBO);
            glDeleteBuffers(1, &m_instancePosVBO);
            glDeleteBuffers(1, &m_instanceColorVBO);
            glDeleteBuffers(1, &m_instanceSizeVBO);
            glDeleteVertexArrays(1, &m_ribbonVAO);
            glDeleteBuffers(1, &m_ribbonVBO);
        }
    }

    void EmitterBase::CopyBaseTo(EmitterBase& dst) const
    {
        dst.m_name         = m_name;
        dst.m_type         = m_type;
        dst.m_open         = m_open;
        dst.m_spawnCount   = m_spawnCount;
        dst.m_maxParticles = m_maxParticles;
        dst.m_spawnInterval= m_spawnInterval;
        dst.m_startFrame   = m_startFrame;
        dst.m_endFrame     = m_endFrame;
        dst.m_spawnProps   = m_spawnProps;
        // Recursively clone child templates
        for (const auto& child : m_children)
            dst.m_children.push_back(child->Clone());
        // GPU resources are NOT copied; dst.m_gpuInitialized stays false
    }

    int EmitterBase::AliveCount() const
    {
        int n = 0;
        for (const auto& p : m_particles)
            if (p->alive) ++n;
        return n;
    }

    // ── Update ────────────────────────────────────────────────────────────────

    void EmitterBase::Update(float dt, int timelineFrame, glm::vec3 basePos)
    {
        // 1. Advance existing live particles
        for (auto& p : m_particles)
        {
            if (!p->alive) continue;

            p->age += dt;
            if (p->age >= p->lifetime)
            {
                p->alive = false;
                p->childInstance.reset();
                p->trailHistory.clear();
                continue;
            }

            UpdateParticlePhysics(*p, dt);

            if (m_spawnProps.particleType == ParticleType::Trail)
            {
                p->trailHistory.insert(p->trailHistory.begin(), p->pos);
                if ((int)p->trailHistory.size() > p->maxTrailLength)
                    p->trailHistory.resize(p->maxTrailLength);
            }

            // Update child instance co-located at this particle's world position
            if (p->childInstance)
                p->childInstance->Update(dt, timelineFrame, p->pos);
        }

        // 2. Emit new particles only within active timeline range
        if (timelineFrame >= m_startFrame && timelineFrame <= m_endFrame)
        {
            m_spawnAccumulator += dt;
            if(m_spawnInterval < 0.001) 
                m_spawnInterval = 0.001;

            while (m_spawnAccumulator >= m_spawnInterval)
            {
                m_spawnAccumulator -= m_spawnInterval;
                EmitBurst(m_spawnCount, basePos);
            }
        }
    }

    void EmitterBase::EmitBurst(int count, glm::vec3 basePos)
    {
        int spawned = 0;
        for (auto& p : m_particles)
        {
            if (spawned >= count) break;
            if (!p->alive)
            {
                ActivateParticle(*p, basePos);
                ++spawned;
            }
        }
        // Grow pool if still below max
        while (spawned < count && (int)m_particles.size() < m_maxParticles)
        {
            auto p = std::make_unique<Particle>();
            ActivateParticle(*p, basePos);
            m_particles.push_back(std::move(p));
            ++spawned;
        }
    }

    void EmitterBase::ActivateParticle(Particle& p, glm::vec3 basePos)
    {
        const auto& sp = m_spawnProps;

        p.alive    = true;
        p.age      = 0.0f;
        p.lifetime = randF(sp.lifetime, sp.lifetimeRand, m_rng);
        if (p.lifetime <= 0.0f) p.lifetime = 0.001f;

        p.pos   = basePos + SampleSpawnOffset(m_rng) + randV3(sp.initPos, sp.initPosRand, m_rng);
        p.vel   = randV3(sp.initVel, sp.initVelRand, m_rng);
        p.accel = randV3(sp.initAccel, sp.initAccelRand, m_rng);

        p.rot      = randF(sp.initRot,      sp.initRotRand,      m_rng);
        p.rotVel   = randF(sp.initRotVel,   sp.initRotVelRand,   m_rng);
        p.rotAccel = randF(sp.initRotAccel, sp.initRotAccelRand, m_rng);

        p.scale      = randF(sp.initScale,      sp.initScaleRand,      m_rng);
        p.scaleVel   = randF(sp.initScaleVel,   sp.initScaleVelRand,   m_rng);
        p.scaleAccel = randF(sp.initScaleAccel, sp.initScaleAccelRand, m_rng);

        p.color          = sp.color;
        p.textureID      = sp.textureID;
        p.maxTrailLength = sp.maxTrailLength;
        p.trailHistory.clear();

        // Clone child template into this particle instance
        if (!m_children.empty())
            p.childInstance = m_children[0]->Clone();
        else
            p.childInstance.reset();
    }

    void EmitterBase::UpdateParticlePhysics(Particle& p, float dt)
    {
        // Semi-implicit Euler: velocity first, then position
        p.vel   += p.accel * dt;
        p.pos   += p.vel   * dt;

        p.rotVel   += p.rotAccel   * dt;
        p.rot      += p.rotVel     * dt;

        p.scaleVel   += p.scaleAccel * dt;
        p.scale      += p.scaleVel   * dt;
        if (p.scale < 0.0f) p.scale = 0.0f;
    }

    // ── GPU Initialization ────────────────────────────────────────────────────

    void EmitterBase::InitGPUResources()
    {
        // Static quad geometry: two triangles covering [-0.5, 0.5]² in XY
        static const float quadVerts[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f,
            -0.5f, -0.5f
        };

        // ── Sprite VAO ─────────────────────────────────────────────────────
        glGenVertexArrays(1, &m_quadVAO);
        glGenBuffers(1, &m_quadVBO);
        glGenBuffers(1, &m_instancePosVBO);
        glGenBuffers(1, &m_instanceColorVBO);
        glGenBuffers(1, &m_instanceSizeVBO);

        glBindVertexArray(m_quadVAO);

        // Location 0: aQuadPos (vec2, per-vertex, not instanced)
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Location 1: instancePos (vec3, per-instance)
        glBindBuffer(GL_ARRAY_BUFFER, m_instancePosVBO);
        glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(glm::vec3), nullptr, GL_STREAM_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribDivisor(1, 1);

        // Location 2: instanceColor (vec4, per-instance)
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceColorVBO);
        glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(glm::vec4), nullptr, GL_STREAM_DRAW);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(2, 1);

        // Location 3: instanceSize (float, per-instance)
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceSizeVBO);
        glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(float), nullptr, GL_STREAM_DRAW);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);

        glBindVertexArray(0);

        // ── Ribbon/Ring VAO (shared for Trail + Ring) ──────────────────────
        // Vertex layout: pos(3) + alpha(1) + uv(2) = 6 floats, 24 bytes
        glGenVertexArrays(1, &m_ribbonVAO);
        glGenBuffers(1, &m_ribbonVBO);

        glBindVertexArray(m_ribbonVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_ribbonVBO);
        // Allocate a small initial buffer; will be replaced by glBufferData each frame
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

        const int stride = 6 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        m_gpuInitialized = true;
    }

    // ── Draw ─────────────────────────────────────────────────────────────────

    void EmitterBase::Draw(Shader* billboardShader, Shader* trailShader,
                           const glm::mat4& view, const glm::mat4& proj,
                           glm::vec3 cameraPos)
    {
        if (!m_gpuInitialized) InitGPUResources();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        switch (m_spawnProps.particleType)
        {
        case ParticleType::Sprite: DrawSprites(billboardShader, view, proj);            break;
        case ParticleType::Trail:  DrawTrails(trailShader,      view, proj, cameraPos); break;
        case ParticleType::Ring:   DrawRings(trailShader,       view, proj);            break;
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // Recurse: draw child emitter instances living inside each particle
        for (auto& p : m_particles)
            if (p->alive && p->childInstance)
                p->childInstance->Draw(billboardShader, trailShader, view, proj, cameraPos);
    }

    // ── Sprite rendering ──────────────────────────────────────────────────────

    void EmitterBase::DrawSprites(Shader* shader, const glm::mat4& view, const glm::mat4& proj)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> colors;
        std::vector<float>     sizes;

        for (const auto& p : m_particles)
        {
            if (!p->alive) continue;
            // Fade alpha toward end of lifetime
            float t = 1.0f - p->age / p->lifetime;
            glm::vec4 c = p->color;
            c.a *= t;
            positions.push_back(p->pos);
            colors.push_back(c);
            sizes.push_back(p->scale);
        }

        if (positions.empty()) return;
        int count = (int)positions.size();

        glBindVertexArray(m_quadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_instancePosVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::vec3), positions.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_instanceColorVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_instanceSizeVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), sizes.data(), GL_STREAM_DRAW);

        shader->use();
        shader->setUnifMat4("view", view);
        shader->setUnifMat4("projection", proj);

        if (m_spawnProps.textureID != 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_spawnProps.textureID);
            shader->setUnifInt("uTexture", 0);
            shader->setUnifInt("uUseTexture", 1);
        }
        else
        {
            shader->setUnifInt("uUseTexture", 0);
        }

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
        glBindVertexArray(0);
        shader->unUse();
    }

    // ── Trail rendering ───────────────────────────────────────────────────────

    void EmitterBase::DrawTrails(Shader* shader, const glm::mat4& view, const glm::mat4& proj,
                                  glm::vec3 camPos)
    {
        shader->use();
        shader->setUnifMat4("view", view);
        shader->setUnifMat4("projection", proj);

        float halfWidth = m_spawnProps.trailWidth * 0.5f;

        std::vector<float> verts;  // 6 floats per vertex

        for (const auto& p : m_particles)
        {
            if (!p->alive || p->trailHistory.size() < 2) continue;

            const auto& hist = p->trailHistory;
            int N = (int)hist.size();
            verts.clear();
            verts.reserve(N * 2 * 6);

            float w = halfWidth * p->scale;
            float overallAlpha = 1.0f - p->age / p->lifetime;

            for (int i = 0; i < N; i++)
            {
                glm::vec3 tangent;
                if      (i == 0)    tangent = hist[0] - hist[1];
                else if (i == N-1)  tangent = hist[N-2] - hist[N-1];
                else                tangent = hist[i-1] - hist[i+1];

                float tLen = glm::length(tangent);
                if (tLen < 1e-6f) tangent = glm::vec3(1, 0, 0);
                else              tangent /= tLen;

                glm::vec3 toCam = camPos - hist[i];
                float camDist = glm::length(toCam);
                if (camDist < 1e-6f) toCam = glm::vec3(0, 1, 0);
                else                 toCam /= camDist;

                glm::vec3 side = glm::cross(tangent, toCam);
                float sideLen = glm::length(side);
                if (sideLen < 1e-6f) side = glm::vec3(1, 0, 0);
                else                 side = (side / sideLen) * w;

                float alpha = overallAlpha * (1.0f - (float)i / (float)(N - 1));

                glm::vec3 lp = hist[i] + side;
                glm::vec3 rp = hist[i] - side;

                // left vertex (UV.y = 0 → edge fade)
                verts.insert(verts.end(), { lp.x, lp.y, lp.z, alpha, 0.0f, 0.0f });
                // right vertex (UV.y = 1 → edge fade)
                verts.insert(verts.end(), { rp.x, rp.y, rp.z, alpha, 1.0f, 1.0f });
            }

            const glm::vec3& col = p->color;
            shader->setUnifVec3("trailColor", col.r, col.g, col.b);

            glBindVertexArray(m_ribbonVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_ribbonVBO);
            glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, N * 2);
            glBindVertexArray(0);
        }

        shader->unUse();
    }

    // ── Ring rendering ────────────────────────────────────────────────────────

    void EmitterBase::DrawRings(Shader* shader, const glm::mat4& view, const glm::mat4& proj)
    {
        shader->use();
        shader->setUnifMat4("view", view);
        shader->setUnifMat4("projection", proj);

        const auto& sp    = m_spawnProps;
        int         segs  = std::max(3, sp.ringSegments);
        float       inner = sp.ringInnerRadius;
        float       outer = sp.ringOuterRadius;

        std::vector<float> verts;
        verts.reserve((segs + 1) * 2 * 6);

        for (const auto& p : m_particles)
        {
            if (!p->alive) continue;

            float alpha = p->color.a * (1.0f - p->age / p->lifetime);
            float cr = p->color.r, cg = p->color.g, cb = p->color.b;
            shader->setUnifVec3("trailColor", cr, cg, cb);

            float cosR = std::cos(p->rot);
            float sinR = std::sin(p->rot);
            float sc   = p->scale;

            verts.clear();

            for (int i = 0; i <= segs; i++)
            {
                float angle = glm::two_pi<float>() * (float)i / (float)segs;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                // Rotate ring in XZ plane by p->rot around Y axis
                float ox = (cosA * cosR - sinA * sinR) * outer * sc;
                float oz = (cosA * sinR + sinA * cosR) * outer * sc;
                float ix = (cosA * cosR - sinA * sinR) * inner * sc;
                float iz = (cosA * sinR + sinA * cosR) * inner * sc;

                glm::vec3 outerPt = p->pos + glm::vec3(ox, 0.0f, oz);
                glm::vec3 innerPt = p->pos + glm::vec3(ix, 0.0f, iz);

                // outer vertex (UV.y=0 → soft outer edge)
                verts.insert(verts.end(), { outerPt.x, outerPt.y, outerPt.z, alpha, 0.0f, 0.0f });
                // inner vertex (UV.y=1 → soft inner edge)
                verts.insert(verts.end(), { innerPt.x, innerPt.y, innerPt.z, alpha, 1.0f, 1.0f });
            }

            glBindVertexArray(m_ribbonVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_ribbonVBO);
            glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, (segs + 1) * 2);
            glBindVertexArray(0);
        }

        shader->unUse();
    }
}
