#pragma once

#include <imgui.h>
#include <imgui_internal.h>

#include <ImNeoSequencer/imgui_neo_sequencer.h>
#include <ImNeoSequencer/imgui_neo_internal.h>

#include <ImGuiFileDialog/ImGuiFileDialog.h>

#include <glm/gtc/type_ptr.hpp>
#include <json.hpp>
using json = nlohmann::json;

#include "Scene/MainScene.h"
#include "Command/CommandStack.h"

namespace CG
{
    struct AnimationTrack;

    // ─── 資料結構 ─────────────────────────────────────────────────────────────

    // 一個關鍵幀的完整 Transform 狀態
    struct KeyframeData
    {
        int         frame;
        glm::vec3   position;
        glm::quat   rotation;
        glm::vec3   scale;

        AnimationTrack* sourceTrack;  // 所屬軌道（用於 Undo/Redo 及刪除操作）
    };

    // 一個 SceneObject 對應的動畫軌道，持有所有關鍵幀
    struct AnimationTrack
    {
        std::string               trackName;    // 軌道名稱（== animationSerializedName）
        std::vector<KeyframeData> keyframes;    // 所有關鍵幀（依幀號排序）
        SceneObject*              linkedObject = nullptr;  // 對應的場景物件
        bool                      open         = true;     // 在序列器中是否展開

        AnimationTrack() = default;

        AnimationTrack(SceneObject* obj)
            : trackName(obj->animationSerializedName), linkedObject(obj) {}

        // 依幀號升序排列關鍵幀（新增或移動關鍵幀後需呼叫）
        void SortKeyframeDatas()
        {
            std::sort(keyframes.begin(), keyframes.end(),
                [](const KeyframeData& a, const KeyframeData& b) { return a.frame < b.frame; });
        }

        // 取得 frame 位置的「前一個」與「後一個」關鍵幀（夾住邊界）
        // 用於插值計算：超出範圍時 prev == next（維持邊界值）
        void GetClampedKeyframes(int frame, KeyframeData& prev, KeyframeData& next)
        {
            if (keyframes.empty()) return;

            auto nextIt = std::lower_bound(keyframes.begin(), keyframes.end(), frame,
                [](const KeyframeData& kf, int f) { return kf.frame < f; });

            if (nextIt == keyframes.end())
                nextIt = std::prev(keyframes.end());

            auto prevIt = (nextIt == keyframes.begin()) ? keyframes.begin() : std::prev(nextIt);

            prev = *prevIt;
            next = *nextIt;
        }
    };

    // 一組動畫群組：對應一個 Gundam 的所有部位軌道，並持有獨立播放狀態
    struct AnimationGroup
    {
        std::string                 groupName;  // 群組名稱（== animationGroupName，例如 "Gundam_0"）
        std::vector<AnimationTrack> tracks;

        // ── 獨立播放狀態（每個機器人各自管理）──────────────────────────────
        bool open          = true;
        int  currentFrame  = 0;
        int  startFrame    = 0;
        int  endFrame      = 240;   // 可調整此群組的最大動畫長度
        bool isPlaying     = false;
        bool enableLoop    = true;  // 播放結束後是否重播
        int  loopStartFrame = 0;   // 重播起始幀（預設 0）

        double lastTime        = 0.0;
        double timeAccumulated = 0.0;

        AnimationGroup() = default;

        // 建構子：遞迴收集 rootObject 及所有後代，各建立一條軌道
        AnimationGroup(SceneObject* rootObject)
            : groupName(rootObject->animationGroupName)
        {
            for (auto* sub : rootObject->GetChildrenObjects())
                tracks.push_back(AnimationTrack(sub));
        }

