#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "../Shader/Shader.h"

namespace CG
{
    struct Particle 
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;      // rgba
        float     life;       // remaining lifetime (seconds)
        float     size;
    };

    class ParticleRenderer 
    {
    public:
        ParticleRenderer(int maxParticles)
        {
            m_maxParticles = maxParticles;
            m_particles.resize(maxParticles);
            initBuffers();
        }
        ~ParticleRenderer()
        {
            glDeleteVertexArrays(1, &m_VAO);
            glDeleteBuffers(1, &m_quadVBO);
            glDeleteBuffers(1, &m_instanceVBO);
        }

        void emit(glm::vec3 origin, int count = 5)
        {
            for (int i = 0; i < count; ++i) 
            {
                int idx = firstDeadParticle();
                if (idx == -1) continue;

                Particle& p = m_particles[idx];
                p.position = origin;
                p.velocity = glm::vec3(
                    (rand() % 200 - 100) / 100.f,   // -1..1
                    (rand() % 100) / 50.f,     //  0..2 (upward)
                    (rand() % 200 - 100) / 100.f
                );
                p.color = glm::vec4(1.f, 0.5f, 0.1f, 1.f); // orange
                p.life = 1.f + (rand() % 100) / 100.f;     // 1–2 sec
                p.size = 0.5f + (rand() % 50) / 1000.f;
            }
        }


        void update(float dt)
        {
            for (auto& p : m_particles) 
            {
                if (p.life <= 0.f) continue;
                p.life -= dt;
                p.position += p.velocity * dt;
                p.velocity += glm::vec3(0.f, -2.f, 0.f) * dt; // gravity
                p.color.a = p.life;                           // fade out
            }
        }
        void Draw()
        {
            // Build packed instance data for all living particles
            std::vector<float> data;
            data.reserve(m_maxParticles * 8);
            int aliveCount = 0;

            for (auto& p : m_particles) 
            {
                if (p.life <= 0.f) continue;
                data.push_back(p.position.x);
                data.push_back(p.position.y);
                data.push_back(p.position.z);
                data.push_back(p.color.r);
                data.push_back(p.color.g);
                data.push_back(p.color.b);
                data.push_back(p.color.a);
                data.push_back(p.size);
                ++aliveCount;
            }

            if (aliveCount == 0) return;

            // Stream instance data to GPU
            glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), nullptr, GL_STREAM_DRAW); // orphan
            glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());

            // Draw

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blending looks great for fire/sparks
            glDepthMask(GL_FALSE);             // don't write to depth buffer

            glBindVertexArray(m_VAO);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, aliveCount); // 4 verts per quad
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

    private:
        int                  m_maxParticles;
        std::vector<Particle> m_particles;

        float quadVertices[8] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
    -0.5f,  0.5f,
     0.5f,  0.5f,
        };

        GLuint m_VAO, m_quadVBO;  // static quad geometry
        GLuint m_instanceVBO;      // per-instance data (position + color + size)

        void initBuffers()
        {
            glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            // --- Static quad VBO (attrib 0: vec2 position) ---
            glGenBuffers(1, &m_quadVBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

            // --- Instance VBO (attrib 1: vec3 pos, attrib 2: vec4 color, attrib 3: float size) ---
            glGenBuffers(1, &m_instanceVBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
            // Allocate max size, will stream with GL_STREAM_DRAW each frame
            glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(float) * 8, nullptr, GL_STREAM_DRAW);

            // layout(location=1) vec3 instancePos
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glVertexAttribDivisor(1, 1); // advance once per INSTANCE

            // layout(location=2) vec4 instanceColor
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glVertexAttribDivisor(2, 1);

            // layout(location=3) float instanceSize
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
            glVertexAttribDivisor(3, 1);

            glBindVertexArray(0);
        }
        int  firstDeadParticle()
        {
            for (int i = 0; i < m_maxParticles; ++i)
                if (m_particles[i].life <= 0.f) return i;
            return -1; // overwrite the oldest if full
        }
    };
}