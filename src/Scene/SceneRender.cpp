#include "Scene/SceneRenderer.h"
#include <iostream>

namespace CG
{
    auto SceneRenderer::Initialize(int width, int height) -> bool
    {
        viewportFramebuffer    = std::make_unique<Framebuffer>(width, height);
        pickingFramebuffer     = std::make_unique<Framebuffer>(width, height);
        postProcessFramebuffer = std::make_unique<Framebuffer>(width, height);
        motionBlurFramebuffer  = std::make_unique<Framebuffer>(width, height);
        shadowMapBuffer        = std::make_unique<ShadowMap>(2048, 2048);

        shaderProgram_particle    = std::make_unique<Shader>("ShaderPrograms/Particle_vertex.vert",              "ShaderPrograms/Particle_fragement.frag");
        shaderProgram_trail       = std::make_unique<Shader>("ShaderPrograms/Trail_vertex.vert",                 "ShaderPrograms/Trail_fragment.frag");
        shaderProgram_worldObject = std::make_unique<Shader>("ShaderPrograms/shader_worldObject_vertex.vert",   "ShaderPrograms/shader_worldObject_fragment.frag");
        shaderProgram_picking     = std::make_unique<Shader>("ShaderPrograms/shader_picking_vertex.vert",       "ShaderPrograms/shader_picking_fragment.frag");
        shaderProgram_shadowDepth = std::make_unique<Shader>("ShaderPrograms/shader_shadow_depth_vertex.vert",  "ShaderPrograms/shader_shadow_depth_fragment.frag");
        shaderProgram_skybox      = std::make_unique<Shader>("ShaderPrograms/skybox_vertex.vert",               "ShaderPrograms/skybox_fragment.frag");
        shaderProgram_water       = std::make_unique<Shader>("ShaderPrograms/water_vertex.vert",                 "ShaderPrograms/water_fragment.frag");
        shaderProgram_mosaic      = std::make_unique<Shader>("ShaderPrograms/shader_mosaic_vertex.vert",        "ShaderPrograms/shader_mosaic_fragment.frag");
        shaderProgram_envmap      = std::make_unique<Shader>("ShaderPrograms/shader_envmap_vertex.vert",        "ShaderPrograms/shader_envmap_fragment.frag");
        shaderProgram_velocity    = std::make_unique<Shader>("ShaderPrograms/shader_velocity_vertex.vert",      "ShaderPrograms/shader_velocity_fragment.frag");
        shaderProgram_motionBlur  = std::make_unique<Shader>("ShaderPrograms/shader_motionblur_vertex.vert",    "ShaderPrograms/shader_motionblur_fragment.frag");
        skybox = std::make_unique<Skybox>("Textures/skyboxsun5deg.png");
        waterPlane = std::make_unique<WaterPlane>();
        waterPlane->Initialize(width, height);
        InitScreenQuad();
        InitVelocityBuffer(width, height);
        return true;
    }

