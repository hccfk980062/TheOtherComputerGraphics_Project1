#include "Skybox/Skybox.h"
#include "Shader/Shader.h"

#include "stb_image.h"

#include <iostream>
#include <vector>
#include <cstring>

namespace CG
{
    // 標準天空盒單位立方體（36 個頂點，無索引）
    static const float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f
    };

    Skybox::Skybox(const char* crossTexturePath)
    {
        cubemapTexture = LoadCubemapFromCross(crossTexturePath);
        SetupMesh();
    }

    Skybox::~Skybox()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteTextures(1, &cubemapTexture);
    }

    void Skybox::Draw(Shader* shader, const glm::mat4& view, const glm::mat4& projection)
    {
        // 天空盒在所有不透明物件之後繪製，深度函數改為 LEQUAL 以通過深度測試
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        shader->use();
        shader->setUnifMat4("view", view);
        shader->setUnifMat4("projection", projection);
        shader->setUnifInt("skybox", 0);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // 還原深度狀態，避免影響後續渲染
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    unsigned int Skybox::LoadCubemapFromCross(const char* path)
    {
        // Cubemap 不需要垂直翻轉
        stbi_set_flip_vertically_on_load(false);

        int imgW, imgH, channels;
        unsigned char* data = stbi_load(path, &imgW, &imgH, &channels, 0);

        if (!data)
        {
            std::cerr << "Skybox: 無法載入貼圖 " << path << std::endl;
            stbi_set_flip_vertically_on_load(true);
            return 0;
        }

        // 十字形排列（4 欄 × 3 列），每面大小 = imgW/4 × imgH/3
        int faceW = imgW / 4;
        int faceH = imgH / 3;

        // 各面在十字圖中的欄列位置
        // 標準水平十字：+Y(1,0)  -X(0,1) +Z(1,1) +X(2,1) -Z(3,1)  -Y(1,2)
        struct FaceDesc { GLenum target; int col, row; };
        FaceDesc faces[] = {
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, 2, 1 },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 1 },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 1, 0 },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 1, 2 },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 1, 1 },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 3, 1 },
        };

        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

        std::vector<unsigned char> facePixels(faceW * faceH * channels);
        GLenum fmt = (channels == 3) ? GL_RGB : GL_RGBA;

        for (auto& f : faces)
        {
            int xOff = f.col * faceW;
            int yOff = f.row * faceH;

            for (int y = 0; y < faceH; y++)
            {
                for (int x = 0; x < faceW; x++)
                {
                    int srcIdx = ((yOff + y) * imgW + (xOff + x)) * channels;
                    int dstIdx = (y * faceW + x) * channels;
                    std::memcpy(&facePixels[dstIdx], &data[srcIdx], channels);
                }
            }

            glTexImage2D(f.target, 0, fmt, faceW, faceH, 0, fmt, GL_UNSIGNED_BYTE, facePixels.data());
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        stbi_image_free(data);

        // 恢復 2D 貼圖所需的翻轉設定
        stbi_set_flip_vertically_on_load(true);

        std::cout << "Skybox: 載入完成 " << path
                  << " (face " << faceW << "x" << faceH << ")" << std::endl;
        return texID;
    }

    void Skybox::SetupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

        // location 0：頂點位置（同時作為 cubemap 取樣方向）
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);
    }
}
