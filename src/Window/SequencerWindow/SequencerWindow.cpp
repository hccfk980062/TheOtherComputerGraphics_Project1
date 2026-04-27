#include "SequencerWindow.h"

namespace CG
{
    SequencerWindow::SequencerWindow() : targetScene(nullptr), m_cmdStack(nullptr) {}
    SequencerWindow::~SequencerWindow() {}

    auto SequencerWindow::Initialize() -> bool { return true; }

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

        // ── 播放控制列 ────────────────────────────────────────────────────────
        {
            // 跳到開頭
            if (ImGui::Button("|<"))
            { currentFrame = startFrame; TransformToFrame(); }
            ImGui::SameLine();
            // 後退一幀
            if (ImGui::Button("<"))
            { currentFrame = std::max(startFrame, currentFrame - 1); TransformToFrame(); }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.f);
            ImGui::DragInt("Frame", &currentFrame, 1, startFrame, endFrame);
            ImGui::SameLine();
            // 前進一幀
            if (ImGui::Button(">"))
            { currentFrame = std::min(endFrame, currentFrame + 1); TransformToFrame(); }
            ImGui::SameLine();
            // 跳到結尾
            if (ImGui::Button(">|"))
            { currentFrame = endFrame; TransformToFrame(); }

            ImGui::SameLine();
            // 播放/暫停切換，同時對所有軌道排序以確保插值正確
            if (ImGui::Button("Play / Pause"))
            {
                isPlaying = !isPlaying;
                lastTime  = ImGui::GetTime();
                for (auto& g : animationGroups)
                    for (auto& t : g.tracks) t.SortKeyframeDatas();
            }

            ImGui::SameLine();
            // 新增關鍵幀：將 selectedAnimationTrack 在 currentFrame 的當前 Transform 記錄下來
            if (ImGui::Button("Add Keyframe"))
            {
                if (selectedAnimationTrack && m_cmdStack)
                {
                    // 同一幀只允許一個關鍵幀，先檢查是否已存在
                    KeyframeData existing, dummy;
                    selectedAnimationTrack->GetClampedKeyframes(currentFrame, existing, dummy);
                    if (existing.frame != currentFrame)
                    {
                        KeyframeData kf{
                            currentFrame,
                            selectedAnimationTrack->linkedObject->transform.position,
                            selectedAnimationTrack->linkedObject->transform.rotation,
                            selectedAnimationTrack->linkedObject->transform.scale,
                            selectedAnimationTrack
                        };
                        m_cmdStack->Execute(std::make_unique<AddKeyframeCommand>(selectedAnimationTrack, kf));
                    }
                }
                else if (selectedAnimationTrack && !m_cmdStack)
                {
                    // 無 CommandStack 時退化為直接修改（不支援 Undo）
                    KeyframeData existing, dummy;
                    selectedAnimationTrack->GetClampedKeyframes(currentFrame, existing, dummy);
                    if (existing.frame != currentFrame)
                        selectedAnimationTrack->keyframes.push_back({
                            currentFrame,
                            selectedAnimationTrack->linkedObject->transform.position,
                            selectedAnimationTrack->linkedObject->transform.rotation,
                            selectedAnimationTrack->linkedObject->transform.scale,
                            selectedAnimationTrack
                        });
                }
            }

