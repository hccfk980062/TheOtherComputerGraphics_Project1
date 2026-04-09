/**
 * @file ModelLoader.h
 * @brief 使用 Assimp 載入 3D 模型，並管理所有 Mesh 與貼圖資源
 *
 * 主要職責：
 *  1. 透過 Assimp 解析模型檔案（.obj / .fbx / .gltf …）
 *  2. 遞迴走訪場景節點，將每個 aiMesh 轉換為 CG::Mesh
 *  3. 快取已載入的貼圖（textures_loaded），避免重複上傳 GPU
 *  4. 對外提供 DrawInstanced()，一次繪製多個相同模型的 instance
 */

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Model/MeshLoader.h"
#include "../Shader/Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

namespace CG
{
    unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

    // =========================================================================
    // [修正] 新增：從記憶體載入內嵌貼圖並上傳至 GPU
    //
    // FBX 內嵌貼圖有兩種格式：
    //  - mHeight == 0 : 壓縮格式（PNG / JPG），mWidth 為資料的位元組大小
    //                   → 使用 stbi_load_from_memory() 解碼
    //  - mHeight  > 0 : 未壓縮的 Raw BGRA 資料
    //                   → 手動轉換為 RGBA 後上傳
    // =========================================================================
    inline unsigned int TextureFromMemory(const aiTexture* tex)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = nullptr;
        bool isRaw = false;

