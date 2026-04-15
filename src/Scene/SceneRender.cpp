#include<Scene/SceneRenderer.h>

namespace CG
{
    auto SceneRenderer::Initialize(int width, int height)-> bool
    {
        viewportFramebuffer    = std::make_unique<Framebuffer>(width, height);
        shaderProgram_particle = std::make_unique<Shader>("ShaderPrograms/Particle_vertex.vert",        "ShaderPrograms/Particle_fragement.frag");
        shaderProgram_trail    = std::make_unique<Shader>("ShaderPrograms/Trail_vertex.vert",           "ShaderPrograms/Trail_fragment.frag");
        shaderProgram_worldObject = std::make_unique<Shader>("ShaderPrograms/shader_worldObject_vertex.vert", "ShaderPrograms/shader_worldObject_fragment.frag");
        return true;
    }


    void SceneRenderer::RenderScene(MainScene *scene)
    {
        // ── Pass 1: Render scene into FBO ──────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, viewportFramebuffer->fbo);
        glViewport(0, 0, viewportFramebuffer->width, viewportFramebuffer->height);
        glClearColor(0.15f, 0.4f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        scene->freeViewCamera.SetProjectionMatrix(viewportFramebuffer->width, viewportFramebuffer->height);
        scene->RenderObjects(shaderProgram_worldObject.get());

        scene->RenderTrails(shaderProgram_trail.get());
        scene->RenderParticles(shaderProgram_particle.get());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  // back to default
    }
    Framebuffer* SceneRenderer::getCurrentViewportFramebuffer()
    {
        return viewportFramebuffer.get();
    }
}