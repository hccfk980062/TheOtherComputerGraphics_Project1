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
	App::App()
	{
		mainWindow = nullptr;
		mainScene = nullptr;
	}

	App::~App()
	{

	}

	auto App::Initialize() -> bool
	{
		// Set error callback
		glfwSetErrorCallback([](int error, const char* description)
			{ fprintf(stderr, "GLFW Error %d: %s\n", error, description); });

		// Initialize GLFW
		if (!glfwInit())
			return false;

		// GL 4.6 + GLSL 460
		const char* glsl_version = "#version 460";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

		// Create window with graphics context
		mainWindow = glfwCreateWindow(1280, 720, "cg-gui", nullptr, nullptr);
		if (mainWindow == nullptr)
			return false;
		glfwMakeContextCurrent(mainWindow);
		glfwSwapInterval(1); // 影格率將被限制在螢幕刷新率 (通常是 60/144Hz)

		// Initialize GLEW
		glewExperimental = GL_TRUE;
		GLenum glew_err = glewInit();
		if (glew_err != GLEW_OK)
		{
			throw std::runtime_error(std::string("Error initializing GLEW, error: ") + (const char*)glewGetErrorString(glew_err));
			return false;
		}

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		io.FontGlobalScale = 1.5f;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		glfwSetWindowUserPointer(mainWindow, this);

		sceneRenderer = new SceneRenderer();
		sceneRenderer->Initialize(1280, 720);

		viewportWindow = new ViewportWindow();
		viewportWindow->Initialize();

		inspectorWindow = new InspectorWindow();
		inspectorWindow->Initialize();

		hierarchyWindow = new HierarchyWindow();
		hierarchyWindow->Initialize();

		sequencerWindow = new SequencerWindow();
		sequencerWindow->Initialize();

		mainScene = new MainScene();
		mainScene->Initialize();

		inspectorWindow->SetTargetScene(mainScene);
		hierarchyWindow->SetTargetScene(mainScene);
		sequencerWindow -> SetTargetScene(mainScene);
		printf("Project Source Header Version: %s (%d)\n", IMGUI_VERSION, IMGUI_VERSION_NUM);
		IMGUI_CHECKVERSION();
		// Initialization done
		return true;
	}

	void App::Loop()
	{
		while (!glfwWindowShouldClose(mainWindow))
		{
			glfwPollEvents();

			timeNow = glfwGetTime();
			timeDelta = timeNow - timeLast;
			timeLast = timeNow;
			Update(timeDelta);

			// ── Step 0: 用上一幀記錄的 Viewport 尺寸，在 Render 之前同步 FBO ──────
			// 這樣可以確保 RenderScene 使用的是正確的尺寸，
			// 而不是在 render 完之後才 resize（那樣會清空剛渲染好的 texture）
			viewportWindow->SyncFramebufferSize(sceneRenderer->getCurrentViewportFramebuffer());

			// ── Step 1: Render 3D scene → FBO ────────────────────────────────────
			sceneRenderer->RenderScene(mainScene);

			// ── Step 2: 清空主視窗（作為 ImGui 的背景） ───────────────────────────
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			int display_w, display_h;
			glfwGetFramebufferSize(mainWindow, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// ── Step 3: Render ImGui UI ────────────────────────────────────────────
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();

			BeginDockspace();

			// UpdateScreen 內部會記錄本幀的 Viewport 尺寸，供下一幀 Step 0 使用
			viewportWindow->UpdateScreen(mainScene, sceneRenderer->getCurrentViewportFramebuffer());
			inspectorWindow->Display();
			hierarchyWindow->Display();
			sequencerWindow->Display();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(mainWindow);
		}
	}

	void App::Terminate()
	{
		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(mainWindow);
		glfwTerminate();
	}

	void App::Update(double dt)
	{
	}

	void App::BeginDockspace()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags host_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_MenuBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("DockSpace", nullptr, host_flags);
		ImGui::PopStyleVar();

		ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

		ImGui::End();
	}
}