#pragma once
#include <memory>

#include "FrameBuffer/Framebuffer.h"
#include "Shader/Shader.h"
#include "Scene/MainScene.h"

namespace CG
{
    // 負責將 MainScene 渲染到離屏 FBO，並提供顏色拾取（Object Picking）功能
    // 所有著色器程式均在 Initialize() 時編譯完成
    class SceneRenderer
    {
    public:
        // 初始化：建立 viewport FBO 與 picking FBO，並載入所有著色器
        auto Initialize(int width, int height) -> bool;

        // 將場景渲染到 viewportFramebuffer（顏色 FBO），供 Viewport 視窗顯示
        void RenderScene(MainScene* scene);

        // 顏色拾取：以物件 ID 編碼顏色渲染一次，讀取 (x, y) 像素取得 SceneObject*
        // 座標系為視口影像空間（左上角為原點）；回傳 nullptr 代表背景
        SceneObject* GetObjectAtPixel(MainScene* scene, int x, int y);

        // 回傳目前 viewport FBO 的指標（ViewportWindow 用於同步大小與顯示）
        Framebuffer* getCurrentViewportFramebuffer();

    private:
        GLenum mode = GL_FILL;  // 目前繪製模式（GL_FILL / GL_LINE）

        std::unique_ptr<Shader>      shaderProgram_particle;     // 粒子 Billboard 著色器
        std::unique_ptr<Shader>      shaderProgram_trail;        // 刀光拖尾著色器
        std::unique_ptr<Shader>      shaderProgram_worldObject;  // 一般 3D 物件著色器（Phong）
        std::unique_ptr<Shader>      shaderProgram_picking;      // 顏色拾取用著色器
        std::unique_ptr<Framebuffer> viewportFramebuffer;        // 顯示用離屏 FBO
        std::unique_ptr<Framebuffer> pickingFramebuffer;         // 拾取用離屏 FBO（懶惰同步大小）
    };
}
