#include <memory>
#include <iostream>

#include "../src/App/App.h"

// 程式進入點：建立應用程式實例並依序執行初始化 → 主迴圈 → 清理
int main()
{
	std::cout << "App start!" << std::endl;

	std::unique_ptr<CG::App> app = std::make_unique<CG::App>();

	app->Initialize();  // 初始化 GLFW、GLEW、ImGui 及所有子系統
	app->Loop();        // 進入主迴圈，直到視窗關閉
	app->Terminate();   // 釋放所有 GPU 資源並關閉視窗

	std::cout << "App end!" << std::endl;

	return 0;
}