        // 根據 currentFrame 插值並套用至所有 linkedObject
        void TransformToFrame()
        {
            for (auto& track : tracks)
            {
                if (track.keyframes.empty()) continue;

                KeyframeData prev, next;
                track.GetClampedKeyframes(currentFrame, prev, next);

                float t     = 0.0f;
                int   range = next.frame - prev.frame;
                if (range > 0)
                    t = glm::clamp(float(currentFrame - prev.frame) / float(range), 0.0f, 1.0f);

                track.linkedObject->SetPosition(glm::mix(prev.position, next.position, t));
                track.linkedObject->SetRotation(glm::slerp(prev.rotation, next.rotation, t));
                track.linkedObject->SetScale(glm::mix(prev.scale, next.scale, t));
            }
        }

        // 推進播放時鐘：每累積 ≥ 15ms（≈ 66fps）推進一幀
        void AdvancePlayback()
        {
            double now = ImGui::GetTime();
            if (!isPlaying)
            {
                lastTime = now;  // 暫停時持續更新，避免恢復播放時時間突增
                return;
            }
            timeAccumulated += now - lastTime;
            lastTime = now;

            if (timeAccumulated >= 0.015)
            {
                timeAccumulated = 0;
                ++currentFrame;
                if (currentFrame > endFrame)
                {
                    if (enableLoop)
                        currentFrame = glm::clamp(loopStartFrame, startFrame, endFrame);
                    else
                    {
                        currentFrame = endFrame;
                        isPlaying    = false;
                    }
                }
                TransformToFrame();
            }
        }
    };

    // ─── JSON 序列化（使用 nlohmann::json）───────────────────────────────────

    inline void to_json(json& j, const KeyframeData& kf)
    {
        j = json{
            {"frame",    kf.frame},
            {"position", {kf.position.x, kf.position.y, kf.position.z}},
            {"rotation", {kf.rotation.w, kf.rotation.x, kf.rotation.y, kf.rotation.z}},
            {"scale",    {kf.scale.x, kf.scale.y, kf.scale.z}}
        };
    }
    inline void from_json(const json& j, KeyframeData& kf)
    {
        j.at("frame").get_to(kf.frame);
        auto pos = j.at("position"); kf.position = { pos[0], pos[1], pos[2] };
        auto rot = j.at("rotation"); kf.rotation = { rot[0], rot[1], rot[2], rot[3] };
        auto scl = j.at("scale");   kf.scale    = { scl[0], scl[1], scl[2] };
    }

    inline void to_json(json& j, const AnimationTrack& t)
    {
        j = json{ {"trackName", t.trackName}, {"keyframes", t.keyframes} };
    }
    inline void from_json(const json& j, AnimationTrack& t)
    {
        j.at("trackName").get_to(t.trackName);
        j.at("keyframes").get_to(t.keyframes);
    }

    // 序列化包含獨立播放設定，向下相容（舊 JSON 缺少欄位時使用預設值）
    inline void to_json(json& j, const AnimationGroup& g)
    {
        j = json{
            {"groupName",      g.groupName},
            {"tracks",         g.tracks},
            {"startFrame",     g.startFrame},
            {"endFrame",       g.endFrame},
            {"enableLoop",     g.enableLoop},
            {"loopStartFrame", g.loopStartFrame}
        };
    }
    inline void from_json(const json& j, AnimationGroup& g)
    {
        j.at("groupName").get_to(g.groupName);
        j.at("tracks").get_to(g.tracks);
        g.startFrame     = j.value("startFrame",     0);
        g.endFrame       = j.value("endFrame",       240);
        g.enableLoop     = j.value("enableLoop",     true);
        g.loopStartFrame = j.value("loopStartFrame", 0);
    }

    // ─── 動畫命令（支援 Undo / Redo）────────────────────────────────────────

    // 新增單一關鍵幀命令
    class AddKeyframeCommand : public ICommand
    {
    public:
        AddKeyframeCommand(AnimationTrack* track, const KeyframeData& kf)
            : m_track(track), m_kf(kf) {}

        void Execute() override
        {
            m_kf.sourceTrack = m_track;
            m_track->keyframes.push_back(m_kf);
            m_track->SortKeyframeDatas();
        }

