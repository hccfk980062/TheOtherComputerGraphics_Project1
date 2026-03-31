#include "SequencerWindow.h"
namespace CG
{
    SequencerWindow::SequencerWindow() : targetScene(nullptr)
    {
    }

    SequencerWindow::~SequencerWindow()
    {
    }

    auto SequencerWindow::Initialize() -> bool
    {
        return true;
    }

    void SequencerWindow::Display()
    {

        if (ImGui::Begin("Animation Sequencer", nullptr))
        {
            if (targetScene)
            {
                // ── Top bar: transport controls ──────────────────────────────
                {
                    if (ImGui::Button("|<"))
                    {
                        currentFrame = startFrame;
                        TransformToFrame();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("<"))
                    {
                        currentFrame = std::max(startFrame, currentFrame - 1);
                        TransformToFrame();
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(80.f);
                    ImGui::DragInt("Frame", &currentFrame, 1, startFrame, endFrame);
                    ImGui::SameLine();
                    if (ImGui::Button(">"))
                    {
                        currentFrame = std::min(endFrame, currentFrame + 1);
                        TransformToFrame();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(">|"))
                    {
                        currentFrame = endFrame;
                        TransformToFrame();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Play / Pause"))
                    {
                        isPlaying = !isPlaying;
                        lastTime = ImGui::GetTime();
                        for (auto& group : animationGroups)
                        {
                            for (auto& track : group.tracks)
                                track.SortKeyframeDatas();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Add keyframe to current selected TimeLine"))
                    {
                        if (selectedAnimationTrack != nullptr)
                        {
                            KeyframeData data;
                            KeyframeData nil;

                            selectedAnimationTrack->GetClampedKeyframes(currentFrame, data, nil);
                            if (data.frame != currentFrame)
                                selectedAnimationTrack->keyframes.push_back(KeyframeData{ currentFrame, selectedAnimationTrack->linkedObject->transform.position, selectedAnimationTrack->linkedObject->transform.rotation, selectedAnimationTrack->linkedObject->transform.scale, selectedAnimationTrack });
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Export ALL Tracks to JSON"))
                    {
                        IGFD::FileDialogConfig config;
                        config.path = ".";
                        config.fileName = "ExportedAnimation.json";
                        ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ExportALL", "Export ALL Animation JSON to:", ".json", config);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Import ALL Tracks from JSON"))
                    {
                        IGFD::FileDialogConfig config;
                        config.path = ".";
                        ImGuiFileDialog::Instance()->OpenDialog("FileDialog_ImportALL", "Import ALL Animation JSON from:", ".json", config);
                    }
                    ImGui::Separator();
                }

                // ── Export / Import window  ─────────────────────────────────────────
               // 
                if (ImGuiFileDialog::Instance()->Display("FileDialog_ExportALL"))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        // action if OK
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        // action
                        for (auto& group : animationGroups)
                        {
                            for (auto& track : group.tracks)
                                track.SortKeyframeDatas();
                        }
                        ExportAllToJson(filePathName);
                    }

                    // close
                    ImGuiFileDialog::Instance()->Close();
                }
                if (ImGuiFileDialog::Instance()->Display("FileDialog_ImportALL"))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        // action if OK
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        // action
                        ImportAllFromJson(filePathName);
                    }

                    // close
                    ImGuiFileDialog::Instance()->Close();
                }

                // ── Sequencer widget ─────────────────────────────────────────
                // BeginNeoSequencer returns false if the widget is collapsed/clipped
                if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, {0,0}, 
                    ImGuiNeoSequencerFlags_EnableSelection | ImGuiNeoSequencerFlags_Selection_EnableDragging))
                {
                    int i = 0;
                    for (auto& group : animationGroups)
                    {
                        std::string TrackName = group.groupName;
                        bool isTimelineGroupSelected = false;

                        // ── 新增：每個 Group 旁的單獨匯出/入按鈕 ──────────────────
                        ImGui::PushID(i);  // 確保按鈕 ID 不衝突

                        std::string exportBtnLabel = "Export##" + std::to_string(i);
                        std::string importBtnLabel = "Import##" + std::to_string(i);

                        if (ImGui::SmallButton(exportBtnLabel.c_str()))
                        {
                            selectedGroupIndex = i;   // 記住要匯出哪個 Group
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            config.fileName = group.groupName + "_Animation.json";
                            ImGuiFileDialog::Instance()->OpenDialog(
                                "FileDialog_ExportGroup", "Export Group Animation to:", ".json", config);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton(importBtnLabel.c_str()))
                        {
                            selectedGroupIndex = i;   // 記住要匯入哪個 Group
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            ImGuiFileDialog::Instance()->OpenDialog(
                                "FileDialog_ImportGroup", "Import Group Animation from:", ".json", config);
                        }
                        ImGui::SameLine();

                        ImGui::PopID();
                        if (ImGui::BeginNeoGroup(TrackName.c_str(), &transformTabOpen[i], &isTimelineGroupSelected))
                        {
                            for (auto&track : group.tracks)
                            {
                                if (ImGui::BeginNeoTimelineEx(track.trackName.c_str()))
                                {
                                    for (auto&& v : track.keyframes)
                                    {
                                        ImGui::NeoKeyframe(&v.frame);

                                        if (ImGui::IsNeoKeyframeRightClicked())
                                        {
                                            std::cout << "Selected keyframe: " << v.frame << " with position: " << v.position.x << std::endl;

                                            //Show the pop up keyframe editor window
                                            selectedKeyframe = &v;                        // 記住目標
                                            ImGui::OpenPopup("KeyframeEditor");           // 觸發 Popup
                                        }
                                    }

                                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                                    {
                                        if (ImGui::IsNeoTimelineSelected())
                                        {
                                            std::cout << "Seleted Track: " << track.trackName <<"\n";
                                            selectedAnimationTrack = &track;
                                        }
                                    }
                                    ImGui::EndNeoTimeLine();
                                }
                            }

                            ImGui::EndNeoGroup();
                        }

                        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                        {
                            if (isTimelineGroupSelected) // 當 NeoGroup被選取時，傳回True
                            {
                                std::cout << "Seleted Timeline Track: " << i << "\n";
                            }
                        }
                        i++;
                    }
                    if (ImGuiFileDialog::Instance()->Display("FileDialog_ExportGroup"))
                    {
                        if (ImGuiFileDialog::Instance()->IsOk())
                        {
                            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                            ExportGroupToJson(selectedGroupIndex, path);
                        }
                        ImGuiFileDialog::Instance()->Close();
                    }
                    if (ImGuiFileDialog::Instance()->Display("FileDialog_ImportGroup"))
                    {
                        if (ImGuiFileDialog::Instance()->IsOk())
                        {
                            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                            ImportGroupFromJson(selectedGroupIndex, path);
                        }
                        ImGuiFileDialog::Instance()->Close();
                    }
                    if (ImGui::BeginPopup("KeyframeEditor"))
                    {
                        if (selectedKeyframe)
                        {
                            ImGui::Text("Edit Keyframe  [Frame %d]", selectedKeyframe->frame);
                            ImGui::Separator();

                            // 幀號（允許移動關鍵幀位置）
                            ImGui::DragInt("Frame", &selectedKeyframe->frame, 1, startFrame, endFrame);

                            // 位置
                            ImGui::DragFloat3("Position", glm::value_ptr(selectedKeyframe->position), 0.1f);

                            // 旋轉（用 Euler 顯示比較直觀，存時轉回 quat）
                            glm::vec3 euler = glm::degrees(glm::eulerAngles(selectedKeyframe->rotation));
                            if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.5f))
                                selectedKeyframe->rotation = glm::quat(glm::radians(euler));

                            // 縮放
                            ImGui::DragFloat3("Scale", glm::value_ptr(selectedKeyframe->scale), 0.01f);

                            ImGui::Separator();

                            // 修改後重新排序（以防幀號被改動）
                            if (ImGui::Button("Apply & Close"))
                            {
                                for (auto& group : animationGroups)
                                {
                                    for (auto& track : group.tracks)
                                        track.SortKeyframeDatas();
                                }
                                selectedKeyframe = nullptr;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete"))
                            {
                                auto& kfs = selectedKeyframe->sourceTrack->keyframes;
                                kfs.erase(std::remove_if(kfs.begin(), kfs.end(),
                                    [this](const KeyframeData& k) { return &k == selectedKeyframe; }),
                                    kfs.end());
                                selectedKeyframe = nullptr;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::EndNeoSequencer();
                }

                if (isPlaying)
                {
                    double now = ImGui::GetTime(); //Second
                    timeAccumulated += now - lastTime;
                    lastTime = now;

                    if (timeAccumulated >= 0.01667) // 60 fps
                    {
                        timeAccumulated = 0;
                        currentFrame += 1;
                        if (currentFrame > endFrame) currentFrame = startFrame;

                        TransformToFrame();
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Scene Assigned!");
            }
        }
        ImGui::End();
    }


    void SequencerWindow::ExportAllToJson(const std::string& filepath)
    {
        json j = animationGroups;   // 自動呼叫 to_json

        std::ofstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "[Sequencer] 無法開啟檔案: " << filepath << "\n";
            return;
        }

        file << j.dump(4);          // 縮排 4 格，方便人工閱讀
        std::cout << "[Sequencer] 已匯出至: " << filepath << "\n";
    }
    void SequencerWindow::ImportAllFromJson(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "[Sequencer] 找不到檔案: " << filepath << "\n";
            return;
        }

        try
        {
            json j = json::parse(file);
            animationGroups = j.get<std::vector<AnimationGroup>>(); // 自動呼叫 from_json
            std::cout << "[Sequencer] 已載入 " << animationGroups.size() << " 條軌道\n";

            for (auto& group : animationGroups)
            {
                for (auto& track : group.tracks)
                {
                    for (auto& keyframe : track.keyframes)
                    {
                        keyframe.sourceTrack = &track;
                    }

                    for (auto& obj : targetScene->GetObjectsInAnimationGroup(group.groupName))
                    {
                        if (obj->animationSerializedName == track.trackName)
                        {
                            track.linkedObject = obj;
                        }
                    }
                }
            }
        }
        catch (const json::exception& e)
        {
            std::cerr << "[Sequencer] JSON 解析錯誤: " << e.what() << "\n";
        }
    }

    // ── 匯出單一 Group ───────────────────────────────────
    void SequencerWindow::ExportGroupToJson(int groupIndex, const std::string& filepath)
    {
        if (groupIndex < 0 || groupIndex >= (int)animationGroups.size()) return;

        // 先排序，確保關鍵幀順序正確
        for (auto& track : animationGroups[groupIndex].tracks)
            track.SortKeyframeDatas();

        json j = animationGroups[groupIndex];   // 呼叫既有的 to_json(AnimationGroup)

        std::ofstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "[Sequencer] 無法開啟檔案: " << filepath << "\n";
            return;
        }
        file << j.dump(4);
        std::cout << "[Sequencer] Group[" << groupIndex << "] 已匯出至: " << filepath << "\n";
    }

