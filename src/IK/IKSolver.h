#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Scene/Transform.h"

// ──────────────────────────────────────────────────────────────────────────────
//  FABRIK Inverse Kinematics
//
//  使用方式：
//    1. 建立 IKChain，填入有序的 SceneObject* 關節串列（根 → 末端效應器）
//    2. 呼叫 chain.ComputeBoneLengths()  (初始化一次即可)
//    3. 每幀將 chain.target 設為世界座標目標點
//    4. chain.enabled = true
//    5. 呼叫 IKSolver::Solve(chain)
// ──────────────────────────────────────────────────────────────────────────────

namespace CG {

// ─── IKChain ──────────────────────────────────────────────────────────────────
struct IKChain
{
    std::string  name;
    // joints[0] = 根關節（root），joints.back() = 末端效應器（end effector）
    std::vector<SceneObject*> joints;

    glm::vec3  target       = glm::vec3(0.0f); // 世界座標目標
    bool       enabled      = false;
    int        maxIter      = 20;              // FABRIK 最大迭代次數
    float      tolerance    = 0.05f;           // 收斂距離（world units）

    // 預先計算的骨骼長度（初始化後不應改變）
    std::vector<float> boneLengths;
    float              totalLength = 0.0f;

    // 根據當前世界矩陣計算骨骼長度，初始化時呼叫一次
    void ComputeBoneLengths()
    {
        boneLengths.clear();
        totalLength = 0.0f;
        for (int i = 0; i + 1 < (int)joints.size(); i++)
        {
            glm::vec3 p0 = glm::vec3(joints[i    ]->GetWorldMatrix()[3]);
            glm::vec3 p1 = glm::vec3(joints[i + 1]->GetWorldMatrix()[3]);
            float len = glm::length(p1 - p0);
            boneLengths.push_back(len);
            totalLength += len;
        }
    }
};

// ─── IKSolver ─────────────────────────────────────────────────────────────────
class IKSolver
{
public:
    // 解算一條 IKChain：
    //   · 從 joints[0].GetWorldMatrix() 取得當前世界座標
    //   · 執行 FABRIK 迭代
    //   · 將結果寫回每個關節的 local rotation
    static void Solve(IKChain& chain)
    {
        if (!chain.enabled) return;
        const int n = (int)chain.joints.size();
        if (n < 2) return;

        // 延遲初始化骨骼長度
        if ((int)chain.boneLengths.size() != n - 1)
            chain.ComputeBoneLengths();

        // ── 收集當前世界座標 ──────────────────────────────────────────────────
        std::vector<glm::vec3> pos(n);
        for (int i = 0; i < n; i++)
            pos[i] = glm::vec3(chain.joints[i]->GetWorldMatrix()[3]);

        const glm::vec3 rootPos = pos[0];

        // ── 目標超出最大伸展長度 → 沿直線全力伸展 ───────────────────────────
        float distToTarget = glm::length(chain.target - rootPos);
        if (distToTarget >= chain.totalLength)
        {
            glm::vec3 dir = glm::normalize(chain.target - rootPos);
            for (int i = 1; i < n; i++)
                pos[i] = pos[i - 1] + dir * chain.boneLengths[i - 1];
        }
        else
        {
            // ── FABRIK 迭代 ───────────────────────────────────────────────────
            for (int iter = 0; iter < chain.maxIter; iter++)
            {
                if (glm::length(pos[n - 1] - chain.target) < chain.tolerance)
                    break;

                // Forward pass：末端貼近目標，向根部收縮
                pos[n - 1] = chain.target;
                for (int i = n - 2; i >= 0; i--)
                {
                    glm::vec3 d = glm::normalize(pos[i] - pos[i + 1]);
                    pos[i] = pos[i + 1] + d * chain.boneLengths[i];
                }

                // Backward pass：根部固定，向末端伸展
                pos[0] = rootPos;
                for (int i = 0; i < n - 1; i++)
                {
                    glm::vec3 d = glm::normalize(pos[i + 1] - pos[i]);
                    pos[i + 1] = pos[i] + d * chain.boneLengths[i];
                }
            }
        }

        // ── 將 FABRIK 世界座標寫回 local rotation ────────────────────────────
        WriteBackRotations(chain, pos);
    }

private:
    // 從 mat4 提取旋轉四元數（剔除縮放影響）
    static glm::quat ExtractRotation(const glm::mat4& m)
    {
        glm::vec3 s;
        s.x = glm::length(glm::vec3(m[0]));
        s.y = glm::length(glm::vec3(m[1]));
        s.z = glm::length(glm::vec3(m[2]));
        if (s.x < 1e-6f || s.y < 1e-6f || s.z < 1e-6f)
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        glm::mat3 r(
            glm::vec3(m[0]) / s.x,
            glm::vec3(m[1]) / s.y,
            glm::vec3(m[2]) / s.z
        );
        return glm::normalize(glm::quat_cast(r));
    }

    // 根據 FABRIK 解算後的世界座標 newPos，逐關節更新 local rotation
    //
    // 核心邏輯（對每個 joint[i]，i = 0 .. n-2）：
    //   1. desiredDir = normalize(newPos[i+1] - newPos[i])
    //   2. 取 joint[i] 的當前世界旋轉 worldRot
    //   3. localBoneDir = normalize(child.transform.position)  ← 子節點在父節點 local 空間的方向
    //   4. currentBoneWorld = worldRot * localBoneDir
    //   5. delta = rotation(currentBoneWorld → desiredDir)     ← 世界空間修正旋轉
    //   6. newWorldRot = delta * worldRot
    //   7. newLocalRot = inv(parentWorldRot) * newWorldRot
    //
    // 處理完 joint[i] 後，joint[i+1] 的實際世界座標 = newPos[i+1]，
    // 因此下一個迭代的 GetWorldMatrix() 會正確反映已修正的父節點。
    static void WriteBackRotations(IKChain& chain, const std::vector<glm::vec3>& newPos)
    {
        const int n = (int)chain.joints.size();
        for (int i = 0; i < n - 1; i++)
        {
            SceneObject* joint = chain.joints[i];
            SceneObject* child = chain.joints[i + 1];

            // 1. 目標骨骼方向（世界空間）
            glm::vec3 toNext = newPos[i + 1] - newPos[i];
            if (glm::length(toNext) < 1e-6f) continue;
            glm::vec3 desiredDir = glm::normalize(toNext);

            // 2. 當前世界旋轉
            glm::quat worldRot = ExtractRotation(joint->GetWorldMatrix());

            // 3. 骨骼參考方向（子節點在本節點 local 空間的位置方向）
            glm::vec3 localBoneVec = child->transform.position;
            if (glm::length(localBoneVec) < 1e-6f) continue;
            glm::vec3 localBoneDir = glm::normalize(localBoneVec);

            // 4. 骨骼當前世界方向
            glm::vec3 currentBoneWorld = worldRot * localBoneDir;

            // 5 & 6. 修正旋轉並套用
            glm::quat delta      = glm::rotation(currentBoneWorld, desiredDir);
            glm::quat newWorldRot = glm::normalize(delta * worldRot);

            // 7. 轉回 local 旋轉
            glm::quat parentWorldRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            if (joint->parent)
                parentWorldRot = ExtractRotation(joint->parent->GetWorldMatrix());

            glm::quat newLocalRot = glm::normalize(glm::inverse(parentWorldRot) * newWorldRot);
            joint->SetRotation(newLocalRot);
        }
    }
};

} // namespace CG
