#include <cmath>
#include <glm/gtc/constants.hpp>

#include "ParticleEffects/ConcreteEmitters.h"

namespace CG
{
    // ── PointEmitter ──────────────────────────────────────────────────────────

    PointEmitter::PointEmitter()
    {
        m_type = EmitterType::Point;
        m_name = "PointEmitter";
    }

    glm::vec3 PointEmitter::SampleSpawnOffset(std::mt19937& /*rng*/) const
    {
        return glm::vec3(0.0f);
    }

    std::unique_ptr<EmitterBase> PointEmitter::Clone() const
    {
        auto copy = std::make_unique<PointEmitter>();
        CopyBaseTo(*copy);
        return copy;
    }

    // ── SphereEmitter ─────────────────────────────────────────────────────────

    SphereEmitter::SphereEmitter()
    {
        m_type = EmitterType::Sphere;
        m_name = "SphereEmitter";
    }

    glm::vec3 SphereEmitter::SampleSpawnOffset(std::mt19937& rng) const
    {
        // Rejection sampling: uniform point on unit sphere surface
        std::uniform_real_distribution<float> d(-1.0f, 1.0f);
        glm::vec3 v;
        float len;
        do {
            v   = glm::vec3(d(rng), d(rng), d(rng));
            len = glm::length(v);
        } while (len < 1e-6f || len > 1.0f);
        return (v / len) * m_radius;
    }

    std::unique_ptr<EmitterBase> SphereEmitter::Clone() const
    {
        auto copy = std::make_unique<SphereEmitter>();
        CopyBaseTo(*copy);
        copy->m_radius = m_radius;
        return copy;
    }

    // ── BoxEmitter ────────────────────────────────────────────────────────────

    BoxEmitter::BoxEmitter()
    {
        m_type = EmitterType::Box;
        m_name = "BoxEmitter";
    }

    glm::vec3 BoxEmitter::SampleSpawnOffset(std::mt19937& rng) const
    {
        std::uniform_real_distribution<float> dx(-m_halfExtents.x, m_halfExtents.x);
        std::uniform_real_distribution<float> dy(-m_halfExtents.y, m_halfExtents.y);
        std::uniform_real_distribution<float> dz(-m_halfExtents.z, m_halfExtents.z);
        return glm::vec3(dx(rng), dy(rng), dz(rng));
    }

    std::unique_ptr<EmitterBase> BoxEmitter::Clone() const
    {
        auto copy = std::make_unique<BoxEmitter>();
        CopyBaseTo(*copy);
        copy->m_halfExtents = m_halfExtents;
        return copy;
    }

    // ── RingEmitter ───────────────────────────────────────────────────────────

    RingEmitter::RingEmitter()
    {
        m_type = EmitterType::Ring;
        m_name = "RingEmitter";
    }

    glm::vec3 RingEmitter::SampleSpawnOffset(std::mt19937& rng) const
    {
        std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
        float half = m_bandWidth * 0.5f;
        std::uniform_real_distribution<float> radiusDist(m_radius - half, m_radius + half);

        float angle = angleDist(rng);
        float r     = radiusDist(rng);
        return glm::vec3(std::cos(angle) * r, 0.0f, std::sin(angle) * r);
    }

    std::unique_ptr<EmitterBase> RingEmitter::Clone() const
    {
        auto copy = std::make_unique<RingEmitter>();
        CopyBaseTo(*copy);
        copy->m_radius    = m_radius;
        copy->m_bandWidth = m_bandWidth;
        return copy;
    }
}
