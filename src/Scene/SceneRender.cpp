#include<Scene/SceneRenderer.h>

namespace CG
{
    auto SceneRenderer::Initialize(int width, int height)-> bool
    {
        framebuffer = new Framebuffer(width, height);

		defaultShader = new Shader("ShaderPrograms/Shader.vert", "ShaderPrograms/Shader.frag");
		shaderProgram_worldObject = new Shader("ShaderPrograms/shader_worldObject_vertex.vert", "ShaderPrograms/shader_worldObject_fragment.frag");
        return true;
    }


    void SceneRenderer::RenderScene(MainScene *scene)
    {
        // ── Pass 1: Render scene into FBO ──────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->fbo);
        glViewport(0, 0, framebuffer->width, framebuffer->height);
        glClearColor(0.15f, 0.4f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        scene->freeViewCamera.SetProjectionMatrix(framebuffer->width, framebuffer->height);

        scene->Render(shaderProgram_worldObject);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);  // back to default
    }
    Framebuffer* SceneRenderer::getCurrentFramebuffer()
    {
        return framebuffer;
    }
}