        // Undo：依幀號找到並移除此關鍵幀
        void Undo() override
        {
            auto& kfs = m_track->keyframes;
            kfs.erase(std::remove_if(kfs.begin(), kfs.end(),
                [this](const KeyframeData& k) { return k.frame == m_kf.frame; }),
                kfs.end());
        }

        std::string GetDescription() const override
        {
            return "Add Keyframe @" + std::to_string(m_kf.frame);
        }

    private:
        AnimationTrack* m_track;
        KeyframeData    m_kf;
    };

    // 批次刪除多個（軌道, 關鍵幀）對的命令，一次 Undo/Redo 完整還原
    class BatchDeleteKeyframesCommand : public ICommand
    {
    public:
        using Pair = std::pair<AnimationTrack*, KeyframeData>;

        explicit BatchDeleteKeyframesCommand(std::vector<Pair> toDelete)
            : m_deleted(std::move(toDelete)) {}

        void Execute() override
        {
            for (auto& [track, kf] : m_deleted)
            {
                auto& kfs = track->keyframes;
                kfs.erase(std::remove_if(kfs.begin(), kfs.end(),
                    [&kf](const KeyframeData& k) { return k.frame == kf.frame; }),
                    kfs.end());
            }
        }

        // Undo：將所有已刪除關鍵幀重新加回並排序
        void Undo() override
        {
            for (auto& [track, kf] : m_deleted)
            {
                track->keyframes.push_back(kf);
                track->SortKeyframeDatas();
            }
        }

        std::string GetDescription() const override
        {
            return "Delete " + std::to_string(m_deleted.size()) + " Keyframe(s)";
        }

    private:
        std::vector<Pair> m_deleted;  // 被刪除的（軌道指標, 關鍵幀資料副本）
    };

    // ─── SequencerWindow ──────────────────────────────────────────────────────

    // 關鍵幀動畫序列器視窗：每個機器人持有獨立 NeoSequencer 軌道組與播放狀態
    class SequencerWindow
    {
    public:
        SequencerWindow();
        ~SequencerWindow();

        auto Initialize() -> bool;
        void Display();  // 每幀呼叫：繪製整個序列器視窗

        // 注入場景後自動為每個頂層物件建立動畫群組
        void SetTargetScene(MainScene* scene)
        {
            targetScene = scene;
            for (auto& obj : targetScene->rootObject.children)
                CreateAnimationGroup(obj.get());
        }

        void SetCommandStack(CommandStack* stack) { m_cmdStack = stack; }

        // 為指定物件（及其所有後代）建立一個 AnimationGroup
        void CreateAnimationGroup(SceneObject* obj)
        {
            animationGroups.push_back(AnimationGroup(obj));
        }

        // 場景重新載入後呼叫：重新將所有軌道的 linkedObject 指向新的 SceneObject
        void RefreshAfterSceneReload();

    private:
        MainScene*    targetScene = nullptr;
        CommandStack* m_cmdStack  = nullptr;

        std::vector<AnimationGroup> animationGroups;

        int             selectedGroupIndex    = -1;
        KeyframeData*   selectedKeyframe      = nullptr;
        AnimationTrack* selectedAnimationTrack = nullptr;

        // 本幀序列器渲染期間收集的「待刪除關鍵幀」，EndNeoSequencer 後批次執行
        std::vector<std::pair<AnimationTrack*, KeyframeData>> m_pendingDeleteKeyframes;

        // 同步場景中的頂層物件與 animationGroups：補建新增的、移除已不存在的
        void SyncAnimationGroups();

        // ── JSON 匯出入 ───────────────────────────────────────────────────────
        void ExportAllToJson(const std::string& filepath);
        void ImportAllFromJson(const std::string& filepath);
        void ExportSpecificGroupToJson(int groupIndex, const std::string& filepath);
        void ImportSpecificGroupFromJson(int groupIndex, const std::string& filepath);
    };

} // namespace CG