    void SceneRenderer::InitScreenQuad()
    {
        // 覆蓋整個 NDC 的全螢幕四邊形（兩個三角形），UV 原點在左下
        constexpr float verts[] = {
            // pos (x, y)   UV (u, v)
            -1.0f,  1.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
            -1.0f,  1.0f,   0.0f, 1.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f,
        };
        glGenVertexArrays(1, &screenQuadVAO);
        glGenBuffers(1, &screenQuadVBO);
        glBindVertexArray(screenQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void SceneRenderer::ApplyMosaicPass()
    {
        // 確保 postProcessFBO 大小與 scene FBO 一致
        if (postProcessFramebuffer->width  != viewportFramebuffer->width ||
            postProcessFramebuffer->height != viewportFramebuffer->height)
        {
            postProcessFramebuffer->ResizeFramebuffer(
                viewportFramebuffer->width, viewportFramebuffer->height);
        }

        // 若動態模糊已啟用，馬賽克應讀取模糊後的結果；否則直接讀取場景 FBO
        GLuint srcTexture = (motionBlurEnabled && !m_motionBlurFirstFrame)
            ? motionBlurFramebuffer->colorTexture
            : viewportFramebuffer->colorTexture;

        glBindFramebuffer(GL_FRAMEBUFFER, postProcessFramebuffer->fbo);
        glViewport(0, 0, postProcessFramebuffer->width, postProcessFramebuffer->height);
        glDisable(GL_DEPTH_TEST);

        shaderProgram_mosaic->use();
        shaderProgram_mosaic->setUnifInt("u_pixelSize", mosaicPixelSize);
        glUniform2f(glGetUniformLocation(shaderProgram_mosaic->ID, "u_resolution"),
            static_cast<float>(viewportFramebuffer->width),
            static_cast<float>(viewportFramebuffer->height));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTexture);
        shaderProgram_mosaic->setUnifInt("u_screenTexture", 0);

        glBindVertexArray(screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
        // ── FBO 尺寸同步 ─────────────────────────────────────────────────────
        // getCurrentViewportFramebuffer() 回傳的是「最終輸出」FBO，
        // 由 App SyncFramebufferSize 根據 ImGui 視窗大小調整它的尺寸。
        // 所有中間 FBO 都必須同步至相同尺寸，確保各 pass 的解析度一致。
        if (mosaicEnabled && mosaicPixelSize > 1)
        {
            // 馬賽克 = 最終輸出，postProcessFBO 驅動尺寸
            if (viewportFramebuffer->width  != postProcessFramebuffer->width ||
                viewportFramebuffer->height != postProcessFramebuffer->height)
            {
                viewportFramebuffer->ResizeFramebuffer(
                    postProcessFramebuffer->width, postProcessFramebuffer->height);
            }
        }
        else if (motionBlurEnabled)
        {
            // 動態模糊 = 最終輸出（無馬賽克時），motionBlurFBO 驅動尺寸
            if (viewportFramebuffer->width  != motionBlurFramebuffer->width ||
                viewportFramebuffer->height != motionBlurFramebuffer->height)
            {
                viewportFramebuffer->ResizeFramebuffer(
                    motionBlurFramebuffer->width, motionBlurFramebuffer->height);
            }
        }

        // 當動態模糊啟用時，確保 motionBlurFBO 與 velocityFBO 尺寸與 viewportFBO 一致
        if (motionBlurEnabled)
        {
            if (motionBlurFramebuffer->width  != viewportFramebuffer->width ||
                motionBlurFramebuffer->height != viewportFramebuffer->height)
            {
                motionBlurFramebuffer->ResizeFramebuffer(
                    viewportFramebuffer->width, viewportFramebuffer->height);
            }
            ResizeVelocityBuffer(viewportFramebuffer->width, viewportFramebuffer->height);
        }

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

        if (cubeEnvMode == 0)
        {
            scene->RenderObjects(shaderProgram_worldObject.get());
        }
        else
        {
            // Phong pass for all objects except the cube
            scene->RenderObjectsExceptCube(shaderProgram_worldObject.get());

            // Env map pass for the cube only
            shaderProgram_envmap->use();
            shaderProgram_envmap->setUnifMat4("view",       scene->freeViewCamera.GetViewMatrix());
            shaderProgram_envmap->setUnifMat4("projection", scene->freeViewCamera.GetProjectionMatrix());
            shaderProgram_envmap->setUnifVec3("cameraPos",
                scene->freeViewCamera.Position.x,
                scene->freeViewCamera.Position.y,
                scene->freeViewCamera.Position.z);
            shaderProgram_envmap->setUnifInt  ("envMode",      cubeEnvMode);
            shaderProgram_envmap->setUnifFloat("refractRatio", cubeRefractRatio);
            shaderProgram_envmap->setUnifInt  ("skybox",       1);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->getCubemapTexture());

            scene->RenderCubeOnly(shaderProgram_envmap.get());

            glActiveTexture(GL_TEXTURE0);  // restore for skybox draw
        }

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

        // ── Motion Blur Post-Process Pass（選用）─────────────────────────────
        // 流程：
        //   1. 速度 Pass：以當前 + 前一幀的矩陣渲染每物件的螢幕空間速度向量
        //   2. 模糊 Pass：沿速度方向多次取樣 viewportFBO 以產生拖影效果
        if (motionBlurEnabled)
        {
            if (!m_motionBlurFirstFrame)
            {
                RenderVelocityPass(scene);
                ApplyMotionBlurPass();
            }

            // 更新前一幀矩陣（供下一幀的速度計算使用）
            glm::mat4 currVP = scene->freeViewCamera.GetProjectionMatrix()
                             * scene->freeViewCamera.GetViewMatrix();
            m_prevViewProjection = currVP;

            for (auto* obj : scene->ObjectList)
            {
                if (obj->model != nullptr)
                    m_prevWorldMatrices[obj->id] = obj->GetWorldMatrix();
            }

            m_motionBlurFirstFrame = false;
        }

        // ── Mosaic Post-Process Pass（選用）────────────────────────────────────
        if (mosaicEnabled && mosaicPixelSize > 1)
            ApplyMosaicPass();
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
        // 後製優先順序（由高到低）：馬賽克 > 動態模糊 > 原始場景
        // 越後面的 pass 越靠近最終輸出，其 FBO 是 SyncFramebufferSize 的目標
        if (mosaicEnabled && mosaicPixelSize > 1)
            return postProcessFramebuffer.get();
        if (motionBlurEnabled)
            return motionBlurFramebuffer.get();
        return viewportFramebuffer.get();
    }

    // ── 動態模糊輔助方法 ──────────────────────────────────────────────────────

    // 建立 RG16F 速度緩衝 FBO（僅在 Initialize 時呼叫一次）
    void SceneRenderer::InitVelocityBuffer(int w, int h)
    {
        velocityFBOWidth  = w;
        velocityFBOHeight = h;

        glGenFramebuffers(1, &velocityFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, velocityFBO);

        // RG16F：兩通道帶號浮點貼圖，用於儲存 X/Y 方向速度
        glGenTextures(1, &velocityColorTex);
        glBindTexture(GL_TEXTURE_2D, velocityColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocityColorTex, 0);

        // 深度 RBO（速度 pass 需要深度測試，避免後方物件覆蓋前方速度）
        glGenRenderbuffers(1, &velocityDepthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, velocityDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, velocityDepthRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[VelocityFBO] FBO incomplete at initialization!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 當視窗尺寸改變時重新分配速度 FBO 的 GPU 記憶體
    void SceneRenderer::ResizeVelocityBuffer(int w, int h)
    {
        if (w <= 0 || h <= 0 || (w == velocityFBOWidth && h == velocityFBOHeight))
            return;

        velocityFBOWidth  = w;
        velocityFBOHeight = h;

        glBindTexture(GL_TEXTURE_2D, velocityColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, nullptr);

        glBindRenderbuffer(GL_RENDERBUFFER, velocityDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, velocityFBO);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[VelocityFBO] FBO incomplete after resize!\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 速度 Pass：以當前幀 + 前一幀的矩陣渲染每個物件，輸出螢幕空間速度向量至 velocityFBO
    void SceneRenderer::RenderVelocityPass(MainScene* scene)
    {
        int w = viewportFramebuffer->width;
        int h = viewportFramebuffer->height;

        glBindFramebuffer(GL_FRAMEBUFFER, velocityFBO);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // 靜止物件速度為 (0, 0)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // u_prevVP 整場景只需設一次（相機運動對所有物件相同）
        shaderProgram_velocity->use();
        shaderProgram_velocity->setUnifMat4("u_prevVP", m_prevViewProjection);

        // 各物件逐一以當前 + 前幀 Model Matrix 渲染（scene 內部設定 view / projection）
        scene->RenderObjectsForVelocity(shaderProgram_velocity.get(), m_prevWorldMatrices);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 動態模糊 Pass：讀取速度貼圖，沿速度方向對場景顏色多次取樣並平均
    void SceneRenderer::ApplyMotionBlurPass()
    {
        int w = viewportFramebuffer->width;
        int h = viewportFramebuffer->height;

        // 確保輸出 FBO 大小正確
        if (motionBlurFramebuffer->width != w || motionBlurFramebuffer->height != h)
            motionBlurFramebuffer->ResizeFramebuffer(w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFramebuffer->fbo);
        glViewport(0, 0, w, h);
        glDisable(GL_DEPTH_TEST);

        shaderProgram_motionBlur->use();
        shaderProgram_motionBlur->setUnifInt  ("u_numSamples",    motionBlurSamples);
        shaderProgram_motionBlur->setUnifFloat("u_blurStrength",  motionBlurStrength);

        // Texture Unit 0：場景顏色（模糊的來源）
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, viewportFramebuffer->colorTexture);
        shaderProgram_motionBlur->setUnifInt("u_screenTexture", 0);

        // Texture Unit 1：速度向量（決定每像素的模糊方向與距離）
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, velocityColorTex);
        shaderProgram_motionBlur->setUnifInt("u_velocityTexture", 1);

        glBindVertexArray(screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
