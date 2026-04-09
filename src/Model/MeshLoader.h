/**
 * @file MeshLoader.h
 * @brief 定義網格（Mesh）相關資料結構與渲染功能
 *
 * 包含：
 *  - Vertex       : 單一頂點的所有屬性（位置、法線、UV、骨骼等）
 *  - MeshTexture  : 貼圖的 GPU handle、類型、路徑
 *  - Material     : Phong 光照模型的材質參數（來自 .mtl）
 *  - Mesh         : 封裝 VAO/VBO/EBO 並支援 Instanced Rendering
 */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Shader/Shader.h"

#include <string>
#include <vector>
using namespace std;

/// 每個頂點最多受幾根骨骼影響（骨骼動畫用）
#define MAX_BONE_INFLUENCE 4

namespace CG
{
	// =========================================================================
	// 資料結構
	// =========================================================================

	/**
	 * @struct Vertex
	 * @brief 描述一個頂點的所有屬性
	 *
	 * 記憶體佈局連續，可直接以 sizeof(Vertex) stride 傳給 OpenGL。
	 */
	struct Vertex {
		glm::vec3 Position;   ///< 頂點在模型空間中的座標
		glm::vec3 Normal;     ///< 頂點法線（用於光照計算）
		glm::vec2 TexCoords;  ///< UV 貼圖座標

		glm::vec3 Tangent;    ///< 切線向量（Normal Mapping 用）
		glm::vec3 Bitangent;  ///< 副切線向量（與 Tangent、Normal 構成 TBN 矩陣）

		/// 影響此頂點的骨骼 ID（最多 MAX_BONE_INFLUENCE 根）
		int m_BoneIDs[MAX_BONE_INFLUENCE];
		/// 對應骨骼的權重（各權重加總應為 1.0）
		float m_Weights[MAX_BONE_INFLUENCE];
	};

	/**
	 * @struct MeshTexture
	 * @brief 代表一張已上傳至 GPU 的貼圖
	 */
	struct MeshTexture {
		unsigned int id;  ///< OpenGL 紋理物件的 ID（由 glGenTextures 產生）
		string type;      ///< 貼圖類型，例如 "texture_diffuse"、"texture_normal"
		string path;      ///< 原始檔案路徑（用於避免重複載入）
	};

	/**
	 * @struct Material
	 * @brief Phong 光照模型的材質參數，對應 .mtl 檔中的屬性
	 */
	struct Material {
		glm::vec3 ambient = glm::vec3(0.1f);  ///< 環境光反射係數（Ka）
		glm::vec3 diffuse = glm::vec3(1.0f);  ///< 漫反射係數（Kd）
		glm::vec3 specular = glm::vec3(0.5f);  ///< 鏡面反射係數（Ks）
		glm::vec3 emissive = glm::vec3(0.0f);  // ← 新增
		float     emissiveStrength = 1.0f;             // ← 新增（對應 Blender 的 Strength
		float     shininess = 32.0f;           ///< 高光指數（Ns），越大光斑越小越亮
		float     opacity = 1.0f;            ///< 不透明度（d / Tr），1.0 = 完全不透明
	};

	// =========================================================================
	// Mesh 類別
	// =========================================================================

	/**
	 * @class Mesh
	 * @brief 封裝單一網格的 GPU 資源，並提供 Instanced Rendering 介面
	 *
	 * 在建構時呼叫 setupMesh() 將頂點、索引資料上傳至 GPU，
	 * 並設定對應的 VAO/VBO/EBO/instanceVBO。
	 *
	 * Vertex Attribute 配置：
	 *  location 0  → Position   (vec3)
	 *  location 1  → Normal     (vec3)
	 *  location 2  → TexCoords  (vec2)
	 *  location 3  → Tangent    (vec3)
	 *  location 4  → Bitangent  (vec3)
	 *  location 5  → BoneIDs    (ivec4)
	 *  location 6  → Weights    (vec4)
	 *  location 7–10 → Instance Model Matrix (mat4，4 × vec4，divisor = 1)
	 */
	class Mesh {
	public:
		// ----- 公開資料成員 -----
		vector<Vertex>       vertices;  ///< CPU 端頂點陣列（建構後仍保留，方便 CPU 端查詢）
		vector<unsigned int> indices;   ///< 三角形索引陣列
		vector<MeshTexture>  textures;  ///< 貼圖列表
		Material             material;  ///< 材質屬性
		unsigned int VAO;               ///< Vertex Array Object（綁定所有 attrib 設定）

