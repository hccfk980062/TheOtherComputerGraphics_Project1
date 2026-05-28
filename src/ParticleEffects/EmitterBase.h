#pragma once
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>

#include "Shader/Shader.h"
#include "ParticleEffects/Particle.h"

namespace CG
{
    struct SceneObject;  // forward declare; defined in Scene/Transform.h

    enum class EmitterType { Point, Sphere, Box, Ring };

    struct EmitterSpawnProps
    {
        // Position offset from spawn origin (plus random amplitude)
        glm::vec3 initPos        = glm::vec3(0);
        glm::vec3 initPosRand    = glm::vec3(0);

        // Velocity
        glm::vec3 initVel        = glm::vec3(0, 1, 0);
        glm::vec3 initVelRand    = glm::vec3(0.1f);

        // Acceleration
        glm::vec3 initAccel      = glm::vec3(0);
        glm::vec3 initAccelRand  = glm::vec3(0);

        // Rotation (radians)
        float initRot          = 0.0f;   float initRotRand       = 0.0f;
        float initRotVel       = 0.0f;   float initRotVelRand    = 0.0f;
        float initRotAccel     = 0.0f;   float initRotAccelRand  = 0.0f;

        // Scale (uniform scalar)
        float initScale        = 0.2f;   float initScaleRand      = 0.0f;
        float initScaleVel     = 0.0f;   float initScaleVelRand   = 0.0f;
        float initScaleAccel   = 0.0f;   float initScaleAccelRand = 0.0f;

        // Appearance
        glm::vec4   color       = glm::vec4(1.0f, 0.5f, 0.1f, 1.0f);
        GLuint      textureID   = 0;   // 0 = solid color
        std::string texturePath = "";  // "" = none; stores file path of loaded image

        // Lifetime
        float lifetime     = 2.0f;
        float lifetimeRand = 0.5f;

        // Trail-specific
        int   maxTrailLength = 20;
        float trailWidth     = 0.1f;

        // Ring-specific (inner/outer radius and segment count)
        float ringInnerRadius = 0.2f;
        float ringOuterRadius = 0.5f;
        int   ringSegments    = 32;

        // Direction offset (Euler angles in degrees) applied to velocity and acceleration.
        // Rotates the particle's travel direction in the emitter's local frame.
        glm::vec3 initDir        = glm::vec3(0.0f);
        glm::vec3 initDirRand    = glm::vec3(0.0f);

        // Angular velocity of travel direction (degrees/sec, XYZ).
        // Continuously rotates vel each frame, creating curved/spiral trajectories.
        glm::vec3 initDirVel     = glm::vec3(0.0f);
        glm::vec3 initDirVelRand = glm::vec3(0.0f);

        ParticleType particleType = ParticleType::Sprite;

        bool fadeOverLifetime = true;
    };

    class EmitterBase
    {
    public:
        EmitterBase();
        virtual ~EmitterBase();

        // Identity & UI state
        std::string  m_name     = "Emitter";
        EmitterType  m_type     = EmitterType::Point;
        bool         m_open     = true;   // hierarchy tree node open state
        SceneObject* ownerNode  = nullptr; // non-owning; set by MainScene::LoadParticleEffect
        int          m_seqFrame = 0;       // driven by SequencerWindow; used as timelineFrame in RenderParticles

        // World-space rotation applied to spawn offsets and initial velocities.
        // Updated every frame by MainScene::RenderParticles from ownerNode's world matrix.
        // Newly activated particles will spawn in the rotated direction automatically.
        glm::quat    m_worldRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity

        // Emission control
        int   m_spawnCount    = 5;
        int   m_maxParticles  = 100;
        float m_spawnInterval = 0.1f;

        // Timeline active range (frames)
        int m_startFrame = 0;
        int m_endFrame   = 120;

        // Per-particle property template
        EmitterSpawnProps m_spawnProps;

        // Live particle pool (dead slots reused via alive flag)
        std::vector<std::unique_ptr<Particle>> m_particles;

        // Child emitter templates (not updated/rendered directly; cloned into each particle)
        std::vector<std::unique_ptr<EmitterBase>> m_children;

        // Per-frame update: physics + emission gated by timeline frame
        void Update(float dt, int timelineFrame, glm::vec3 basePos = glm::vec3(0));

        // Render dispatch: Sprite→billboard, Trail→ribbon, Ring→disc, then recurse children
        void Draw(Shader* billboardShader, Shader* trailShader,
                  const glm::mat4& view, const glm::mat4& proj,
                  glm::vec3 cameraPos);

        // Deep copy for child instance spawning; subclasses call CopyBaseTo() then copy own fields
        virtual std::unique_ptr<EmitterBase> Clone() const = 0;

        int AliveCount() const;

    protected:
        virtual glm::vec3 SampleSpawnOffset(std::mt19937& rng) const = 0;
        void CopyBaseTo(EmitterBase& dst) const;

    private:
        float        m_spawnAccumulator = 0.0f;
        std::mt19937 m_rng;

        void EmitBurst(int count, glm::vec3 basePos);
        void ActivateParticle(Particle& p, glm::vec3 basePos);
        void UpdateParticlePhysics(Particle& p, float dt);

        // ── GPU resources (lazily initialized on first Draw call) ─────────────
        GLuint m_quadVAO          = 0;
        GLuint m_quadVBO          = 0;
        GLuint m_instancePosVBO   = 0;
        GLuint m_instanceColorVBO = 0;
        GLuint m_instanceSizeVBO  = 0;
        GLuint m_instanceRotVBO   = 0;
        GLuint m_ribbonVAO        = 0;
        GLuint m_ribbonVBO        = 0;
        bool   m_gpuInitialized   = false;

        void InitGPUResources();
        void DrawSprites(Shader* shader, const glm::mat4& view, const glm::mat4& proj);
        void DrawTrails(Shader* shader, const glm::mat4& view, const glm::mat4& proj, glm::vec3 camPos);
        void DrawRings(Shader* shader, const glm::mat4& view, const glm::mat4& proj);
    };
}
