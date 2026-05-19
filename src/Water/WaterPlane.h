#pragma once
#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Shader/Shader.h"
#include "FrameBuffer/Framebuffer.h"

namespace CG
{
    class WaterPlane
    {
    public:
        static constexpr float HEIGHT = -1.0f;   // world-space water surface Y
        static constexpr float SIZE   = 50.0f;   // half-extent in X and Z

        void Initialize(int viewW, int viewH);
        void Draw(Shader* shader, glm::mat4 view, glm::mat4 proj,
                  glm::vec3 camPos, float time,
                  GLuint reflectionTex, glm::vec3 lightDir, glm::vec3 lightColor);

        std::unique_ptr<Framebuffer> reflectionFBO;

        ~WaterPlane();

    private:
        GLuint VAO = 0, VBO = 0, EBO = 0;
        int    indexCount = 0;
        void   BuildMesh();
    };
}
