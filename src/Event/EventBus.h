#pragma once
#include <any>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Scene/Transform.h"  // SceneObject

namespace CG
{
    // 輕量型別安全的發布-訂閱事件匯流排
    // Subscribe<EventT>(handler) — 以事件型別為鍵，註冊回呼函式
    // Publish<EventT>(event)     — 派發事件至所有該型別的訂閱者
    //
    // 使用 std::type_index 作為鍵，std::any 包裝事件實例，達到型別擦除
    class EventBus
    {
    public:
        template<typename EventT>
        void Subscribe(std::function<void(const EventT&)> handler)
        {
            auto key = std::type_index(typeid(EventT));
            m_handlers[key].emplace_back([handler](const std::any& e)
            {
                handler(std::any_cast<const EventT&>(e));
            });
        }

        template<typename EventT>
        void Publish(const EventT& event)
        {
            auto key = std::type_index(typeid(EventT));
            auto it  = m_handlers.find(key);
            if (it == m_handlers.end()) return;  // 無訂閱者，直接返回

            std::any wrapped(event);
            for (auto& h : it->second)
                h(wrapped);
        }

    private:
        // key = 事件型別；value = 所有訂閱此事件的回呼（型別擦除後）
        std::unordered_map<std::type_index,
            std::vector<std::function<void(const std::any&)>>> m_handlers;
    };

    // ─── 事件型別定義 ─────────────────────────────────────────────────────────

    // 「物件被選取」事件
    // 發布者：ViewportWindow（顏色拾取點擊）、HierarchyWindow（樹狀列表點擊）
    // 訂閱者：App（更新 mainScene->selectedObject）、SequencerWindow（自動捲動至對應軌道）
    struct ObjectSelectedEvent
    {
        SceneObject* object;  // 被選取的物件；nullptr 代表取消選取
    };

} // namespace CG
