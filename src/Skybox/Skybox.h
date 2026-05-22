#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace CG
{
    class Shader;

    // 從單張十字形 (Cross Layout) PNG 建立 OpenGL Cubemap 並渲染天空盒
    class Skybox
    {
    public:
        // crossTexturePath：4×3 十字形排列的天空盒貼圖路徑
        Skybox(const char* crossTexturePath);
        ~Skybox();

        // 以 GL_LEQUAL 深度測試繪製天空盒（應在所有不透明物件之後呼叫）
        void Draw(Shader* shader, const glm::mat4& view, const glm::mat4& projection);

        unsigned int getCubemapTexture() const { return cubemapTexture; }

    private:
        unsigned int VAO, VBO;
        unsigned int cubemapTexture;

        // 從十字形 PNG 提取 6 個面並上傳為 GL_TEXTURE_CUBE_MAP
        unsigned int LoadCubemapFromCross(const char* path);
        void SetupMesh();
    };
}
