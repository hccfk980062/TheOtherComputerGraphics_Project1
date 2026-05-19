#include "Water/WaterPlane.h"

#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace CG
{
    WaterPlane::~WaterPlane()
    {
        if (VAO) { glDeleteVertexArrays(1, &VAO); }
        if (VBO) { glDeleteBuffers(1, &VBO); }
        if (EBO) { glDeleteBuffers(1, &EBO); }
    }

    void WaterPlane::Initialize(int viewW, int viewH)
    {
        reflectionFBO = std::make_unique<Framebuffer>(viewW, viewH);
        BuildMesh();
    }

    // Tessellated grid: (DIVS+1)^2 vertices, DIVS^2 quads (2 tris each)
    void WaterPlane::BuildMesh()
    {
        constexpr int   DIVS = 32;
        constexpr float step = (SIZE * 2.0f) / DIVS;

        struct Vertex { float x, y, z, u, v; };
        std::vector<Vertex>   verts;
        std::vector<unsigned> indices;

        verts.reserve((DIVS + 1) * (DIVS + 1));
        for (int row = 0; row <= DIVS; ++row)
        {
            float z = -SIZE + row * step;
            for (int col = 0; col <= DIVS; ++col)
            {
                float x = -SIZE + col * step;
                float u = static_cast<float>(col) / DIVS;
                float v = static_cast<float>(row) / DIVS;
                verts.push_back({ x, 0.0f, z, u, v });
            }
        }

        indices.reserve(DIVS * DIVS * 6);
        for (int row = 0; row < DIVS; ++row)
        {
            for (int col = 0; col < DIVS; ++col)
            {
                unsigned tl = row * (DIVS + 1) + col;
                unsigned tr = tl + 1;
                unsigned bl = tl + (DIVS + 1);
                unsigned br = bl + 1;
                indices.insert(indices.end(), { tl, bl, tr, tr, bl, br });
            }
        }
        indexCount = static_cast<int>(indices.size());

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     verts.size() * sizeof(Vertex),
                     verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned),
                     indices.data(), GL_STATIC_DRAW);

        // location 0: position (xyz)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), (void*)0);

        // location 1: texcoords (uv)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    void WaterPlane::Draw(Shader* shader, glm::mat4 view, glm::mat4 proj,
                          glm::vec3 camPos, float time,
                          GLuint reflectionTex,
                          glm::vec3 lightDir, glm::vec3 lightColor)
    {
        shader->use();

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, HEIGHT, 0.0f));
        shader->setUnifMat4("model",      model);
        shader->setUnifMat4("view",       view);
        shader->setUnifMat4("projection", proj);
        shader->setUnifVec3("cameraPos",  camPos.x, camPos.y, camPos.z);
        shader->setUnifFloat("time",      time);
        shader->setUnifVec3("lightDir",   lightDir.x, lightDir.y, lightDir.z);
        shader->setUnifVec3("lightColor", lightColor.x, lightColor.y, lightColor.z);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, reflectionTex);
        shader->setUnifInt("reflectionTexture", 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}
