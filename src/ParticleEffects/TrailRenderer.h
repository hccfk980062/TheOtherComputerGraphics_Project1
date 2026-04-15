#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <deque>
#include "../Shader/Shader.h"

namespace CG
{
    // 一個採樣時間點：刀刃兩端的世界座標
    struct TrailPoint 
    {
        glm::vec3 edgeA;     ///< 刀刃一側（例如刀背）
        glm::vec3 edgeB;     ///< 刀刃另一側（例如刀刃）
        float     timestamp;
    };

    // 傳給 GPU 的頂點格式
    struct TrailVertex
    {
        glm::vec3 position;
        float     alpha;
        glm::vec2 uv;       ///< u = 新舊程度(0舊~1新)，v = 0/1(兩側)
    };

    class TrailRenderer 
    {
    public:
        // ── 可調參數 ──────────────────────────────────────────
        float     duration = 0.25f;  ///< 拖尾存活時間（秒）
        float     minDist = 0.02f;  ///< 最小採樣距離（靜止時不塞點）
        float     maxAlpha = 0.9f;   ///< 最新端的 alpha 上限
        int       maxPoints = 128;    ///< 最多保留幾個採樣點
        glm::vec3 color = glm::vec3(1.0f, 0.1f, 0.05f); ///< 拖尾顏色

        TrailRenderer()
        {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
                maxPoints * 2 * sizeof(TrailVertex),
                nullptr, GL_DYNAMIC_DRAW);

            // location 0: position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                sizeof(TrailVertex), (void*)0);
            // location 1: alpha
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                sizeof(TrailVertex), (void*)offsetof(TrailVertex, alpha));
            // location 2: uv
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                sizeof(TrailVertex), (void*)offsetof(TrailVertex, uv));

            glBindVertexArray(0);
        }

        ~TrailRenderer()
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }

        /**
         * @brief 每幀呼叫：採樣刀刃兩端世界座標
         *
         * 使用方式：
         *   glm::vec3 tipA = glm::vec3(modelMatrix * glm::vec4(localEdgeA, 1.0f));
         *   glm::vec3 tipB = glm::vec3(modelMatrix * glm::vec4(localEdgeB, 1.0f));
         *   trail.update(tipA, tipB, glfwGetTime());
         */
        void update(glm::vec3 edgeA, glm::vec3 edgeB, float currentTime)
        {
            // 清除過期點
            while (!points.empty() && currentTime - points.front().timestamp > duration)
                points.pop_front();

            // 移動距離不夠則不採樣（避免靜止時色帶過厚）
            if (!points.empty()) {
                glm::vec3 lastMid = (points.back().edgeA + points.back().edgeB) * 0.5f;
                glm::vec3 newMid = (edgeA + edgeB) * 0.5f;
                if (glm::distance(lastMid, newMid) < minDist) return;
            }

            if ((int)points.size() >= maxPoints) points.pop_front();
            points.push_back({ edgeA, edgeB, currentTime });
        }

        /**
         * @brief 每幀呼叫：建立並繪製色帶
         * @note  請在 FBO Emissive Pass 中呼叫，使拖尾一併被 Bloom
         */
        void Draw(Shader* shader)
        {
            if (points.size() < 2) return;

            shader->use();

            std::vector<TrailVertex> verts;
            verts.reserve(points.size() * 2);

            int n = static_cast<int>(points.size());
            for (int i = 0; i < n; i++)
            {
                float t = static_cast<float>(i) / static_cast<float>(n - 1);
                float alpha = t * maxAlpha;  // 舊端透明，新端不透明

                verts.push_back({ points[i].edgeA, alpha, { t, 0.0f } });
                verts.push_back({ points[i].edgeB, alpha, { t, 1.0f } });
            }

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                verts.size() * sizeof(TrailVertex), verts.data());

            shader->setUnifVec3("trailColor", &color[0]);

            // Additive Blend：自發光拖尾疊加效果
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);  // 不寫深度，避免遮擋後方物件

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0,
                static_cast<GLsizei>(verts.size()));
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        void clear() { points.clear(); }

    private:
        std::deque<TrailPoint> points;
        unsigned int VAO, VBO;
    };
}