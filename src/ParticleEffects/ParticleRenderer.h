#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "../Shader/Shader.h"

namespace CG
{
    // 單一粒子的狀態資料
    struct Particle
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;  // rgba，a 通道隨生命值衰減（fade out）
        float     life;   // 剩餘生命時間（秒）；≤ 0 代表死亡
        float     size;
    };

    // Billboard 粒子渲染器：以 Instanced Rendering 一次繪製所有存活粒子
    // 每個粒子以一張螢幕對齊四邊形（quad）表示，從頂點著色器中取用 view matrix 計算 billboard 方向
    class ParticleRenderer
    {
    public:
        ParticleRenderer(int maxParticles)
        {
            m_maxParticles = maxParticles;
            m_particles.resize(maxParticles);  // 預先分配粒子池，避免執行期動態配置
            initBuffers();
        }

        ~ParticleRenderer()
        {
            glDeleteVertexArrays(1, &m_VAO);
            glDeleteBuffers(1, &m_quadVBO);
            glDeleteBuffers(1, &m_instanceVBO);
        }

        // 在指定位置發射 count 顆粒子，尋找死亡粒子槽重用（物件池策略）
        void emit(glm::vec3 origin, int count = 5)
        {
            for (int i = 0; i < count; ++i)
            {
                int idx = firstDeadParticle();
                if (idx == -1) continue;  // 粒子池已滿，跳過

                Particle& p = m_particles[idx];
                p.position = origin;
                p.velocity = glm::vec3(
                    (rand() % 200 - 100) / 100.f,  // X：-1..1 隨機
                    (rand() % 100) / 50.f,          // Y：0..2 向上噴出
                    (rand() % 200 - 100) / 100.f   // Z：-1..1 隨機
                );
                p.color = glm::vec4(1.f, 0.5f, 0.1f, 1.f);  // 橘色電漿
                p.life  = 1.f + (rand() % 100) / 100.f;      // 1~2 秒存活時間
                p.size  = 0.5f + (rand() % 50) / 1000.f;
            }
        }

        // 每幀更新所有存活粒子：施加重力、移動位置、根據生命值淡出 alpha
        void update(float dt)
        {
            for (auto& p : m_particles)
            {
                if (p.life <= 0.f) continue;
                p.life      -= dt;
                p.position  += p.velocity * dt;
                p.velocity  += glm::vec3(0.f, -2.f, 0.f) * dt;  // 重力加速度
                p.color.a    = p.life;                            // life 直接作為 alpha（fade out）
            }
        }

        // 將所有存活粒子的 instance 資料打包上傳至 GPU，並以加法混合繪製
        void Draw()
        {
            // 收集所有存活粒子的 per-instance 資料（位置 + 顏色 + 大小，共 8 floats）
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

            // 以 buffer orphaning 策略（先傳 nullptr）減少 GPU 驅動的同步等待
            glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), nullptr, GL_STREAM_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());

            // 加法混合（Additive Blending）適合火焰/火花等自發光效果
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);  // 粒子不寫入深度緩衝，避免遮擋後方物件

            glBindVertexArray(m_VAO);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, aliveCount);  // 每個粒子 4 個頂點的 quad
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

    private:
        int                   m_maxParticles;
        std::vector<Particle> m_particles;  // 粒子物件池

        // 靜態 quad 頂點（螢幕空間 2D 座標，billboard 旋轉在頂點著色器中完成）
        float quadVertices[8] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
            -0.5f,  0.5f,
             0.5f,  0.5f,
        };

        GLuint m_VAO, m_quadVBO;  // 靜態 quad 幾何
        GLuint m_instanceVBO;      // per-instance 資料（位置 + 顏色 + 大小），每幀更新

        // 初始化 VAO / VBO：靜態 quad 為 attrib 0，per-instance 為 attrib 1~3
        void initBuffers()
        {
            glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            // attrib 0：靜態 quad 頂點位置（vec2），所有粒子共用
            glGenBuffers(1, &m_quadVBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

            // attrib 1: vec3 instancePos；attrib 2: vec4 instanceColor；attrib 3: float instanceSize
            glGenBuffers(1, &m_instanceVBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(float) * 8, nullptr, GL_STREAM_DRAW);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glVertexAttribDivisor(1, 1);  // 每個 instance 前進一次

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glVertexAttribDivisor(2, 1);

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
            glVertexAttribDivisor(3, 1);

            glBindVertexArray(0);
        }

        // 線性掃描粒子池，回傳第一個死亡粒子的索引；池滿時回傳 -1
        int firstDeadParticle()
        {
            for (int i = 0; i < m_maxParticles; ++i)
                if (m_particles[i].life <= 0.f) return i;
            return -1;
        }
    };
}