        if (tex->mHeight == 0)
        {
            // 壓縮格式（PNG / JPG …）
            data = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(tex->pcData),
                static_cast<int>(tex->mWidth),
                &width, &height, &nrComponents, 0
            );
        }
        else
        {
            // 未壓縮 Raw BGRA：Assimp 的 aiTexel 欄位順序為 b, g, r, a
            // 轉換為 OpenGL 期望的 RGBA 順序
            width = static_cast<int>(tex->mWidth);
            height = static_cast<int>(tex->mHeight);
            nrComponents = 4;
            isRaw = true;

            data = new unsigned char[width * height * 4];
            for (int i = 0; i < width * height; i++)
            {
                data[i * 4 + 0] = tex->pcData[i].r;
                data[i * 4 + 1] = tex->pcData[i].g;
                data[i * 4 + 2] = tex->pcData[i].b;
                data[i * 4 + 3] = tex->pcData[i].a;
            }
        }

        if (data)
        {
            GLenum format = GL_RGBA;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 3) format = GL_RGB;
            else if (nrComponents == 4) format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format,
                width, height, 0,
                format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // 壓縮格式由 stbi 配置記憶體；Raw 格式由上方 new[] 配置
            if (isRaw) delete[] data;
            else       stbi_image_free(data);
        }
        else
        {
            std::cout << "ERROR: 內嵌貼圖解碼失敗。" << std::endl;
        }

        return textureID;
    }

    // =========================================================================
    // Model 類別
    // =========================================================================

	/**
	 * @class Model
	 * @brief 代表一個完整的 3D 模型，由多個 Mesh 組合而成
	 *
	 * 使用方式：
	 * @code
	 *   CG::Model myModel("assets/backpack/backpack.obj");
	 *   // 每幀
	 *   myModel.DrawInstanced(shader, instanceMatrices);
	 * @endcode
	 */
    class Model
    {
    public:
		// ----- 公開資料成員 -----
		vector<MeshTexture> textures_loaded;  ///< 已載入貼圖的快取，key = 檔案路徑
		vector<Mesh>        meshes;           ///< 模型包含的所有 Mesh
		string              directory;        ///< 模型檔案所在資料夾（用於解析相對貼圖路徑）
		bool gammaCorrection;                 ///< 是否對貼圖套用 Gamma 校正
		bool useTextures;                     ///< false = 只用材質顏色，不綁定貼圖

		/**
		 * @brief 建構子：載入模型檔案並完成 GPU 資源初始化
		 * @param path        模型檔案的完整路徑
		 * @param gamma       是否啟用 Gamma 校正
		 * @param useTextures 是否載入並使用貼圖
		 */
        Model(string const& path, bool gamma = false, bool useTextures = true) : gammaCorrection(gamma), useTextures(useTextures)
        {
			defaultTextureID = createDefaultTexture();  // 預先建立 1×1 純白貼圖備用
            loadModel(path);
        }


		/**
		 * @brief 以 Instanced Rendering 繪製此模型的多個副本
		 *
		 * 將 modelMatrices 傳給每個 Mesh 的 DrawInstanced()，
		 * 一次 Draw Call 繪製 modelMatrices.size() 個 instance。
		 *
		 * @param shader        使用的 Shader 程式
		 * @param modelMatrices 每個 instance 的 Model Matrix 陣列
		 */
        void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& modelMatrices)
        {
            for (auto& mesh : meshes)
                mesh.DrawInstanced(shader, modelMatrices, useTextures, defaultTextureID);
        }

    private:
		unsigned int defaultTextureID = 0;  ///< 1×1 純白貼圖的 GPU ID，用於無貼圖模式

		// =====================================================================
		// 私有工具函式
		// =====================================================================

		/**
		 * @brief 建立一張 1×1 的純白 RGBA 貼圖
		 *
		 * 當模型沒有貼圖，或 useTextures = false 時，
		 * 使用此貼圖確保 Shader 仍能正常取樣（顏色完全由材質 diffuse 決定）。
		 *
		 * @return 產生的 OpenGL 紋理物件 ID
		 */
        unsigned int createDefaultTexture()
        {
            unsigned int texID;
            glGenTextures(1, &texID);
            glBindTexture(GL_TEXTURE_2D, texID);

			// 上傳單一白色像素（RGBA = 255, 255, 255, 255）
            unsigned char whitePixel[4] = { 255, 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

			// 設定取樣參數
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            return texID;
        }

		/**
		 * @brief 透過 Assimp 讀取模型檔案並啟動節點遞迴
		 *
		 * Assimp 後處理選項：
		 *  - aiProcess_Triangulate       : 將所有多邊形三角化
		 *  - aiProcess_GenSmoothNormals  : 自動生成平滑法線（若模型沒有法線）
		 *  - aiProcess_FlipUVs           : 翻轉 V 軸（OpenGL UV 原點在左下）
		 *  - aiProcess_CalcTangentSpace  : 自動計算 Tangent / Bitangent
		 *
		 * @param path 模型檔案路徑
		 */
        void loadModel(string const& path)
        {
			std::cout << "Loading 3D Model: " << path << std::endl;

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                path,
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace
            );

			// 驗證場景是否有效（場景為空、旗標不完整、或根節點不存在都視為錯誤）
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
                return;
            }

            // 擷取模型所在資料夾，供後續解析相對貼圖路徑使用
            // [修正] 同時處理 '/' 與 '\' 兩種路徑分隔符號
            directory = path.substr(0, path.find_last_of("/\\"));

			// 從根節點開始遞迴處理整棵場景樹
            processNode(scene->mRootNode, scene);
        }

		/**
		 * @brief 遞迴走訪場景節點，將每個節點的 aiMesh 轉換為 CG::Mesh
		 *
		 * Assimp 場景樹（aiNode）只儲存索引，實際網格資料在 aiScene::mMeshes 中。
		 * 此函式取得索引 → 查詢 scene → 呼叫 processMesh() 轉換。
		 *
		 * @param node  當前節點
		 * @param scene Assimp 場景根物件（含所有網格、材質、貼圖資料）
		 */
        void processNode(aiNode* node, const aiScene* scene)
        {
			// 處理此節點上的所有網格
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                meshes.push_back(processMesh(mesh, scene));
            }

			// 遞迴處理子節點
            for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
                processNode(node->mChildren[i], scene);
            }
		}

		/**
		 * @brief 將 Assimp 的 aiMesh 轉換為 CG::Mesh
		 *
		 * 轉換步驟：
		 *  1. 走訪所有頂點，複製位置、法線、UV、切線等屬性
		 *  2. 走訪所有面（三角形），展開成索引陣列
		 *  3. 從 aiMaterial 讀取材質顏色（ambient / diffuse / specular / shininess / opacity）
		 *  4. 載入各類型貼圖（diffuse / specular / normal / height / emissive）
		 *
		 * @param mesh  Assimp 網格物件
		 * @param scene Assimp 場景（用於查詢材質）
		 * @return      轉換完成的 CG::Mesh（已上傳至 GPU）
		 */
        Mesh processMesh(aiMesh* mesh, const aiScene* scene)
        {
            vector<Vertex>       vertices;
            vector<unsigned int> indices;
            vector<MeshTexture>  textures;
            Material             material;

			// ── Step 1：轉換頂點資料 ──────────────────────────────────
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex vertex;
				glm::vec3 vector;  // Assimp 向量型別不能直接轉 glm，需中介變數

				// 頂點座標
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.Position = vector;

				// 法線（HasNormals() 確認模型包含法線資料）
                if (mesh->HasNormals())
                {
                    vector.x = mesh->mNormals[i].x;
                    vector.y = mesh->mNormals[i].y;
                    vector.z = mesh->mNormals[i].z;
                    vertex.Normal = vector;
                }

				// UV 座標、切線、副切線（mTextureCoords[0] 為第一組 UV，最常用）
                if (mesh->mTextureCoords[0])
                {
                    glm::vec2 vec;
                    vec.x = mesh->mTextureCoords[0][i].x;
                    vec.y = mesh->mTextureCoords[0][i].y;
                    vertex.TexCoords = vec;

					// Tangent（由 aiProcess_CalcTangentSpace 自動計算）
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;

					// Bitangent
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
                else
                {
					vertex.TexCoords = glm::vec2(0.0f, 0.0f);  // 無 UV 時設為原點
                }

                vertices.push_back(vertex);
            }

			// ── Step 2：展開三角形索引 ───────────────────────────────
			// aiProcess_Triangulate 已確保每個 face 都是三角形（mNumIndices = 3）
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }

			// ── Step 3：讀取材質顏色屬性（來自 .mtl 或嵌入材質）────
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiColor3D color;

            if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
                material.ambient = glm::vec3(color.r, color.g, color.b);

            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
                material.diffuse = glm::vec3(color.r, color.g, color.b);

            if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
                material.specular = glm::vec3(color.r, color.g, color.b);

            aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
            if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
                material.emissive = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);

            // Assimp 5.2+ 支援讀取 Blender 的 Emission Strength
            float emissiveStrength = 1.0f;
            if (mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength) == AI_SUCCESS)
                material.emissiveStrength = emissiveStrength;

            float shininess = 0.0f;
            if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
                material.shininess = shininess;

            float opacity = 1.0f;
            if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
                material.opacity = opacity;

            // ── Step 4：載入各類型貼圖 ──
            // [修正] 傳入 scene，讓 loadMaterialTextures 能判斷是否為內嵌貼圖
            auto diffuseMaps = loadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            auto specularMaps = loadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular", scene);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            auto normalMaps = loadMaterialTextures(mat, aiTextureType_NORMALS, "texture_normal", scene);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            auto heightMaps = loadMaterialTextures(mat, aiTextureType_DISPLACEMENT, "texture_height", scene);
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

            auto emissiveColorMaps = loadMaterialTextures(mat, aiTextureType_EMISSION_COLOR, "texture_emissiveColor", scene);
            textures.insert(textures.end(), emissiveColorMaps.begin(), emissiveColorMaps.end());

            return Mesh(vertices, indices, textures, material);
        }

        // [修正] 新增 scene 參數，用於判斷貼圖是否內嵌於 FBX
        vector<MeshTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
        {
            vector<MeshTexture> textures;

            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);

                // 快取命中檢查（路徑相同視為同一張貼圖）
                bool skip = false;
                for (unsigned int j = 0; j < textures_loaded.size(); j++)
                {
                    if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0 && textures_loaded[j].type == typeName)
                    {
                        textures.push_back(textures_loaded[j]);
                        skip = true;
                        break;
                    }
                }

                if (!skip)
                {
                    MeshTexture texture;
                    const aiTexture* embeddedTex = scene->GetEmbeddedTexture(str.C_Str());
                    if (embeddedTex)
                    {
                        std::cout << "載入內嵌貼圖：" << str.C_Str() << std::endl;
                        texture.id = TextureFromMemory(embeddedTex);
                    }
                    else
                    {
                        // 外部檔案：TextureFromFile 內部會先嘗試完整路徑，
                        // 失敗後再以「僅檔名」在 directory 下重試
                        std::cout << "載入外部貼圖：" << str.C_Str() << std::endl;
                        texture.id = TextureFromFile(str.C_Str(), this->directory);
                    }

                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            }
            return textures;
        }
    };

    // =========================================================================
    // TextureFromFile 實作
    // =========================================================================

	/**
	 * @brief 從檔案路徑載入一張 2D 貼圖並上傳至 GPU
	 *
	 * 使用 stb_image 讀取圖片，自動根據通道數設定 OpenGL 格式：
	 *  - 1 ch → GL_RED
	 *  - 3 ch → GL_RGB
	 *  - 4 ch → GL_RGBA
	 *
	 * 設定參數：
	 *  - Wrap S/T     : GL_REPEAT（貼圖重複鋪滿）
	 *  - Min Filter   : GL_LINEAR_MIPMAP_LINEAR（三線性過濾，遠距離使用 Mipmap）
	 *  - Mag Filter   : GL_LINEAR（放大時線性插值）
	 *
	 * @note inline 宣告是為了讓 .h 直接包含實作，避免多個 .cpp 連結時產生重複定義錯誤。
	 *
	 * @param path      相對於 directory 的貼圖檔名
	 * @param directory 模型所在資料夾路徑
	 * @param gamma     （預留）Gamma 校正旗標，目前未實作
	 * @return OpenGL 紋理物件 ID
	 */
    inline unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
    {
        string rawPath = string(path);

        // 1st try：直接將 Assimp 回傳路徑與 directory 拼接
        string filename = directory + '/' + rawPath;

        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

        // [修正] 2nd try：若第一次失敗，擷取純檔名後在 directory 下重試
        // 解決 FBX 回傳 "modelname.fbm/tex.png" 或 "C:\...\tex.png" 的情況
        if (!data)
        {
            size_t slashPos = rawPath.find_last_of("/\\");
            if (slashPos != string::npos)
            {
                string filenameOnly = rawPath.substr(slashPos + 1);
                string fallbackPath = directory + '/' + filenameOnly;

                std::cout << "找不到貼圖 [" << filename << "]，改以純檔名重試：" << fallbackPath << std::endl;

                data = stbi_load(fallbackPath.c_str(), &width, &height, &nrComponents, 0);
                if (data) filename = fallbackPath;
            }
        }

        unsigned int textureID;
        glGenTextures(1, &textureID);

        if (data)
        {
            GLenum format = GL_RGBA;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 3) format = GL_RGB;
            else if (nrComponents == 4) format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format,
                width, height, 0,
                format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);  // 自動產生 Mipmap 層級

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);  // 上傳完成後釋放 CPU 端記憶體
        }
        else
        {
			std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }
}