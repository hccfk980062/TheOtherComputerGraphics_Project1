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
        shaderProgram_water       = std::make_unique<Shader>("ShaderPrograms/water_vertex.vert",                 "ShaderPrograms/water_fragment.frag");
        skybox = std::make_unique<Skybox>("Textures/skyboxsun5deg.png");
        waterPlane = std::make_unique<WaterPlane>();
        waterPlane->Initialize(width, height);
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
        lightSpaceMatrix = ComputeLightSpaceMatrix(scene->light);

        glViewport(0, 0, shadowMapBuffer->width, shadowMapBuffer->height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapBuffer->fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        shaderProgram_shadowDepth->use();
        shaderProgram_shadowDepth->setUnifMat4("lightSpaceMatrix", lightSpaceMatrix);
        scene->RenderObjects(shaderProgram_shadowDepth.get());

        // ── Reflection Pass ────────────────────────────────────────────────────
        // 鏡像相機翻轉至水面以下，只渲染水面以上的物件
        {
            const Camera& cam = scene->freeViewCamera;
            float wh = WaterPlane::HEIGHT;

            glm::vec3 refPos   = glm::vec3(cam.Position.x,
                                           2.0f * wh - cam.Position.y,
                                           cam.Position.z);
            glm::vec3 refFront = glm::vec3(cam.Front.x, -cam.Front.y, cam.Front.z);
            glm::vec3 refUp    = glm::vec3(cam.Up.x,    -cam.Up.y,    cam.Up.z);
            glm::mat4 refView  = glm::lookAt(refPos, refPos + refFront, refUp);

            // Resize if viewport changed
            if (waterPlane->reflectionFBO->width  != viewportFramebuffer->width ||
                waterPlane->reflectionFBO->height != viewportFramebuffer->height)
            {
                waterPlane->reflectionFBO->ResizeFramebuffer(
                    viewportFramebuffer->width, viewportFramebuffer->height);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, waterPlane->reflectionFBO->fbo);
            glViewport(0, 0, waterPlane->reflectionFBO->width, waterPlane->reflectionFBO->height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // Reflection flips winding — cull front faces so back-faces render correctly
            glCullFace(GL_FRONT);

            const LightData& lt = scene->light;
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, shadowMapBuffer->depthTexture);

            shaderProgram_worldObject->use();
            shaderProgram_worldObject->setUnifInt  ("shadowMap",        4);
            shaderProgram_worldObject->setUnifMat4 ("lightSpaceMatrix", lightSpaceMatrix);
            shaderProgram_worldObject->setUnifInt  ("lightType",        static_cast<int>(lt.type));
            shaderProgram_worldObject->setUnifVec3 ("lightPosition",    lt.position.x,  lt.position.y,  lt.position.z);
            shaderProgram_worldObject->setUnifVec3 ("lightDirection",   lt.direction.x, lt.direction.y, lt.direction.z);
            shaderProgram_worldObject->setUnifVec3 ("lightColor",       lt.color.x,     lt.color.y,     lt.color.z);
            shaderProgram_worldObject->setUnifFloat("lightIntensity",   lt.intensity);
            shaderProgram_worldObject->setUnifFloat("shadowBias",       lt.shadowBias);
            // Clip plane: only render geometry above water (y > wh)
            shaderProgram_worldObject->setUnifVec4 ("clipPlane",        0.0f, 1.0f, 0.0f, -wh);

            glEnable(GL_CLIP_DISTANCE0);
            scene->RenderObjects(shaderProgram_worldObject.get(), refView);
            skybox->Draw(shaderProgram_skybox.get(), refView,
                         scene->freeViewCamera.GetProjectionMatrix());
            glDisable(GL_CLIP_DISTANCE0);

            glCullFace(GL_BACK);
        }

        // ── Main Phong Render Pass ─────────────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, viewportFramebuffer->fbo);
        glViewport(0, 0, viewportFramebuffer->width, viewportFramebuffer->height);
        glClearColor(0.15f, 0.4f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene->freeViewCamera.SetProjectionMatrix(viewportFramebuffer->width, viewportFramebuffer->height);

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
        shaderProgram_worldObject->setUnifVec4 ("clipPlane",       0.0f, 0.0f, 0.0f, 0.0f);  // disabled

        scene->RenderObjects(shaderProgram_worldObject.get());

        skybox->Draw(
            shaderProgram_skybox.get(),
            scene->freeViewCamera.GetViewMatrix(),
            scene->freeViewCamera.GetProjectionMatrix()
        );

        // ── Water Surface (semi-transparent, after skybox, before particles) ──
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);  // don't write depth for transparent water

            float time = static_cast<float>(glfwGetTime());
            glm::vec3 lightDir = (lt.type == LightData::Type::Directional)
                               ? lt.direction
                               : glm::normalize(lt.position - glm::vec3(0.0f));

            waterPlane->Draw(
                shaderProgram_water.get(),
                scene->freeViewCamera.GetViewMatrix(),
                scene->freeViewCamera.GetProjectionMatrix(),
                scene->freeViewCamera.Position,
                time,
                waterPlane->reflectionFBO->colorTexture,
                lightDir, lt.color * lt.intensity
            );

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

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
