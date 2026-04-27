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
    // 封裝 OpenGL GLSL 著色器程式（頂點 + 片段）的編譯、連結與 uniform 設定
    class Shader
    {
    public:
        unsigned int ID;  // 已連結的 GL 著色器程式 ID

        // 從檔案路徑讀取、編譯並連結頂點與片段著色器
        Shader(const char* vertexPath, const char* fragmentPath);
        ~Shader();

        void use();     // 啟用此著色器程式（glUseProgram）
        void unUse();   // 還原為預設（glUseProgram(0)）
        unsigned int getReference();  // 回傳程式 ID

        // ── Uniform 設定介面 ─────────────────────────────────────────────────
        void setUnifInt(const std::string& UnifVarname, int v0);
        void setUnifFloat(const std::string& UnifVarname, float v0);

        // 向量版本：直接傳三個 float 或指標（支援陣列）
        void setUnifVec3(const std::string& UnifVarname, float v0, float v1, float v2);
        void setUnifVec3(const std::string& UnifVarname, GLfloat* value, int count = 1);

        void setUnifVec4(const std::string& UnifVarname, float v0, float v1, float v2, float v3);
        void setUnifMat4(const std::string& UnifVarname, glm::mat4 matrix_4x4);
    };
}
