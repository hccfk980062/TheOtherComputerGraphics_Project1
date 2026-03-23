#include "Shader.h"

namespace CG
{
	Shader::Shader(const char* vertexPath, const char* fragmentPath)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ at " << e.what() << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		// 2. compile shaders
		unsigned int vertex, fragment;
		int success;
		char infoLog[512];

		// vertex Shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		// print compile errors if any
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		};
		// similiar for Fragment Shader
		// fragment shader
		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
		glCompileShader(fragmentShader);
		// check for shader compile errors
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		}

		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragmentShader);
		glLinkProgram(ID);
		// print linking errors if any
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(fragmentShader);
		std::cout << "Shader compiled\n" << std::endl;
	}
	Shader::~Shader()
	{
		glDeleteProgram(ID);
	}
	void Shader::use()
	{
		glUseProgram(ID);
	}
	void Shader::unUse()
	{
		glUseProgram(0);
	}
	unsigned int Shader::getReference()
	{
		return ID;
	}
	void Shader::setUnifInt(const std::string& UnifVarname, int v0)
	{
		glUniform1i(glGetUniformLocation(ID, UnifVarname.c_str()), v0);
	}
	void Shader::setUnifFloat(const std::string& UnifVarname, float v0)
	{
		glUniform1f(glGetUniformLocation(ID, UnifVarname.c_str()), v0);
	}
	void Shader::setUnifVec3(const std::string& UnifVarname, float v0, float v1, float v2)
	{
		glUniform3f(glGetUniformLocation(ID, UnifVarname.c_str()), v0, v1, v2);
	}
	void Shader::setUnifVec3(const std::string& UnifVarname, GLfloat *value, int count)
	{
		glUniform3fv(glGetUniformLocation(ID, UnifVarname.c_str()), count, value);
	}
	void Shader::setUnifVec4(const std::string& UnifVarname, float v0, float v1, float v2, float v3)
	{
		glUniform4f(glGetUniformLocation(ID, UnifVarname.c_str()), v0, v1, v2, v3);
	}
	void Shader::setUnifMat4(const std::string& UnifVarname, glm::mat4 matrix_4x4)
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, UnifVarname.c_str()), 1, GL_FALSE, &matrix_4x4[0][0]);
	}
}