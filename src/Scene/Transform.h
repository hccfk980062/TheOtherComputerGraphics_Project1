#pragma once

#include <glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CG
{
#define PI 3.1415926

	struct Transform
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(); // Quaternion
		glm::vec3 scale = glm::vec3(1.0f);

		// 計算本地矩陣（Local → Parent）
		glm::mat4 GetLocalMatrix() const
		{
			glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 r = glm::mat4_cast(rotation);
			glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
			return t * r * s;  // TRS 順序
		}
	};

	struct SceneObject
	{
    private:
        mutable glm::mat4 cachedWorldMatrix = glm::mat4(1.0f);
        mutable bool isDirty = true;
    public:
        uint32_t id;
        std::string name;
        Transform transform;         // Local transform（給使用者操作）
        Model* model = nullptr;
        int objectType = 0;

        SceneObject* parent = nullptr;
        std::vector<std::unique_ptr<SceneObject>> children;

        // 遞迴向上取得世界矩陣
        glm::mat4 GetWorldMatrix() const
        {
            if (isDirty)
            {
                if (parent != nullptr)
                    cachedWorldMatrix = parent->GetWorldMatrix() * transform.GetLocalMatrix();
                else
                    cachedWorldMatrix = transform.GetLocalMatrix();

                isDirty = false;
            }
            return cachedWorldMatrix;
        }
        void MarkDirty()
        {
            isDirty = true;
            // 父物件改變時，所有子物件也需要更新
            for (auto& child : children)
                child->MarkDirty();
        }

        void SetPosition(const glm::vec3& pos) 
        { 
            transform.position = pos; 
            this->MarkDirty();
        }
        void SetRotation(const glm::vec3& rot) 
        { 
            transform.rotation = glm::quat(rot);
            this->MarkDirty();
        }
        void SetRotation(const glm::quat& rot)
        {
            transform.rotation = rot;
            this->MarkDirty();
        }
        void SetScale(const glm::vec3& s) { transform.scale = s; }
	};
}