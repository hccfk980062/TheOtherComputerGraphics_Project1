#pragma once
#include <memory>
#include <unordered_map>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "FrameBuffer/Framebuffer.h"
#include "FrameBuffer/ShadowMap.h"
#include "Shader/Shader.h"
#include "Scene/MainScene.h"
#include "Skybox/Skybox.h"
#include "Water/WaterPlane.h"

namespace CG
{
    // 負責將 MainScene 渲染到離屏 FBO，並提供顏色拾取（Object Picking）功能
    // 渲染流程：Shadow Depth Pass → Main Phong Pass（含 PCF 陰影）→ Post-Process Pass
    class SceneRenderer
    {
    public:
        // 初始化：建立 viewport / picking / shadowMap FBO，並載入所有著色器
        auto Initialize(int width, int height) -> bool;

        // 渲染到 viewportFramebuffer：先做 shadow pass，再做主光照 pass
        void RenderScene(MainScene* scene);

        // 顏色拾取：以物件 ID 編碼顏色渲染一次，讀取 (x, y) 像素取得 SceneObject*
        SceneObject* GetObjectAtPixel(MainScene* scene, int x, int y);

        // 回傳目前 viewport FBO 的指標（ViewportWindow 用）
        // 馬賽克啟用時回傳 post-process FBO，否則回傳 scene FBO
        Framebuffer* getCurrentViewportFramebuffer();

        // ── 馬賽克濾鏡參數（可由 InspectorWindow 直接讀寫）───────────────────
        bool mosaicEnabled  = false;
        int  mosaicPixelSize = 16;   // 馬賽克格子大小（像素），範圍 2–128

        // ── 動態模糊參數（可由 InspectorWindow 直接讀寫）─────────────────────
        bool  motionBlurEnabled  = false;
        int   motionBlurSamples  = 8;      // 採樣次數（2~16），越高越順滑但越費效能
        float motionBlurStrength = 1.0f;   // 速度縮放係數（0.5~3.0），調高可加強效果

        // ── 環境貼圖參數（Cube 反射 / 折射）─────────────────────────────────
        int   cubeEnvMode      = 0;       // 0=Phong, 1=Reflect, 2=Refract
        float cubeRefractRatio = 0.658f;  // 折射率（預設玻璃）

    private:
        GLenum mode = GL_FILL;

        std::unique_ptr<Shader>      shaderProgram_particle;
        std::unique_ptr<Shader>      shaderProgram_trail;
        std::unique_ptr<Shader>      shaderProgram_worldObject;
        std::unique_ptr<Shader>      shaderProgram_picking;
        std::unique_ptr<Shader>      shaderProgram_shadowDepth;  // Shadow depth pass 著色器
        std::unique_ptr<Shader>      shaderProgram_skybox;       // 天空盒著色器
        std::unique_ptr<Shader>      shaderProgram_envmap;       // 環境貼圖著色器（Cube 反射/折射）
        std::unique_ptr<Skybox>      skybox;                     // 天空盒（Cross Layout Cubemap）

        std::unique_ptr<Framebuffer> viewportFramebuffer;
        std::unique_ptr<Framebuffer> pickingFramebuffer;
        std::unique_ptr<ShadowMap>   shadowMapBuffer;             // 2048×2048 深度貼圖 FBO
        std::unique_ptr<Shader>      shaderProgram_water;
        std::unique_ptr<WaterPlane>  waterPlane;

        // ── Post-Processing（馬賽克濾鏡）────────────────────────────────────
        std::unique_ptr<Framebuffer> postProcessFramebuffer;      // 馬賽克輸出 FBO
        std::unique_ptr<Shader>      shaderProgram_mosaic;
        unsigned int screenQuadVAO = 0;
        unsigned int screenQuadVBO = 0;

        void InitScreenQuad();   // 建立全螢幕四邊形 VAO/VBO（僅呼叫一次）
        void ApplyMosaicPass();  // 從 (motionBlur 或 viewport) FBO → postProcessFramebuffer

        // ── Post-Processing（動態模糊）──────────────────────────────────────
        std::unique_ptr<Framebuffer> motionBlurFramebuffer;       // 動態模糊輸出 FBO
        std::unique_ptr<Shader>      shaderProgram_velocity;      // 速度向量渲染著色器
        std::unique_ptr<Shader>      shaderProgram_motionBlur;    // 動態模糊後製著色器

        // 速度緩衝 FBO（RG16F 浮點貼圖，直接管理而不使用 Framebuffer 類別）
        GLuint velocityFBO      = 0;
        GLuint velocityColorTex = 0;   // RG16F 速度貼圖
        GLuint velocityDepthRBO = 0;   // 深度 RBO（速度 pass 需要深度測試）
        int    velocityFBOWidth = 0, velocityFBOHeight = 0;

        // 前一幀的矩陣資料（計算每物件運動向量用）
        glm::mat4 m_prevViewProjection = glm::mat4(1.0f);
        std::unordered_map<uint32_t, glm::mat4> m_prevWorldMatrices;  // key = SceneObject::id
        bool m_motionBlurFirstFrame = true;  // 第一幀無前幀資料，跳過動態模糊

        void InitVelocityBuffer(int w, int h);           // 建立 RG16F 速度 FBO
        void ResizeVelocityBuffer(int w, int h);         // 調整速度 FBO 大小
        void RenderVelocityPass(MainScene* scene);       // 渲染速度向量緩衝
        void ApplyMotionBlurPass();                      // 讀取速度貼圖，輸出模糊結果

        glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);            // 光源空間矩陣（每幀更新）

        // 根據光源類型與參數計算 lightSpaceMatrix
        glm::mat4 ComputeLightSpaceMatrix(const LightData& light);
    };
}
