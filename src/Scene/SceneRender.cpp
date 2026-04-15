#include<Scene/SceneRenderer.h>

namespace CG
{
    auto SceneRenderer::Initialize(int width, int height)-> bool
    {
        viewportFramebuffer = new Framebuffer(width, height);

        shaderProgram_particle = new Shader("ShaderPrograms/Particle_vertex.vert", "ShaderPrograms/Particle_fragement.frag");
		shaderProgram_trail = new Shader("ShaderPrograms/Trail_vertex.vert", "ShaderPrograms/Trail_fragment.frag");
		shaderProgram_worldObject = new Shader("ShaderPrograms/shader_worldObject_vertex.vert", "ShaderPrograms/shader_worldObject_fragment.frag");
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
        scene->RenderObjects(shaderProgram_worldObject);

        scene->RenderTrails(shaderProgram_trail);
        scene->RenderParticles(shaderProgram_particle);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  // back to default
    }
    Framebuffer* SceneRenderer::getCurrentViewportFramebuffer()
    {
        return viewportFramebuffer;
    }
}