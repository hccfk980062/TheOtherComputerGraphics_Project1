#include <iostream>
#include <functional>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>

#include "App.h"

namespace CG
{
    App::App() : mainWindow(nullptr) {}
    App::~App() {}

    auto App::Initialize() -> bool
    {
        // 設定 GLFW 錯誤回呼，方便除錯
        glfwSetErrorCallback([](int error, const char* desc)
            { fprintf(stderr, "GLFW Error %d: %s\n", error, desc); });

        if (!glfwInit()) return false;

        // 要求 OpenGL 4.6 相容性設定檔（Compatibility Profile 支援舊式 GL 呼叫）
        const char* glsl_version = "#version 460";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

        mainWindow = glfwCreateWindow(1280, 720, "cg-gui", nullptr, nullptr);
        if (!mainWindow) return false;

        glfwMakeContextCurrent(mainWindow);
        glfwSwapInterval(1);  // 開啟垂直同步，限制最高幀率為螢幕更新率

        // 初始化 GLEW（必須在 context 建立後才能呼叫）
        glewExperimental = GL_TRUE;
        GLenum glew_err = glewInit();
        if (glew_err != GLEW_OK)
            throw std::runtime_error(std::string("GLEW init error: ") + (const char*)glewGetErrorString(glew_err));

        // ── 初始化 ImGui ────────────────────────────────────────────────────
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // 啟用視窗停靠功能
        io.FontGlobalScale = 1.5f;  // 全域字體放大，提高 HiDPI 可讀性

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // 將 this 指標存入 GLFW 使用者資料，供回呼函式取用
        glfwSetWindowUserPointer(mainWindow, this);

        // ── 建立核心子系統 ───────────────────────────────────────────────────
        sceneRenderer = std::make_unique<SceneRenderer>();
        sceneRenderer->Initialize(1280, 720);

        mainScene = std::make_unique<MainScene>();
        mainScene->Initialize();

        // ── 建立並連接各 UI 視窗 ─────────────────────────────────────────────
        viewportWindow = std::make_unique<ViewportWindow>();
        viewportWindow->Initialize();
        viewportWindow->SetCommandStack(&commandStack);
        viewportWindow->SetEventBus(&eventBus);
        viewportWindow->SetSceneRenderer(sceneRenderer.get());

        inspectorWindow = std::make_unique<InspectorWindow>();
        inspectorWindow->Initialize();
        inspectorWindow->SetTargetScene(mainScene.get());

        hierarchyWindow = std::make_unique<HierarchyWindow>();
        hierarchyWindow->Initialize();
        hierarchyWindow->SetTargetScene(mainScene.get());

        sequencerWindow = std::make_unique<SequencerWindow>();
        sequencerWindow->Initialize();
        sequencerWindow->SetTargetScene(mainScene.get());
        sequencerWindow->SetCommandStack(&commandStack);

        // ── 訂閱「物件選取」事件：同步更新 mainScene->selectedObject ─────────
        // ViewportWindow（顏色拾取）和 HierarchyWindow（樹狀點擊）都會發布此事件
        eventBus.Subscribe<ObjectSelectedEvent>([this](const ObjectSelectedEvent& e)
        {
            if (e.object)
                mainScene->selectedObject = e.object;
        });

        printf("ImGui version: %s (%d)\n", IMGUI_VERSION, IMGUI_VERSION_NUM);
        return true;
    }

    void App::Loop()
    {
        while (!glfwWindowShouldClose(mainWindow))
        {
            glfwPollEvents();

            // 計算幀間時間差（delta time）
            timeNow   = glfwGetTime();
            timeDelta = timeNow - timeLast;
            timeLast  = timeNow;

            // ── 固定頻率更新（60 Hz）：累積超過 FIXED_DT 才觸發一次 ──────────
            m_accumulator += timeDelta;
            while (m_accumulator >= FIXED_DT)
            {
                FixedUpdate(FIXED_DT);
                m_accumulator -= FIXED_DT;
            }

            // ── 將 FBO 大小同步至上一幀 Viewport 視窗大小，然後渲染場景 ───────
            viewportWindow->SyncFramebufferSize(sceneRenderer->getCurrentViewportFramebuffer());
            sceneRenderer->RenderScene(mainScene.get());

            // ── 清除主視窗（螢幕 FBO），只需清顏色（深度由各 FBO 自己管）───────
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            int display_w, display_h;
            glfwGetFramebufferSize(mainWindow, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // ── 開始 ImGui 新幀 ───────────────────────────────────────────────
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            ProcessShortcuts();

            // 建立 DockSpace 並更新所有 UI 視窗
            BeginDockspace();
            viewportWindow->UpdateScreen(mainScene.get(), sceneRenderer->getCurrentViewportFramebuffer());
            inspectorWindow->Display();
            hierarchyWindow->Display();
            sequencerWindow->Display();

            // 將 ImGui 繪製資料提交至 GPU
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(mainWindow);
        }
    }

    void App::Terminate()
    {
        // 依序釋放 ImGui 後端、Context，再銷毀 GLFW 視窗與整個 GLFW
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
    }

    // 固定頻率邏輯更新：目前只驅動 FABRIK IK 解算
    void App::FixedUpdate(double /*fixedDt*/)
    {
        if (mainScene) mainScene->SolveIK();
    }

    // 全域快捷鍵處理：WantTextInput 為 true 時（使用者正在輸入文字）不攔截
    void App::ProcessShortcuts()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput) return;

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
            commandStack.Undo();  // Ctrl+Z：復原上一個命令
        else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
            commandStack.Redo();  // Ctrl+Y：重做下一個命令
    }

    // 建立覆蓋整個主視窗的透明 DockSpace 容器
    // ImGuiDockNodeFlags_PassthruCentralNode：中央空白區域不擋住 OpenGL 渲染
    void App::BeginDockspace()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar        |
            ImGuiWindowFlags_NoCollapse        |
            ImGuiWindowFlags_NoResize          |
            ImGuiWindowFlags_NoMove            |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus        |
            ImGuiWindowFlags_NoBackground      |
            ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("DockSpace", nullptr, host_flags);
        ImGui::PopStyleVar();

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }
}
