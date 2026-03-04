#pragma once
#include <array>
#include <string>
#include <map>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Shader/Shader.h"
#include "Model/ModelLoader.h"

#include "Camera/Camera.h"
namespace CG
{
	class MainScene
	{
	public:
		MainScene();
		~MainScene();

		auto Initialize() -> bool;
		void Update(double dt);
		void Render(int windowWidth, int windowHeigh);

		void OnResize(int width, int height);
		void OnKeyboard(int key, int action);
		void OnMouseClick(int key, int action);
		void OnMouseDrag(double xPos, double yPos);
		void SetMode(int mode);
		Camera	freeViewCamera;
	private:

		GLenum mode = GL_FILL; // GL_FILL or GL_LINE

		GLuint VertexBufferObject;
		GLuint VertexArrayObject;
		GLuint VertexElementObject;

		Shader* defaultShader;
		Shader* shaderProgram_worldObject;

		std::array<float, 3> p0c; // Color of P0
		std::array<float, 3> p1c; // Color of P1
		std::array<float, 3> p2c; // Color of P2

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		Model* model_Train;

		std::array<bool, 4>pressedButton;//W,S,A,D
		bool flag_RightButtonDown = false;


		double last_Xpos = 0;
		double last_Ypos = 0;
	};
}

