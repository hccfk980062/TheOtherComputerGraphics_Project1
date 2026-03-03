#pragma once
#include <array>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CG
{
	class Shader
	{
	public:
		// the program ID
		unsigned int ID;

		// constructor reads and builds the shader
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();
		// use/activate the shader
		void use();
		void unUse();
		unsigned int getReference();
		// utility uniform functions
		void setUnifInt(const std::string& UnifVarname, int v0);
		void setUnifFloat(const std::string& UnifVarname, float v0);
		void setUnifVec3(const std::string& UnifVarname, float v0, float v1, float v2);
		void setUnifVec4(const std::string& UnifVarname, float v0, float v1, float v2, float v3);
		void setUnifMat4(const std::string& UnifVarname, glm::mat4 matrix_4x4);
	};
}