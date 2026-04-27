#pragma once
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace CG
{
    // 封裝一個 OpenGL 離屏 Framebuffer（FBO），包含顏色貼圖與深度/模板 RBO
    // 主要用途：將 3D 場景渲染到貼圖，再以 ImGui::Image 顯示於 Viewport 視窗
    class Framebuffer
    {
    public:
        GLuint fbo;           // Framebuffer Object 控制代碼
        GLuint colorTexture;  // 顏色附件：RGBA 貼圖，供 ImGui 作為影像顯示
        GLuint depthRBO;      // 深度 + 模板 Renderbuffer，支援深度測試
        int width, height;    // 當前 FBO 尺寸（像素）

        Framebuffer(int width, int height)
        {
            CreateFramebuffer(width, height);
        }

        ~Framebuffer()
        {
            glDeleteTextures(1, &colorTexture);
            glDeleteRenderbuffers(1, &depthRBO);
            glDeleteFramebuffers(1, &fbo);
        }

        // 建立指定尺寸的 FBO、顏色貼圖與深度 RBO
        void CreateFramebuffer(int width, int height)
        {
            this->width  = width;
            this->height = height;

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            // 顏色附件：GL_RGB 格式，將作為 ImGui 顯示的影像貼圖
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

            // 深度 + 模板 Renderbuffer（GL_DEPTH24_STENCIL8）
            glGenRenderbuffers(1, &depthRBO);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "[Framebuffer] CreateFramebuffer: FBO incomplete!\n";

            glBindFramebuffer(GL_FRAMEBUFFER, 0);  // 解除綁定，避免後續誤寫入
        }

        // 調整 FBO 大小：重新分配顏色貼圖與深度 RBO 的儲存空間
        // 由 ViewportWindow::SyncFramebufferSize() 在視窗尺寸改變時呼叫
        void ResizeFramebuffer(int w, int h)
        {
            if (w <= 0 || h <= 0)
            {
                std::cerr << "[Framebuffer] ResizeFramebuffer: invalid size (" << w << "x" << h << ")\n";
                return;
            }

            this->width  = w;
            this->height = h;

            // 重新上傳顏色貼圖（不需要重建 GL 物件，直接改儲存大小）
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "[Framebuffer] ResizeFramebuffer: FBO incomplete after resize!\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    };
}
