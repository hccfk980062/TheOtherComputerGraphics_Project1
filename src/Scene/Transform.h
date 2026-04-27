#pragma once

#include <glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CG
{
    // 封裝位移 / 旋轉 / 縮放，並計算本地 TRS 矩陣
    struct Transform
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat();      // 以四元數儲存旋轉，避免萬向節鎖
        glm::vec3 scale    = glm::vec3(1.0f);

        // 計算本地矩陣（Local → Parent），TRS 順序：先縮放、再旋轉、再位移
        glm::mat4 GetLocalMatrix() const
        {
            glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 r = glm::mat4_cast(rotation);
            glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
            return t * r * s;
        }
    };

    // 場景中的一個節點，持有模型引用、Transform 及父子關係
    struct SceneObject
    {
    private:
        mutable glm::mat4 cachedWorldMatrix = glm::mat4(1.0f);
        mutable bool isDirty = true;  // 標記世界矩陣快取是否已失效，需重新計算

    public:
        uint32_t    id;                       // 唯一識別碼，用於顏色拾取編碼
        std::string objectName;               // 顯示名稱（格式：groupName_serializedName）
        std::string animationGroupName;       // 所屬動畫群組名稱（例如 "Gundam_0"）
        std::string animationSerializedName;  // 此節點在 JSON 序列化時使用的名稱
        Transform   transform;                // Local transform（相對於父節點）
        Model*      model      = nullptr;     // 指向渲染模型；nullptr 代表空節點
        int         objectType = 0;           // 物件類型：0 = 空節點，1 = 一般模型物件

        SceneObject* parent = nullptr;  // 父節點指標；nullptr 代表根節點
        std::vector<std::unique_ptr<SceneObject>> children;

        // 取得世界矩陣（含快取，dirty 時才重新計算）
        // 採用懶惰求值：只在 transform 改變（MarkDirty）後才重算
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

        // 標記此節點及所有子節點的世界矩陣快取失效（transform 改變時呼叫）
        void MarkDirty()
        {
            isDirty = true;
            for (auto& child : children)
                child->MarkDirty();
        }

        // ── Transform Setter（自動觸發 MarkDirty）───────────────────────────
        void SetPosition(const glm::vec3& pos)
        {
            transform.position = pos;
            this->MarkDirty();
        }

        // 向量版：將尤拉角轉為四元數後儲存
        void SetRotation(const glm::vec3& rot)
        {
            transform.rotation = glm::quat(rot);
            this->MarkDirty();
        }

        // 四元數版：直接設定旋轉四元數
        void SetRotation(const glm::quat& rot)
        {
            transform.rotation = rot;
            this->MarkDirty();
        }

        void SetScale(const glm::vec3& s) { transform.scale = s; MarkDirty(); }

        // 遞迴收集此節點及所有後代節點（深度優先），回傳扁平列表
        std::vector<SceneObject*> GetChildrenObjects()
        {
            std::vector<SceneObject*> objects;
            objects.push_back(this);
            for (auto& child : children)
            {
                std::vector<SceneObject*> childObjs = child->GetChildrenObjects();
                objects.insert(objects.end(), childObjs.begin(), childObjs.end());
            }
            return objects;
        }
    };
}
