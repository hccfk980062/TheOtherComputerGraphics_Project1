#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>

namespace CG
{
    class EmitterBase;

    enum class ParticleType { Sprite, Trail, Ring };

    struct Particle
    {
        glm::vec3 pos    = glm::vec3(0);
        glm::vec3 vel    = glm::vec3(0, 1, 0);
        glm::vec3 accel  = glm::vec3(0);
        glm::vec3 dirVel = glm::vec3(0);  // angular velocity of travel direction (degrees/sec, XYZ)

        float rot      = 0.0f;
        float rotVel   = 0.0f;
        float rotAccel = 0.0f;

        // World-space orientation captured at spawn time (from emitter's ownerNode).
        // Used by DrawRings to orient the ring geometry into the correct plane.
        glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity

        float scale      = 1.0f;
        float scaleVel   = 0.0f;
        float scaleAccel = 0.0f;

        glm::vec4 color    = glm::vec4(1.0f);
        GLuint    textureID = 0;  // 0 = solid color

        float lifetime = 1.0f;
        float age      = 0.0f;
        bool  alive    = false;

        // Trail type: world-space position history, newest first
        std::vector<glm::vec3> trailHistory;
        int maxTrailLength = 20;

        // Owned child emitter instance; lives and dies with the particle
        std::unique_ptr<EmitterBase> childInstance;
    };
}
