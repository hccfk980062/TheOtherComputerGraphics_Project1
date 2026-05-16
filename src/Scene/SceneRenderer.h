#pragma once
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "FrameBuffer/Framebuffer.h"
#include "FrameBuffer/ShadowMap.h"
#include "Shader/Shader.h"
#include "Scene/MainScene.h"
#include "Skybox/Skybox.h"

namespace CG
{
    // 負責將 MainScene 渲染到離屏 FBO，並提供顏色拾取（Object Picking）功能
    // 渲染流程：Shadow Depth Pass → Main Phong Pass（含 PCF 陰影）
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
        Framebuffer* getCurrentViewportFramebuffer();

    private:
        GLenum mode = GL_FILL;

        std::unique_ptr<Shader>      shaderProgram_particle;
        std::unique_ptr<Shader>      shaderProgram_trail;
        std::unique_ptr<Shader>      shaderProgram_worldObject;
        std::unique_ptr<Shader>      shaderProgram_picking;
        std::unique_ptr<Shader>      shaderProgram_shadowDepth;  // Shadow depth pass 著色器
        std::unique_ptr<Shader>      shaderProgram_skybox;       // 天空盒著色器
        std::unique_ptr<Skybox>      skybox;                     // 天空盒（Cross Layout Cubemap）

        std::unique_ptr<Framebuffer> viewportFramebuffer;
        std::unique_ptr<Framebuffer> pickingFramebuffer;
        std::unique_ptr<ShadowMap>   shadowMapBuffer;             // 2048×2048 深度貼圖 FBO

        glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);            // 光源空間矩陣（每幀更新）

        // 根據光源類型與參數計算 lightSpaceMatrix
        glm::mat4 ComputeLightSpaceMatrix(const LightData& light);
    };
}
