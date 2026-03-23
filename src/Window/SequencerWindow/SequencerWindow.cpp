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
                if (ImGui::Button("|<")) currentFrame = startFrame;
                ImGui::SameLine();
                if (ImGui::Button("<"))  currentFrame = std::max(startFrame, currentFrame - 1);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.f);
                ImGui::DragInt("Frame", &currentFrame, 1, startFrame, endFrame);
                ImGui::SameLine();
                if (ImGui::Button(">"))  currentFrame = std::min(endFrame, currentFrame + 1);
                ImGui::SameLine();
                if (ImGui::Button(">|")) currentFrame = endFrame;
                ImGui::SameLine();
                if (ImGui::Button("Play / Pause"))
                {
                    isPlaying = !isPlaying;
                    lastTime = ImGui::GetTime();
                    for (int i = 0; i < animationTracks.size(); i++)
                    {
                        animationTracks[i].SortKeyframeDatas();
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
                ImGui::Separator();

                // ── Sequencer widget ─────────────────────────────────────────
                // BeginNeoSequencer returns false if the widget is collapsed/clipped
                if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, {0,0}, 
                    ImGuiNeoSequencerFlags_EnableSelection | ImGuiNeoSequencerFlags_Selection_EnableDragging
                    ))
                {
                    if (ImGui::BeginNeoGroup("Transform", &transformTabOpen)) 
                    {
                        for(int i =0; i<animationTracks.size(); i++)
                        {
                            if (ImGui::BeginNeoTimelineEx(animationTracks[i].trackName.c_str()))
                            {
                                for (auto&& v : animationTracks[i].keyframes)
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

                                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) )
                                {
                                    if(ImGui::IsNeoTimelineSelected())
                                    {
                                        selectedAnimationTrack = &animationTracks[i];
                                    }
                                }
                                ImGui::EndNeoTimeLine();
                            }
                        }



                        ImGui::EndNeoGroup();
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
                                animationTracks[0].SortKeyframeDatas();
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

                        for(int i=0;i< animationTracks.size();i++)
                        {
                            if (animationTracks[i].keyframes.size() == 0) continue;

                            KeyframeData previousFrameData;
                            KeyframeData nextFrameData;

                            animationTracks[i].GetClampedKeyframes(currentFrame, previousFrameData, nextFrameData);

                            float t = 0.0f;
                            int range = nextFrameData.frame - previousFrameData.frame;
                            if (range > 0)
                                t = float(currentFrame - previousFrameData.frame) / float(range);

                            t = glm::clamp(t, 0.0f, 1.0f);

                            glm::vec3 pos = glm::mix(previousFrameData.position, nextFrameData.position, t);
                            glm::quat rot = glm::slerp(previousFrameData.rotation, nextFrameData.rotation, t);
                            glm::vec3 scl = glm::mix(previousFrameData.scale, nextFrameData.scale, t);

                            animationTracks[i].linkedObject->SetPosition(pos);
                            animationTracks[i].linkedObject->SetRotation(rot);
                            animationTracks[i].linkedObject->SetScale(scl);
                        }
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
}