		/**
		 * @brief 建構子：接收網格資料並立即上傳至 GPU
		 * @param vertices  頂點資料
		 * @param indices   索引資料
		 * @param textures  貼圖列表
		 * @param material  材質參數
		 */
		Mesh(vector<Vertex> vertices, vector<unsigned int> indices,
			vector<MeshTexture> textures, Material material)
		{
			this->vertices = vertices;
			this->indices = indices;
			this->textures = textures;
			this->material = material;
			setupMesh();  // 將資料上傳至 GPU，建立 VAO/VBO/EBO/instanceVBO
		}

		/**
		 * @brief Instanced Rendering：一次 Draw Call 繪製多個相同網格
		 *
		 * 流程：
		 *  1. 將本次的 modelMatrices 上傳至 instanceVBO（GL_DYNAMIC_DRAW）
		 *  2. 設定 Shader uniform（材質 & 貼圖）
		 *  3. 呼叫 glDrawElementsInstanced 一次繪製所有 instance
		 *
		 * @param shader         使用的 Shader 程式
		 * @param modelMatrices  每個 instance 的 Model Matrix 陣列
		 * @param useTextures    true = 綁定模型本身的貼圖；false = 使用 defaultTexID
		 * @param defaultTexID   當 useTextures = false 或貼圖為空時使用的預設貼圖 ID
		 */
		void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& modelMatrices, bool useTextures = true, unsigned int defaultTexID = 0)
		{
			// ── Step 1：動態更新 instanceVBO 的 Model Matrix 資料 ──
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data(), GL_DYNAMIC_DRAW);  // DYNAMIC 因為每幀都可能改變

			// ── Step 2：設定材質 uniform ──
			shader.setUnifInt("useTextures", useTextures);
			shader.setUnifVec3("material.ambient", &material.ambient[0]);
			shader.setUnifVec3("material.diffuse", &material.diffuse[0]);
			shader.setUnifVec3("material.specular", &material.specular[0]);
			shader.setUnifVec3("material.emissive", &material.emissive[0]);
			shader.setUnifFloat("material.emissiveStrength", material.emissiveStrength);
			shader.setUnifFloat("material.shininess", material.shininess);

