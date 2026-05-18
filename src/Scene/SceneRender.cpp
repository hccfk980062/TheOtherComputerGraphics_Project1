#include "Scene/SceneRenderer.h"

namespace CG
{
    auto SceneRenderer::Initialize(int width, int height) -> bool
    {
        viewportFramebuffer = std::make_unique<Framebuffer>(width, height);
        pickingFramebuffer  = std::make_unique<Framebuffer>(width, height);
        shadowMapBuffer     = std::make_unique<ShadowMap>(2048, 2048);

        shaderProgram_particle    = std::make_unique<Shader>("ShaderPrograms/Particle_vertex.vert",              "ShaderPrograms/Particle_fragement.frag");
        shaderProgram_trail       = std::make_unique<Shader>("ShaderPrograms/Trail_vertex.vert",                 "ShaderPrograms/Trail_fragment.frag");
        shaderProgram_worldObject = std::make_unique<Shader>("ShaderPrograms/shader_worldObject_vertex.vert",   "ShaderPrograms/shader_worldObject_fragment.frag");
        shaderProgram_picking     = std::make_unique<Shader>("ShaderPrograms/shader_picking_vertex.vert",       "ShaderPrograms/shader_picking_fragment.frag");
        shaderProgram_shadowDepth = std::make_unique<Shader>("ShaderPrograms/shader_shadow_depth_vertex.vert",  "ShaderPrograms/shader_shadow_depth_fragment.frag");
        shaderProgram_skybox      = std::make_unique<Shader>("ShaderPrograms/skybox_vertex.vert",               "ShaderPrograms/skybox_fragment.frag");
        skybox = std::make_unique<Skybox>("Textures/skyboxsun5deg.png");
        return true;
    }

    // 依光源類型計算 light-space MVP 矩陣，供 shadow pass 與主 pass 共用
    glm::mat4 SceneRenderer::ComputeLightSpaceMatrix(const LightData& light)
    {
        glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);
        glm::mat4 lightView, lightProj;

        if (light.type == LightData::Type::Directional)
        {
            glm::vec3 dir     = glm::normalize(light.direction);
            glm::vec3 lightPos = sceneCenter - dir * 50.0f;
            // 避免 up 向量與 dir 平行（造成 lookAt 退化）
            glm::vec3 up = (glm::abs(glm::dot(dir, glm::vec3(0, 1, 0))) < 0.99f)
                         ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            lightView = glm::lookAt(lightPos, sceneCenter, up);
            // 正交投影覆蓋場景範圍（-30~30 世界單位）
            lightProj = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 200.0f);
        }
        else
        {
            glm::vec3 toCenter = sceneCenter - light.position;
            glm::vec3 up = (glm::length(toCenter) > 0.001f &&
                            glm::abs(glm::dot(glm::normalize(toCenter), glm::vec3(0, 1, 0))) < 0.99f)
                         ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            lightView = glm::lookAt(light.position, sceneCenter, up);
            // 透視投影（90° FOV，正方形 aspect）
            lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);
        }

        return lightProj * lightView;
    }

    void SceneRenderer::RenderScene(MainScene* scene)
    {
        // ── Shadow Depth Pass ──────────────────────────────────────────────────
        // 從光源視角渲染整個場景，僅寫入深度值到 shadowMapBuffer
        lightSpaceMatrix = ComputeLightSpaceMatrix(scene->light);

        glViewport(0, 0, shadowMapBuffer->width, shadowMapBuffer->height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapBuffer->fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        shaderProgram_shadowDepth->use();
        shaderProgram_shadowDepth->setUnifMat4("lightSpaceMatrix", lightSpaceMatrix);
        // RenderObjects 內部會再次呼叫 use()（no-op）並嘗試設 view/projection（無此 uniform 則靜默略過）
        scene->RenderObjects(shaderProgram_shadowDepth.get());

        // ── Main Phong Render Pass ─────────────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, viewportFramebuffer->fbo);
        glViewport(0, 0, viewportFramebuffer->width, viewportFramebuffer->height);
        glClearColor(0.15f, 0.4f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene->freeViewCamera.SetProjectionMatrix(viewportFramebuffer->width, viewportFramebuffer->height);

        // 將 shadow depth texture 綁定至 texture unit 4（0~3 由材質貼圖使用）
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, shadowMapBuffer->depthTexture);

        const LightData& lt = scene->light;

        shaderProgram_worldObject->use();
        shaderProgram_worldObject->setUnifInt  ("shadowMap",       4);
        shaderProgram_worldObject->setUnifMat4 ("lightSpaceMatrix",lightSpaceMatrix);
        shaderProgram_worldObject->setUnifInt  ("lightType",       static_cast<int>(lt.type));
        shaderProgram_worldObject->setUnifVec3 ("lightPosition",   lt.position.x,  lt.position.y,  lt.position.z);
        shaderProgram_worldObject->setUnifVec3 ("lightDirection",  lt.direction.x, lt.direction.y, lt.direction.z);
        shaderProgram_worldObject->setUnifVec3 ("lightColor",      lt.color.x,     lt.color.y,     lt.color.z);
        shaderProgram_worldObject->setUnifFloat("lightIntensity",  lt.intensity);
        shaderProgram_worldObject->setUnifFloat("shadowBias",      lt.shadowBias);

        // 渲染順序：
        //   1. 不透明物件（寫入 depth buffer）
        //   2. 天空盒（GL_LEQUAL，不寫 depth，填補無物件區域的背景色）
        //   3. 半透明物件 Trail / 粒子（不寫 depth，以 src_alpha 混色疊加）
        // 若天空盒在粒子之後繪製，GL_LEQUAL 會在粒子的 depth=1.0 區域通過並蓋掉粒子色
        scene->RenderObjects(shaderProgram_worldObject.get());

        skybox->Draw(
            shaderProgram_skybox.get(),
            scene->freeViewCamera.GetViewMatrix(),
            scene->freeViewCamera.GetProjectionMatrix()
        );

        scene->RenderTrails(shaderProgram_trail.get());
        scene->RenderParticles(shaderProgram_particle.get(), shaderProgram_trail.get());

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    SceneObject* SceneRenderer::GetObjectAtPixel(MainScene* scene, int x, int y)
    {
        if (pickingFramebuffer->width  != viewportFramebuffer->width ||
            pickingFramebuffer->height != viewportFramebuffer->height)
        {
            pickingFramebuffer->ResizeFramebuffer(
                viewportFramebuffer->width, viewportFramebuffer->height);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, pickingFramebuffer->fbo);
        glViewport(0, 0, pickingFramebuffer->width, pickingFramebuffer->height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene->freeViewCamera.SetProjectionMatrix(pickingFramebuffer->width, pickingFramebuffer->height);
        scene->RenderObjectsForPicking(shaderProgram_picking.get());

        int flippedY = pickingFramebuffer->height - 1 - y;
        GLubyte pixel[3] = { 0xFF, 0xFF, 0xFF };
        glReadPixels(x, flippedY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        uint32_t id = (static_cast<uint32_t>(pixel[0]) << 16)
                    | (static_cast<uint32_t>(pixel[1]) << 8)
                    |  static_cast<uint32_t>(pixel[2]);

        if (id == 0xFFFFFFu) return nullptr;
        return scene->FindObjectById(id);
    }

    Framebuffer* SceneRenderer::getCurrentViewportFramebuffer()
    {
        return viewportFramebuffer.get();
    }
}
