#pragma once
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace CG
{
    // 深度專用 FBO，用於 Shadow Mapping（無顏色附件，僅深度貼圖）
    class ShadowMap
    {
    public:
        GLuint fbo;
        GLuint depthTexture;
        int width, height;

        ShadowMap(int w, int h) : width(w), height(h)
        {
            glGenTextures(1, &depthTexture);
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                         w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            // 超出 shadow map 範圍的座標一律判定為「不在陰影中」
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "[ShadowMap] FBO incomplete!\n";

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~ShadowMap()
        {
            glDeleteTextures(1, &depthTexture);
            glDeleteFramebuffers(1, &fbo);
        }
    };
}