    // ── 匯入單一 Group (只覆蓋對應索引的軌道資料) ─────────────────
    void SequencerWindow::ImportGroupFromJson(int groupIndex, const std::string& filepath)
    {
        if (groupIndex < 0 || groupIndex >= (int)animationGroups.size()) return;

        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "[Sequencer] 找不到檔案: " << filepath << "\n";
            return;
        }

        try
        {
            json j = json::parse(file);
            AnimationGroup loadedGroup = j.get<AnimationGroup>(); // 呼叫既有的 from_json

            // 修復指標：sourceTrack & linkedObject
            for (auto& track : loadedGroup.tracks)
            {
                for (auto& keyframe : track.keyframes)
                    keyframe.sourceTrack = &track;

                // 根據 trackName 重新連結場景物件
                for (auto* obj : targetScene->GetObjectsInAnimationGroup(animationGroups[groupIndex].groupName))
                {
                    if (obj->animationSerializedName == track.trackName)
                    {
                        track.linkedObject = obj;
                        break;
                    }
                }
            }

            // 只替換指定索引的 Group，其餘 Group 保持不變
            animationGroups[groupIndex] = std::move(loadedGroup);

            std::cout << "[Sequencer] Group[" << groupIndex << "] 已載入自: " << filepath << "\n";
        }
        catch (const json::exception& e)
        {
            std::cerr << "[Sequencer] JSON 解析錯誤: " << e.what() << "\n";
        }
    }
}