			// ── Step 3：綁定貼圖 ──
			if (!useTextures || textures.empty())
			{
				// 無貼圖模式：綁定一張純白 1x1 貼圖（由 Model 產生），顏色完全由材質決定
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, defaultTexID);
				shader.setUnifInt("texture_diffuse1", 0);
			}
			else
			{
				// 有貼圖模式：依類型計數命名（texture_diffuse1、texture_specular1 …）
				// 並逐一綁定到對應的 Texture Unit
				unsigned int diffuseNr = 1, specularNr = 1,
					normalNr = 1, heightNr = 1, emissiveNr = 1;

				for (unsigned int i = 0; i < textures.size(); i++)
				{
					glActiveTexture(GL_TEXTURE0 + i);  // 啟用第 i 個 Texture Unit

					std::string number, name = textures[i].type;
					// 根據貼圖類型累加計數，形成 "texture_diffuse1"、"texture_specular1" 等名稱
					if (name == "texture_diffuse")  number = std::to_string(diffuseNr++);
					else if (name == "texture_specular") number = std::to_string(specularNr++);
					else if (name == "texture_normal")   number = std::to_string(normalNr++);
					else if (name == "texture_height")   number = std::to_string(heightNr++);
					else if (name == "texture_emissive")   number = std::to_string(emissiveNr++);

					// 將 Texture Unit 編號傳入 Shader，再綁定貼圖
					glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
					glBindTexture(GL_TEXTURE_2D, textures[i].id);
				}
			}

			// ── Step 4：繪製 ──
			glBindVertexArray(VAO);
			glDrawElementsInstanced(
				GL_TRIANGLES,
				static_cast<unsigned int>(indices.size()),  // 索引總數
				GL_UNSIGNED_INT,
				0,
				static_cast<GLsizei>(modelMatrices.size())  // instance 數量
			);
			glBindVertexArray(0);
			glActiveTexture(GL_TEXTURE0);  // 還原 active texture unit，避免影響後續繪製
		}

	private:
		// ----- 私有 GPU 資源 -----
		unsigned int VBO;         ///< Vertex Buffer Object（頂點資料）
		unsigned int EBO;         ///< Element Buffer Object（索引資料）
		unsigned int instanceVBO; ///< 存放每個 instance 的 Model Matrix（每幀更新）

		/**
		 * @brief 建立並初始化所有 GPU buffer 及頂點屬性指標
		 *
		 * 呼叫時機：僅在 Mesh 建構子中呼叫一次。
		 *
		 * VAO 綁定的 Attribute 配置詳見類別文件。
		 * instanceVBO 在此只建立空 buffer（大小 0），
		 * 實際資料在 DrawInstanced() 呼叫時才以 GL_DYNAMIC_DRAW 填入。
		 */
		void setupMesh()
		{
			// 建立 GPU 物件
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			glGenBuffers(1, &instanceVBO);

			glBindVertexArray(VAO);

			// ── 上傳頂點資料 ──
			// Vertex struct 記憶體佈局連續，可直接傳指標給 OpenGL
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER,
				vertices.size() * sizeof(Vertex),
				&vertices[0],
				GL_STATIC_DRAW);  // 頂點資料靜態，不會改變

			// ── 上傳索引資料 ──
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				indices.size() * sizeof(unsigned int),
				&indices[0],
				GL_STATIC_DRAW);

			// ── 設定每個頂點屬性的偏移與格式 ──

			// location 0：Position（vec3，offset = 0）
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

			// location 1：Normal（vec3）
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
				(void*)offsetof(Vertex, Normal));

			// location 2：TexCoords（vec2）
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
				(void*)offsetof(Vertex, TexCoords));

			// location 3：Tangent（vec3，Normal Mapping 用）
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
				(void*)offsetof(Vertex, Tangent));

			// location 4：Bitangent（vec3，Normal Mapping 用）
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
				(void*)offsetof(Vertex, Bitangent));

			// location 5：BoneIDs（ivec4，使用整數版本 glVertexAttribIPointer）
			glEnableVertexAttribArray(5);
			glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex),
				(void*)offsetof(Vertex, m_BoneIDs));

			// location 6：Weights（vec4，骨骼權重）
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
				(void*)offsetof(Vertex, m_Weights));

			// ── 設定 Instance Model Matrix（location 7–10）──
			// 一個 mat4 = 4 × vec4，需佔用 4 個連續 attribute location
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);  // 先建立空 buffer

			for (unsigned int i = 0; i < 4; i++)
			{
				glEnableVertexAttribArray(7 + i);
				glVertexAttribPointer(7 + i, 4, GL_FLOAT, GL_FALSE,
					sizeof(glm::mat4),
					(void*)(i * sizeof(glm::vec4)));  // 每列偏移一個 vec4
				glVertexAttribDivisor(7 + i, 1);  // divisor = 1：每繪製一個 instance 才前進一次
			}

			glBindVertexArray(0);  // 解除 VAO 綁定，避免後續操作意外修改
		}
	};
}