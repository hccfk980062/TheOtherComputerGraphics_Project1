#include "SequencerWindow.h"

namespace CG
{
    SequencerWindow::SequencerWindow() : targetScene(nullptr), m_cmdStack(nullptr) {}
    SequencerWindow::~SequencerWindow() {}

    auto SequencerWindow::Initialize() -> bool { return true; }

    void SequencerWindow::SyncAnimationGroups()
    {
        if (!targetScene) return;

        // 移除已不存在的群組：
        //   - 頂層非 Emitter 物件：按 rootObject.children 查找
        //   - Emitter 物件：按 m_emitterObjects 查找，確保 Reparent 後不會誤刪群組
        animationGroups.erase(
            std::remove_if(animationGroups.begin(), animationGroups.end(),
                [&](const AnimationGroup& g)
                {
                    for (auto& childPtr : targetScene->rootObject.children)
                        if (childPtr->animationGroupName == g.groupName) return false;
                    for (auto* emObj : targetScene->m_emitterObjects)
                        if (emObj->animationGroupName == g.groupName) return false;
                    return true;
                }),
            animationGroups.end());

        // 為新增的頂層 SceneObject 補建 AnimationGroup
        for (auto& childPtr : targetScene->rootObject.children)
        {
            const std::string& gName = childPtr->animationGroupName;
            bool found = false;
            for (auto& g : animationGroups)
                if (g.groupName == gName) { found = true; break; }
            if (!found)
                CreateAnimationGroup(childPtr.get());
        }

        // 為所有 Emitter 補建 AnimationGroup（它們可能已被 Reparent 至非根層級，
        // 因此不在 rootObject.children 中，但仍需維持自己的時間軸）
        for (auto* emObj : targetScene->m_emitterObjects)
        {
            const std::string& gName = emObj->animationGroupName;
            bool found = false;
            for (auto& g : animationGroups)
                if (g.groupName == gName) { found = true; break; }
            if (!found)
                CreateAnimationGroup(emObj);
        }
    }

    void SequencerWindow::Display()
    {
        if (!ImGui::Begin("Animation Sequencer", nullptr))
        {
            ImGui::End();
            return;
        }

        if (!targetScene)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Scene Assigned!");
            ImGui::End();
            return;
        }

        // 每幀同步：補建新 Emitter 的軌道、移除已清除 Emitter 的軌道
        SyncAnimationGroups();

        // ── 全域匯出入按鈕 ────────────────────────────────────────────────────
        if (ImGui::Button("Export ALL"))
        {
            IGFD::FileDialogConfig cfg; cfg.path = "."; cfg.fileName = "ExportedAnimation.json";
            ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ExportALL", "Export ALL to:", ".json", cfg);
        }
        ImGui::SameLine();
        if (ImGui::Button("Import ALL"))
        {
            IGFD::FileDialogConfig cfg; cfg.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ImportALL", "Import ALL from:", ".json", cfg);
        }

        // ── 全域播放控制 ─────────────────────────────────────────────────────
        // 判斷目前是否「全部都在播放」，以決定 Play All 按鈕顯示狀態
        bool allPlaying = !animationGroups.empty() &&
            std::all_of(animationGroups.begin(), animationGroups.end(),
                [](const AnimationGroup& g){ return g.isPlaying; });