            // ── JSON 匯出入按鈕（透過 ImGuiFileDialog 選擇路徑）──────────────
            ImGui::SameLine();
            if (ImGui::Button("Export ALL"))
            {
                IGFD::FileDialogConfig cfg; cfg.path = "."; cfg.fileName = "ExportedAnimation.json";
                ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ExportALL", "Export ALL Animation JSON to:", ".json", cfg);
            }
            ImGui::SameLine();
            if (ImGui::Button("Import ALL"))
            {
                IGFD::FileDialogConfig cfg; cfg.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ImportALL", "Import ALL Animation JSON from:", ".json", cfg);
            }
            ImGui::SameLine();
            if (ImGui::Button("Export Selected") && selectedGroupIndex >= 0 && selectedGroupIndex < (int)animationGroups.size())
            {
                IGFD::FileDialogConfig cfg; cfg.path = ".";
                cfg.fileName = "ExportedAnimation_" + animationGroups[selectedGroupIndex].groupName + ".json";
                ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ExportSpecific",
                    "Export Group " + animationGroups[selectedGroupIndex].groupName + " to:", ".json", cfg);
            }
            ImGui::SameLine();
            if (ImGui::Button("Import Selected") && selectedGroupIndex >= 0 && selectedGroupIndex < (int)animationGroups.size())
            {
                IGFD::FileDialogConfig cfg; cfg.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ImportSpecific",
                    "Import Group " + animationGroups[selectedGroupIndex].groupName + " from:", ".json", cfg);
            }
            ImGui::Separator();
        }

        // ── 檔案對話框結果處理 ────────────────────────────────────────────────
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
                for (auto& g : animationGroups) for (auto& t : g.tracks) t.SortKeyframeDatas();
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

        // ── ImNeoSequencer 主體 ───────────────────────────────────────────────
        // 每幀渲染前清空待刪除緩衝（在 BeginNeoTimelineEx scope 內收集，EndNeoSequencer 後消費）
        m_pendingDeleteKeyframes.clear();

        if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, { 0, 0 },
            ImGuiNeoSequencerFlags_EnableSelection          |
            ImGuiNeoSequencerFlags_Selection_EnableDragging |
            ImGuiNeoSequencerFlags_Selection_EnableDeletion))
        {
            int groupIdx = 0;
            for (auto& group : animationGroups)
            {
                bool isGroupSelected = false;
                if (ImGui::BeginNeoGroup(group.groupName.c_str(), &transformTabOpen[groupIdx], &isGroupSelected))
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
                                    {
                                        for (auto& kf : track.keyframes)
                                        {
                                            if (kf.frame == selFrame)
                                            {
                                                m_pendingDeleteKeyframes.emplace_back(&track, kf);
                                                break;
                                            }
                                        }
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
                    ImGui::EndNeoGroup();
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isGroupSelected)
                    selectedGroupIndex = groupIdx;

                ++groupIdx;
            }

            // ── 關鍵幀編輯 Popup（右鍵點擊關鍵幀後開啟）────────────────────
            if (ImGui::BeginPopup("KeyframeEditor"))
            {
                if (selectedKeyframe)
                {
                    ImGui::Text("Edit Keyframe  [Frame %d]", selectedKeyframe->frame);
                    ImGui::Separator();

                    ImGui::DragInt("Frame",    &selectedKeyframe->frame,    1, startFrame, endFrame);
                    ImGui::DragFloat3("Position", glm::value_ptr(selectedKeyframe->position), 0.1f);

                    // 旋轉以尤拉角顯示，輸入後轉回四元數
                    glm::vec3 euler = glm::degrees(glm::eulerAngles(selectedKeyframe->rotation));
                    if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.5f))
                        selectedKeyframe->rotation = glm::quat(glm::radians(euler));

                    ImGui::DragFloat3("Scale", glm::value_ptr(selectedKeyframe->scale), 0.01f);
                    ImGui::Separator();

                    if (ImGui::Button("Apply & Close"))
                    {
                        for (auto& g : animationGroups) for (auto& t : g.tracks) t.SortKeyframeDatas();
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
                            // 無 CommandStack 時直接刪除（不支援 Undo）
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

            // ── 按下 Delete / Backspace 批次刪除選取的關鍵幀 ────────────────
            // m_pendingDeleteKeyframes 可能含有重複項目（多 track 迭代造成），
            // BatchDeleteKeyframesCommand 以幀號唯一性去重
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

        // ── 播放時鐘：每累積 ≥ 16ms（≈ 60fps）推進一幀 ─────────────────────
        if (isPlaying)
        {
            double now = ImGui::GetTime();
            timeAccumulated += now - lastTime;
            lastTime = now;

            if (timeAccumulated >= 0.015)
            {
                timeAccumulated = 0;
                ++currentFrame;
                // 超出結尾時從 middleRepeatFrame 或 startFrame 重新開始
                if (currentFrame > endFrame)
                    currentFrame = repeatFromMiddle ? middleRepeatFrame : startFrame;
                TransformToFrame();
            }
        }

        ImGui::End();
    }

    // ─── JSON 匯出入實作 ───────────────────────────────────────────────────────

    // 將所有動畫群組序列化並寫入 JSON 檔（縮排 4 格）
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

    // 匯出指定群組的動畫資料至獨立 JSON 檔
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
            loaded.groupName = animationGroups[idx].groupName;  // 確保群組名稱一致
            for (int i = 0; i < (int)animationGroups[idx].tracks.size(); ++i)
            {
                for (auto& kf : loaded.tracks[i].keyframes)
                    animationGroups[idx].tracks[i].keyframes.push_back(kf);
            }
            std::cout << "[Sequencer] Group[" << idx << "] imported from: " << filepath << "\n";
        }
        catch (const json::exception& e) { std::cerr << "[Sequencer] JSON error: " << e.what() << "\n"; }
    }

} // namespace CG
