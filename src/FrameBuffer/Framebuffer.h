#pragma once
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace CG
{
    class Framebuffer
    {
    public:
        GLuint fbo, colorTexture, depthRBO;
        int width, height;

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

        void CreateFramebuffer(int width, int height)
        {
            this->width  = width;
            this->height = height;

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            // Color attachment (this becomes the ImGui image)
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

            // Depth + stencil renderbuffer
            glGenRenderbuffers(1, &depthRBO);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "[Framebuffer] CreateFramebuffer: FBO incomplete!\n";

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void ResizeFramebuffer(int w, int h)
        {
            if (w <= 0 || h <= 0)
            {
                std::cerr << "[Framebuffer] ResizeFramebuffer: invalid size (" << w << "x" << h << ")\n";
                return;
            }

            this->width  = w;
            this->height = h;

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
