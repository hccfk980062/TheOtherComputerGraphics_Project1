#pragma once
#include <glm/glm.hpp>

namespace CG
{
    struct LightData
    {
        enum class Type { Directional = 0, Point = 1 };

        Type      type       = Type::Point;
        glm::vec3 position   = glm::vec3(0.0f, 10.0f, 50.0f);
        glm::vec3 direction  = glm::vec3(0.0f, -1.0f, -1.0f);
        glm::vec3 color      = glm::vec3(1.0f, 1.0f, 1.0f);
        float     intensity  = 1.0f;
        float     shadowBias = 0.005f;
    };
}
