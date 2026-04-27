#pragma once
#include <memory>
#include <stack>
#include <string>
#include <glm/glm.hpp>

#include "Scene/Transform.h"
#include "IK/IKSolver.h"

namespace CG
{
    // ─── ICommand：命令介面 ───────────────────────────────────────────────────
    // 所有可撤銷操作均繼承此介面並實作 Execute() 與 Undo()
    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
        virtual void Undo()    = 0;
        virtual std::string GetDescription() const { return ""; }
    };

    // ─── CommandStack：Undo / Redo 堆疊 ──────────────────────────────────────
    // 執行命令時推入 undoStack；Undo 時移至 redoStack；有新命令時清空 redoStack
    class CommandStack
    {
    public:
        // 執行命令並壓入 undoStack，同時清空 redoStack（新操作使重做記錄失效）
        void Execute(std::unique_ptr<ICommand> cmd)
        {
            cmd->Execute();
            m_undoStack.push(std::move(cmd));
            while (!m_redoStack.empty()) m_redoStack.pop();
        }

        // 復原：將最上層命令從 undoStack 移至 redoStack 並呼叫 Undo()
        void Undo()
        {
            if (m_undoStack.empty()) return;
            m_undoStack.top()->Undo();
            m_redoStack.push(std::move(m_undoStack.top()));
            m_undoStack.pop();
        }

        // 重做：將最上層命令從 redoStack 移回 undoStack 並再次 Execute()
        void Redo()
        {
            if (m_redoStack.empty()) return;
            m_redoStack.top()->Execute();
            m_undoStack.push(std::move(m_redoStack.top()));
            m_redoStack.pop();
        }

        bool CanUndo() const { return !m_undoStack.empty(); }
        bool CanRedo() const { return !m_redoStack.empty(); }

    private:
        std::stack<std::unique_ptr<ICommand>> m_undoStack;
        std::stack<std::unique_ptr<ICommand>> m_redoStack;
    };

    // ─── TransformCommand：物件 Transform 變更命令 ───────────────────────────
    // 在 Gizmo 拖曳「開始」時記錄 m_before，「結束」時記錄 m_after，再呼叫 Execute()
    class TransformCommand : public ICommand
    {
    public:
        TransformCommand(SceneObject* obj, const Transform& before, const Transform& after)
            : m_object(obj), m_before(before), m_after(after) {}

        void Execute() override
        {
            m_object->SetPosition(m_after.position);
            m_object->SetRotation(m_after.rotation);
            m_object->SetScale(m_after.scale);
        }

        void Undo() override
        {
            m_object->SetPosition(m_before.position);
            m_object->SetRotation(m_before.rotation);
            m_object->SetScale(m_before.scale);
        }

        std::string GetDescription() const override
        {
            return "Transform: " + m_object->objectName;
        }

    private:
        SceneObject* m_object;
        Transform    m_before;  // 拖曳前的 Transform 快照
        Transform    m_after;   // 拖曳後的 Transform 快照
    };

    // ─── IKTargetMoveCommand：IK 目標點移動命令 ──────────────────────────────
    // 記錄拖曳前後 IK Chain 目標點的世界座標，支援 Undo / Redo
    class IKTargetMoveCommand : public ICommand
    {
    public:
        IKTargetMoveCommand(IKChain* chain, const glm::vec3& before, const glm::vec3& after)
            : m_chain(chain), m_before(before), m_after(after) {}

        void Execute() override { m_chain->target = m_after;  }
        void Undo()    override { m_chain->target = m_before; }

        std::string GetDescription() const override
        {
            return "IK Target: " + m_chain->name;
        }

    private:
        IKChain*  m_chain;
        glm::vec3 m_before;  // 移動前的目標世界座標
        glm::vec3 m_after;   // 移動後的目標世界座標
    };

} // namespace CG
