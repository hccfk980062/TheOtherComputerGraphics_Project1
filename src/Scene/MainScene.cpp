#include <iostream>

#include "MainScene.h"
namespace CG
{
	MainScene::MainScene()
	{
		p0c = { 1.0, 0.0, 0.0 };
		p1c = { 0.0, 1.0, 0.0 };
		p2c = { 0.0, 0.0, 1.0 };

		vertices = {
			// Front face        // Color
			-0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

			// Back face
			-0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,
		};
		indices = {
			// Front
			0, 1, 2,  2, 3, 0,
			// Back
			6, 5, 4,  4, 7, 6,
			// Left
			4, 0, 3,  3, 7, 4,
			// Right
			1, 5, 6,  6, 2, 1,
			// Top
			3, 2, 6,  6, 7, 3,
			// Bottom
			4, 5, 1,  1, 0, 4,
		};
	}

	MainScene::~MainScene()
	{
	}

	auto MainScene::Initialize() -> bool
	{
		freeViewCamera = Camera(glm::vec3(-3, 0, 0));
		freeViewCamera.configureLookAt(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));

		model_Train = new Model("objModels/Train/Train.obj");

		defaultShader = new Shader("ShaderPrograms/Shader.vert", "ShaderPrograms/Shader.frag");
		shaderProgram_worldObject = new Shader("ShaderPrograms/shader_worldObject_vertex.vert", "ShaderPrograms/shader_worldObject_fragment.frag");

		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		// 1. 先建立並綁定 VAO（讓它開始「錄製」）
		glGenVertexArrays(1, &VertexArrayObject);
		glBindVertexArray(VertexArrayObject);

		// 2. 建立 VBO 並上傳資料（此時 VAO 已在錄製）
		glGenBuffers(1, &VertexBufferObject);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		// 3. 告訴 VAO 如何解讀 VBO 資料
		// Position attribute (location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// Color attribute (location = 1)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// 4. 建立 EBO（在 VAO 綁定期間，會被自動記錄進 VAO）
		glGenBuffers(1, &VertexElementObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexElementObject);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		// 5. 解除綁定 (防止後續誤操作)
		glBindVertexArray(0);

		// 初始化場景物件
		SceneObject trainObj;
		trainObj.name = "Train Model";
		trainObj.model = model_Train;
		trainObj.objectType = 1;
		trainObj.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		sceneObjects.push_back(trainObj);

		return true;
	}

	SceneObject* MainScene::GetObjectByIndex(int index)
	{
		if (index >= 0 && index < sceneObjects.size())
			return &sceneObjects[index];
		return nullptr;
	}

	void MainScene::Update(double dt)
	{
		if (pressedButton.size() != 0)
			freeViewCamera.ProcessKeyboard(pressedButton, 0.05);
	}

	void MainScene::Render(int windowWidth, int windowHeigh)
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1); // Black background
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Start drawing
		shaderProgram_worldObject->use();

		glm::mat4 view = freeViewCamera.GetViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeigh, 0.1f, 100.0f);

		// 渲染所有場景物件
		for (const auto& obj : sceneObjects)
		{
			if (obj.model)
			{
				glm::mat4 model = obj.transform.GetModelMatrix();
				shaderProgram_worldObject->setUnifMat4("model", model);
				shaderProgram_worldObject->setUnifMat4("view", view);
				shaderProgram_worldObject->setUnifMat4("projection", proj);
				obj.model->Draw(*shaderProgram_worldObject);
			}
		}
	}

	void MainScene::OnResize(int width, int height)
	{
		std::cout << "MainScene Resize: " << width << " " << height << std::endl;
	}

	void MainScene::OnKeyboard(int key, int action)
	{
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case GLFW_KEY_W:
				pressedButton[0] = true;
				break;
			case GLFW_KEY_S:
				pressedButton[1] = true;
				break;
			case GLFW_KEY_A:
				pressedButton[2] = true;
				break;
			case GLFW_KEY_D:
				pressedButton[3] = true;
				break;
			default:
				break;
			}
		}
		if (action == GLFW_RELEASE)
		{
			switch (key)
			{
			case GLFW_KEY_W:
				pressedButton[0] = false;
				break;
			case GLFW_KEY_S:
				pressedButton[1] = false;
				break;
			case GLFW_KEY_A:
				pressedButton[2] = false;
				break;
			case GLFW_KEY_D:
				pressedButton[3] = false;
				break;
			default:
				break;
			}
		}
	}
	void MainScene::OnMouseClick(int button, int action)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS) {
				flag_RightButtonDown = true;
			}
			else if (action == GLFW_RELEASE) {
				flag_RightButtonDown = false;
			}
		}
	}
	void MainScene::OnMouseDrag(double xPos, double yPos)
	{
		if (!flag_RightButtonDown)
		{
			last_Xpos = xPos;
			last_Ypos = yPos;
		}

		if (flag_RightButtonDown) {
			// Calculate the change in position
			double delta_x = xPos - last_Xpos;
			double delta_y = last_Ypos - yPos; // Note: y-coordinates are top-to-bottom by default

			// Perform your drag operations here (e.g., move an object, rotate camera)

			freeViewCamera.ProcessMouseMovement(delta_x, delta_y);
			// Update the last position for the next callback
			last_Xpos = xPos;
			last_Ypos = yPos;
		}
	}

	void MainScene::SetMode(int mode_)
	{
		mode = mode_;
		if (mode_ == 0)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else if (mode_ == 1)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
	}
}