#include "Scene/SceneRenderer.h"

namespace CG
{
    auto SceneRenderer::Initialize(int width, int height) -> bool
    {
        // 建立顯示用與拾取用的離屏 FBO
        viewportFramebuffer = std::make_unique<Framebuffer>(width, height);
        pickingFramebuffer  = std::make_unique<Framebuffer>(width, height);

        // 載入並編譯各用途的著色器程式
        shaderProgram_particle    = std::make_unique<Shader>("ShaderPrograms/Particle_vertex.vert",           "ShaderPrograms/Particle_fragement.frag");
        shaderProgram_trail       = std::make_unique<Shader>("ShaderPrograms/Trail_vertex.vert",              "ShaderPrograms/Trail_fragment.frag");
        shaderProgram_worldObject = std::make_unique<Shader>("ShaderPrograms/shader_worldObject_vertex.vert", "ShaderPrograms/shader_worldObject_fragment.frag");
        shaderProgram_picking     = std::make_unique<Shader>("ShaderPrograms/shader_picking_vertex.vert",     "ShaderPrograms/shader_picking_fragment.frag");
        return true;
    }

    void SceneRenderer::RenderScene(MainScene* scene)
    {
        // 渲染目標切換至 viewport FBO
        glBindFramebuffer(GL_FRAMEBUFFER, viewportFramebuffer->fbo);
        glViewport(0, 0, viewportFramebuffer->width, viewportFramebuffer->height);
        glClearColor(0.15f, 0.4f, 0.15f, 1.0f);  // 深綠底色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // 確保相機投影矩陣與 FBO 當前尺寸一致（視窗縮放後需重算）
        scene->freeViewCamera.SetProjectionMatrix(viewportFramebuffer->width, viewportFramebuffer->height);

        // 依序渲染：實體物件 → 刀光拖尾 → 粒子（粒子需疊加混合，放最後）
        scene->RenderObjects(shaderProgram_worldObject.get());
        scene->RenderTrails(shaderProgram_trail.get());
        scene->RenderParticles(shaderProgram_particle.get());

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    SceneObject* SceneRenderer::GetObjectAtPixel(MainScene* scene, int x, int y)
    {
        // 若 picking FBO 大小與 viewport FBO 不一致則重設（懶惰同步）
        if (pickingFramebuffer->width  != viewportFramebuffer->width ||
            pickingFramebuffer->height != viewportFramebuffer->height)
        {
            pickingFramebuffer->ResizeFramebuffer(
                viewportFramebuffer->width, viewportFramebuffer->height);
        }

        // ── 拾取渲染 Pass：以物件 ID 編碼為顏色，渲染整個場景 ────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFramebuffer->fbo);
        glViewport(0, 0, pickingFramebuffer->width, pickingFramebuffer->height);
        // 背景清為純白（0xFFFFFF），這是一個無效哨兵值，不會與任何合法物件 ID 衝突
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene->freeViewCamera.SetProjectionMatrix(pickingFramebuffer->width, pickingFramebuffer->height);
        scene->RenderObjectsForPicking(shaderProgram_picking.get());

        // ── 讀取目標像素顏色 ─────────────────────────────────────────────────
        // OpenGL 原點在左下角；ImGui 影像座標原點在左上角，需翻轉 Y 軸
        int flippedY = pickingFramebuffer->height - 1 - y;
        GLubyte pixel[3] = { 0xFF, 0xFF, 0xFF };
        glReadPixels(x, flippedY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 將 RGB 三通道合併還原為 24 位元物件 ID
        uint32_t id = (static_cast<uint32_t>(pixel[0]) << 16)
                    | (static_cast<uint32_t>(pixel[1]) << 8)
                    |  static_cast<uint32_t>(pixel[2]);

        if (id == 0xFFFFFFu) return nullptr;  // 點到背景
        return scene->FindObjectById(id);
    }

    Framebuffer* SceneRenderer::getCurrentViewportFramebuffer()
    {
        return viewportFramebuffer.get();
    }
}