        // Play All：讓所有群組開始播放（先整理關鍵幀排序）
        ImGui::BeginDisabled(allPlaying);
        if (ImGui::Button("Play All"))
        {
            for (auto& g : animationGroups)
            {
                for (auto& t : g.tracks) t.SortKeyframeDatas();
                g.isPlaying = true;
            }
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Start playback on all animation groups");

        ImGui::SameLine();

        // Pause All：暫停所有群組（保留目前幀位置）
        bool anyPlaying = std::any_of(animationGroups.begin(), animationGroups.end(),
            [](const AnimationGroup& g){ return g.isPlaying; });
        ImGui::BeginDisabled(!anyPlaying);
        if (ImGui::Button("Pause All"))
        {
            for (auto& g : animationGroups)
                g.isPlaying = false;
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Pause all animation groups");

        ImGui::SameLine();

        // Rewind All：全部歸 0（重設至各群組的 startFrame 並更新場景姿勢）
        if (ImGui::Button("|< All"))
        {
            for (auto& g : animationGroups)
            {
                g.isPlaying    = false;
                g.currentFrame = g.startFrame;
                g.TransformToFrame();
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Stop & rewind all groups to their start frame");

        ImGui::Separator();

        // ── 全域檔案對話框 ────────────────────────────────────────────────────
        if (ImGuiFileDialog::Instance()->Display("FileDialog_ExportALL", 32, ImVec2(300, 200)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                for (auto& g : animationGroups) for (auto& t : g.tracks) t.SortKeyframeDatas();
                ExportAllToJson(ImGuiFileDialog::Instance()->GetFilePathName());
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGuiFileDialog::Instance()->Display("FileDialog_ImportALL", 32, ImVec2(300, 200)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
                ImportAllFromJson(ImGuiFileDialog::Instance()->GetFilePathName());
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGuiFileDialog::Instance()->Display("FileDialog_ExportSpecific", 32, ImVec2(300, 200)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                if (selectedGroupIndex >= 0 && selectedGroupIndex < (int)animationGroups.size())
                    for (auto& t : animationGroups[selectedGroupIndex].tracks) t.SortKeyframeDatas();
                ExportSpecificGroupToJson(selectedGroupIndex, ImGuiFileDialog::Instance()->GetFilePathName());
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGuiFileDialog::Instance()->Display("FileDialog_ImportSpecific", 32, ImVec2(300, 200)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
                ImportSpecificGroupFromJson(selectedGroupIndex, ImGuiFileDialog::Instance()->GetFilePathName());
            ImGuiFileDialog::Instance()->Close();
        }

        // ── 各機器人獨立軌道組 ────────────────────────────────────────────────
        int groupIdx = 0;
        for (auto& group : animationGroups)
        {
            ImGui::PushID(groupIdx);

            // 判斷目前選取的軌道是否屬於此群組
            bool trackInGroup = false;
            if (selectedAnimationTrack)
                for (auto& t : group.tracks)
                    if (&t == selectedAnimationTrack) { trackInGroup = true; break; }

            // ── 群組折疊標題 ──────────────────────────────────────────────────
            bool headerOpen = ImGui::CollapsingHeader(group.groupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            // 無論是否展開都推進播放（折疊時仍繼續計時）
            group.AdvancePlayback();

            // 將 Sequencer 當前幀寫入所有 Emitter 節點，使粒子發射時間窗與時間軸同步
            for (auto& track : group.tracks)
                if (track.linkedObject && track.linkedObject->emitter)
                    track.linkedObject->emitter->m_seqFrame = group.currentFrame;

            if (headerOpen)
            {
                // ── 播放控制列 ────────────────────────────────────────────────
                if (ImGui::Button("|<"))
                { group.currentFrame = group.startFrame; group.TransformToFrame(); }
                ImGui::SameLine();
                if (ImGui::Button("<"))
                { group.currentFrame = std::max(group.startFrame, group.currentFrame - 1); group.TransformToFrame(); }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.f);
                if (ImGui::DragInt("##frame", &group.currentFrame, 1, group.startFrame, group.endFrame))
                    group.TransformToFrame();
                ImGui::SameLine();
                if (ImGui::Button(">"))
                { group.currentFrame = std::min(group.endFrame, group.currentFrame + 1); group.TransformToFrame(); }
                ImGui::SameLine();
                if (ImGui::Button(">|"))
                { group.currentFrame = group.endFrame; group.TransformToFrame(); }
                ImGui::SameLine();
                if (ImGui::Button(group.isPlaying ? "Pause" : "Play "))
                {
                    group.isPlaying = !group.isPlaying;
                    if (group.isPlaying)
                        for (auto& t : group.tracks) t.SortKeyframeDatas();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(!trackInGroup);
                if (ImGui::Button("Add KF") && selectedAnimationTrack)
                {
                    KeyframeData prev, next;
                    selectedAnimationTrack->GetClampedKeyframes(group.currentFrame, prev, next);
                    // 同一幀只允許一個關鍵幀（以 next 判斷較精確）
                    if (next.frame != group.currentFrame)
                    {
                        KeyframeData kf{
                            group.currentFrame,
                            selectedAnimationTrack->linkedObject->transform.position,
                            selectedAnimationTrack->linkedObject->transform.rotation,
                            selectedAnimationTrack->linkedObject->transform.scale,
                            selectedAnimationTrack
                        };
                        if (m_cmdStack)
                            m_cmdStack->Execute(std::make_unique<AddKeyframeCommand>(selectedAnimationTrack, kf));
                        else
                        {
                            kf.sourceTrack = selectedAnimationTrack;
                            selectedAnimationTrack->keyframes.push_back(kf);
                            selectedAnimationTrack->SortKeyframeDatas();
                        }
                    }
                }
                ImGui::EndDisabled();

                // ── 動畫長度與循環設定列 ──────────────────────────────────────
                ImGui::Text("Range:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.f);
                ImGui::DragInt("##start", &group.startFrame, 1, 0, group.endFrame - 1);
                ImGui::SameLine();
                ImGui::Text("~");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.f);
                ImGui::DragInt("##end", &group.endFrame, 1, group.startFrame + 1, 9999);
                ImGui::SameLine();
                ImGui::Checkbox("Loop", &group.enableLoop);
                if (group.enableLoop)
                {
                    ImGui::SameLine();
                    ImGui::Text("from");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(60.f);
                    ImGui::DragInt("##loopFrom", &group.loopStartFrame, 1, group.startFrame, group.endFrame);
                }
                ImGui::SameLine();
                if (ImGui::Button("Export"))
                {
                    selectedGroupIndex = groupIdx;
                    IGFD::FileDialogConfig cfg; cfg.path = ".";
                    cfg.fileName = "ExportedAnimation_" + group.groupName + ".json";
                    ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ExportSpecific",
                        ("Export " + group.groupName + " to:").c_str(), ".json", cfg);
                }
                ImGui::SameLine();
                if (ImGui::Button("Import"))
                {
                    selectedGroupIndex = groupIdx;
                    IGFD::FileDialogConfig cfg; cfg.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ImportSpecific",
                        ("Import " + group.groupName + " from:").c_str(), ".json", cfg);
                }

                // ── 此群組的 NeoSequencer ─────────────────────────────────────
                m_pendingDeleteKeyframes.clear();

                if (ImGui::BeginNeoSequencer(group.groupName.c_str(),
                    &group.currentFrame, &group.startFrame, &group.endFrame,
                    { 0, 0 },
                    ImGuiNeoSequencerFlags_EnableSelection          |
                    ImGuiNeoSequencerFlags_Selection_EnableDragging |
                    ImGuiNeoSequencerFlags_Selection_EnableDeletion))
                {
                    for (auto& track : group.tracks)
                    {
                        if (ImGui::BeginNeoTimelineEx(track.trackName.c_str()))
                        {
                            for (auto& kf : track.keyframes)
                            {
                                ImGui::NeoKeyframe(&kf.frame);

                                // 右鍵點擊關鍵幀：開啟編輯 Popup
                                if (ImGui::IsNeoKeyframeRightClicked())
                                {
                                    selectedKeyframe = &kf;
                                    ImGui::OpenPopup("KeyframeEditor");
                                }
                            }

                            // 收集本幀被選取的關鍵幀（供 Delete/Backspace 觸發批次刪除）
                            if (ImGui::NeoHasSelection())
                            {
                                uint32_t selSize = ImGui::GetNeoKeyframeSelectionSize();
                                if (selSize > 0)
                                {
                                    std::vector<ImGui::FrameIndexType> selFrames(selSize);
                                    ImGui::GetNeoKeyframeSelection(selFrames.data());
                                    for (auto selFrame : selFrames)
                                        for (auto& kf : track.keyframes)
                                            if (kf.frame == selFrame)
                                            {
                                                m_pendingDeleteKeyframes.emplace_back(&track, kf);
                                                break;
                                            }
                                }
                            }

                            // 滑鼠釋放且時間軸被選中時，更新 selectedAnimationTrack
                            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                                ImGui::IsNeoTimelineSelected())
                            {
                                selectedAnimationTrack = &track;
                            }

                            ImGui::EndNeoTimeLine();
                        }
                    }

                    // ── 關鍵幀編輯 Popup（右鍵點擊關鍵幀後開啟）──────────────
                    if (ImGui::BeginPopup("KeyframeEditor"))
                    {
                        if (selectedKeyframe)
                        {
                            ImGui::Text("Edit Keyframe  [Frame %d]", selectedKeyframe->frame);
                            ImGui::Separator();

                            ImGui::DragInt("Frame##kf",    &selectedKeyframe->frame,    1, group.startFrame, group.endFrame);
                            ImGui::DragFloat3("Position", glm::value_ptr(selectedKeyframe->position), 0.1f);

                            // 旋轉以尤拉角顯示，輸入後轉回四元數
                            glm::vec3 euler = glm::degrees(glm::eulerAngles(selectedKeyframe->rotation));
                            if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.5f))
                                selectedKeyframe->rotation = glm::quat(glm::radians(euler));

                            ImGui::DragFloat3("Scale", glm::value_ptr(selectedKeyframe->scale), 0.01f);
                            ImGui::Separator();

                            if (ImGui::Button("Apply & Close"))
                            {
                                for (auto& t : group.tracks) t.SortKeyframeDatas();
                                selectedKeyframe = nullptr;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete"))
                            {
                                if (m_cmdStack)
                                {
                                    std::vector<BatchDeleteKeyframesCommand::Pair> del;
                                    del.emplace_back(selectedKeyframe->sourceTrack, *selectedKeyframe);
                                    m_cmdStack->Execute(std::make_unique<BatchDeleteKeyframesCommand>(std::move(del)));
                                }
                                else
                                {
                                    auto& kfs = selectedKeyframe->sourceTrack->keyframes;
                                    kfs.erase(std::remove_if(kfs.begin(), kfs.end(),
                                        [this](const KeyframeData& k) { return &k == selectedKeyframe; }),
                                        kfs.end());
                                }
                                selectedKeyframe = nullptr;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndPopup();
                    }

                    // ── 按下 Delete / Backspace 批次刪除選取的關鍵幀 ─────────
                    bool deletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete,    false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
                    if (deletePressed && !m_pendingDeleteKeyframes.empty() &&
                        ImGui::NeoCanDeleteSelection() && m_cmdStack)
                    {
                        m_cmdStack->Execute(
                            std::make_unique<BatchDeleteKeyframesCommand>(m_pendingDeleteKeyframes));
                        ImGui::NeoClearSelection();
                        m_pendingDeleteKeyframes.clear();
                    }

                    ImGui::EndNeoSequencer();
                }
            }

            ImGui::PopID();
            ++groupIdx;
        }

        ImGui::End();
    }

    // ─── JSON 匯出入實作 ───────────────────────────────────────────────────────

    // 將所有動畫群組（含播放設定）序0列化並寫入 JSON 檔
    void SequencerWindow::ExportAllToJson(const std::string& filepath)
    {
        std::ofstream file(filepath);
        if (!file.is_open()) { std::cerr << "[Sequencer] Cannot open: " << filepath << "\n"; return; }
        file << json(animationGroups).dump(4);
        std::cout << "[Sequencer] Exported ALL to: " << filepath << "\n";
    }

    // 從 JSON 檔讀入所有群組，並重新連結 linkedObject 與 sourceTrack
    void SequencerWindow::ImportAllFromJson(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open()) { std::cerr << "[Sequencer] Not found: " << filepath << "\n"; return; }
        try
        {
            animationGroups = json::parse(file).get<std::vector<AnimationGroup>>();
            for (auto& group : animationGroups)
            {
                for (auto& track : group.tracks)
                {
                    // 還原每個關鍵幀的 sourceTrack 指標（序列化時不含指標）
                    for (auto& kf : track.keyframes) kf.sourceTrack = &track;
                    // 依 trackName 找到對應的 SceneObject 並連結
                    for (auto* obj : targetScene->GetObjectsInAnimationGroup(group.groupName))
                        if (obj->animationSerializedName == track.trackName)
                            track.linkedObject = obj;
                }
            }
            std::cout << "[Sequencer] Imported " << animationGroups.size() << " groups\n";
        }
        catch (const json::exception& e) { std::cerr << "[Sequencer] JSON error: " << e.what() << "\n"; }
    }

    // 匯出指定群組的動畫資料（含播放設定）至獨立 JSON 檔
    void SequencerWindow::ExportSpecificGroupToJson(int idx, const std::string& filepath)
    {
        if (idx < 0 || idx >= (int)animationGroups.size()) return;
        for (auto& t : animationGroups[idx].tracks) t.SortKeyframeDatas();
        std::ofstream file(filepath);
        if (!file.is_open()) { std::cerr << "[Sequencer] Cannot open: " << filepath << "\n"; return; }
        file << json(animationGroups[idx]).dump(4);
        std::cout << "[Sequencer] Group[" << idx << "] exported to: " << filepath << "\n";
    }

    // 從 JSON 檔讀入指定群組的動畫並合併（追加至現有關鍵幀，不清空）
    void SequencerWindow::ImportSpecificGroupFromJson(int idx, const std::string& filepath)
    {
        if (idx < 0 || idx >= (int)animationGroups.size()) return;
        std::ifstream file(filepath);
        if (!file.is_open()) { std::cerr << "[Sequencer] Not found: " << filepath << "\n"; return; }
        try
        {
            AnimationGroup loaded = json::parse(file).get<AnimationGroup>();
            auto& target = animationGroups[idx];
            // 同步播放設定
            target.startFrame     = loaded.startFrame;
            target.endFrame       = loaded.endFrame;
            target.enableLoop     = loaded.enableLoop;
            target.loopStartFrame = loaded.loopStartFrame;
            // 合併關鍵幀
            for (int i = 0; i < (int)target.tracks.size() && i < (int)loaded.tracks.size(); ++i)
                for (auto& kf : loaded.tracks[i].keyframes)
                    target.tracks[i].keyframes.push_back(kf);
            std::cout << "[Sequencer] Group[" << idx << "] imported from: " << filepath << "\n";
        }
        catch (const json::exception& e) { std::cerr << "[Sequencer] JSON error: " << e.what() << "\n"; }
    }

} // namespace CG
