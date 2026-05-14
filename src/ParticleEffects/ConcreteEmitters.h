#pragma once
#include "ParticleEffects/EmitterBase.h"

namespace CG
{
    class PointEmitter : public EmitterBase
    {
    public:
        PointEmitter();
        std::unique_ptr<EmitterBase> Clone() const override;
    protected:
        glm::vec3 SampleSpawnOffset(std::mt19937& rng) const override;
    };

    // Spawns particles on the surface of a sphere
    class SphereEmitter : public EmitterBase
    {
    public:
        SphereEmitter();
        float m_radius = 1.0f;
        std::unique_ptr<EmitterBase> Clone() const override;
    protected:
        glm::vec3 SampleSpawnOffset(std::mt19937& rng) const override;
    };

    // Spawns particles uniformly inside an axis-aligned box
    class BoxEmitter : public EmitterBase
    {
    public:
        BoxEmitter();
        glm::vec3 m_halfExtents = glm::vec3(1.0f);
        std::unique_ptr<EmitterBase> Clone() const override;
    protected:
        glm::vec3 SampleSpawnOffset(std::mt19937& rng) const override;
    };

    // Spawns particles on a ring (circle) in the XZ plane
    class RingEmitter : public EmitterBase
    {
    public:
        RingEmitter();
        float m_radius     = 1.0f;
        float m_bandWidth  = 0.1f;  // radial thickness
        std::unique_ptr<EmitterBase> Clone() const override;
    protected:
        glm::vec3 SampleSpawnOffset(std::mt19937& rng) const override;
    };
}
