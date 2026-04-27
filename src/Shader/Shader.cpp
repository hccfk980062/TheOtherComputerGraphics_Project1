#include "Shader.h"

namespace CG
{
    Shader::Shader(const char* vertexPath, const char* fragmentPath)
    {
        // ── Step 1：從檔案讀取著色器原始碼 ──────────────────────────────────
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // 設定 ifstream 例外，確保檔案開啟失敗時能被 catch 捕捉
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // 將整個檔案內容讀入字串流
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode   = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (const std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ at " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // ── Step 2：編譯著色器 ───────────────────────────────────────────────
        int  success;
        char infoLog[512];

        // 編譯頂點著色器
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        };

        // 編譯片段著色器
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // ── Step 3：連結著色器程式 ───────────────────────────────────────────
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        // 連結完成後，個別著色器物件已不需要，可釋放 GPU 資源
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

    // ── Uniform 設定函式：每次都透過名稱查詢 location，效率足夠於本專案規模 ──

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

    // 指標版本：支援上傳單一 vec3 或 vec3 陣列（count = 元素數量）
    void Shader::setUnifVec3(const std::string& UnifVarname, GLfloat* value, int count)
    {
        glUniform3fv(glGetUniformLocation(ID, UnifVarname.c_str()), count, value);
    }

    void Shader::setUnifVec4(const std::string& UnifVarname, float v0, float v1, float v2, float v3)
    {
        glUniform4f(glGetUniformLocation(ID, UnifVarname.c_str()), v0, v1, v2, v3);
    }

    // GL_FALSE：不需要轉置，glm 的矩陣已是列主序（column-major），與 OpenGL 一致
    void Shader::setUnifMat4(const std::string& UnifVarname, glm::mat4 matrix_4x4)
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, UnifVarname.c_str()), 1, GL_FALSE, &matrix_4x4[0][0]);
    }
}
