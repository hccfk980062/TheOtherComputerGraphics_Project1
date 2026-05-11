#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <memory>
#include <string>
#include <GL/glew.h>

#include "Shader/Shader.h"
#include "ParticleEffects/TrailRenderer.h"

namespace CG
{
    enum class EmitterType { Point, Sphere, Box, Ring, Ribbon };

    struct ParticleInstance
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 acceleration;
        float life;
        float maxLife;
        float size;
        glm::vec4 color;
        bool alive = false;
    };

    // 粒子 Emitter 的抽象基底類別
    // 持有 Emitter 運動狀態、發射設定、粒子池與 GPU 緩衝物件
    class EmitterBase
    {
    public:
        std::string name;
        EmitterType type;

        // ── 層級 ──────────────────────────────────────────────────────────────
        EmitterBase*                              parent = nullptr;
        std::vector<std::unique_ptr<EmitterBase>> children;

        // ── Emitter 運動 ───────────────────────────────────────────────────────
        glm::vec3 position        = {0, 0, 0};
        glm::vec3 velocity        = {0, 0, 0};
        glm::vec3 acceleration    = {0, 0, 0};
        glm::quat rotation        = glm::quat(1, 0, 0, 0);
        glm::vec3 angularVelocity = {0, 0, 0};

        // ── 發射設定 ───────────────────────────────────────────────────────────
        int   emitCount    = 5;
        int   maxParticles = 100;
        float emitInterval = 0.1f;
        float startTime    = 0.0f;
        float endTime      = 5.0f;

        // ── 粒子屬性 ───────────────────────────────────────────────────────────
        float     particleLife             = 1.0f;
        glm::vec3 particleInitVelocity     = {0, 1, 0};
        glm::vec3 particleAcceleration     = {0, -1, 0};
        glm::vec3 particleAngularVelocity  = {0, 0, 0};
        bool      particleRotFollowEmitter = false;
        glm::vec4 particleColor            = {1, 1, 1, 1};
        float     particleSize             = 0.2f;

        virtual ~EmitterBase();

        virtual glm::vec3 SampleSpawnOffset() = 0;
        // 依各 Emitter 類型計算粒子初速度；預設直接回傳 particleInitVelocity
        virtual glm::vec3 ComputeParticleVelocity(const glm::vec3& spawnOffset) { return particleInitVelocity; }
        void Initialize();  // 分配粒子池 + 建立 VAO/VBO（Ribbon 類型自動跳過）
        virtual void Update(float dt, float currentTime);
        virtual void Draw(Shader* particleShader, Shader* ribbonShader,
                          const glm::mat4& view, const glm::mat4& projection);
        glm::vec3 GetWorldPosition() const;  // 沿 parent 鏈累加世界座標

    protected:
        std::vector<ParticleInstance> m_particles;
        float  m_emitTimer   = 0.0f;
        GLuint m_VAO         = 0;
        GLuint m_quadVBO     = 0;
        GLuint m_instanceVBO = 0;

        void Emit();
        int  FirstDeadSlot();
    };

    // ── 具體 Emitter 類型 ─────────────────────────────────────────────────────

    class PointEmitter : public EmitterBase
    {
    public:
        PointEmitter()            { type = EmitterType::Point;  name = "PointEmitter";  }
        glm::vec3 SampleSpawnOffset() override { return {0, 0, 0}; }
    };

    class SphereEmitter : public EmitterBase
    {
    public:
        float radius = 1.0f;
        // 若啟用，每個粒子的初速度方向會指向球心（產生「匯聚成一點」效果）
        bool  convergeToCenter = false;
        float convergeSpeed    = 1.0f;  // 匯聚速度大小（world units/s）

        SphereEmitter()           { type = EmitterType::Sphere; name = "SphereEmitter"; }
        glm::vec3 SampleSpawnOffset() override;
        glm::vec3 ComputeParticleVelocity(const glm::vec3& spawnOffset) override;
    };

    class BoxEmitter : public EmitterBase
    {
    public:
        glm::vec3 extents = {1, 1, 1};
        BoxEmitter()              { type = EmitterType::Box;    name = "BoxEmitter";    }
        glm::vec3 SampleSpawnOffset() override;
    };

    class RingEmitter : public EmitterBase
    {
    public:
        float radius = 1.0f;
        float width  = 0.1f;
        RingEmitter()             { type = EmitterType::Ring;   name = "RingEmitter";   }
        glm::vec3 SampleSpawnOffset() override;
    };

    // RibbonEmitter 不使用 billboard 粒子，改以 TrailRenderer 產生條狀幾何
    class RibbonEmitter : public EmitterBase
    {
    public:
        float     ribbonWidth = 0.2f;
        glm::vec3 ribbonColor = {1, 0.5f, 0};

        RibbonEmitter();
        ~RibbonEmitter() override = default;

        glm::vec3 SampleSpawnOffset() override { return {0, 0, 0}; }
        void Update(float dt, float currentTime) override;
        void Draw(Shader* particleShader, Shader* ribbonShader,
                  const glm::mat4& view, const glm::mat4& projection) override;

    private:
        TrailRenderer m_trail;
    };

} // namespace CG
