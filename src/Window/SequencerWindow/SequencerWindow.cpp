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
        animationTracks.push_back(AnimationTrack());
        animationTracks[0].trackName = "Trasform";
        animationTracks[0].keyframes.push_back(KeyframeData{ 0, glm::vec3(0.0f) });
        animationTracks[0].keyframes.push_back(KeyframeData{ 30, glm::vec3(10.0f) });
        animationTracks[0].keyframes.push_back(KeyframeData{ 60, glm::vec3(00.0f) });
        animationTracks[0].keyframes.push_back(KeyframeData{ 90, glm::vec3(-10.0f) });
        animationTracks[0].keyframes.push_back(KeyframeData{ 120, glm::vec3(0.0f) });
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
                    animationTracks[0].SortKeyframeDatas();
                }
                ImGui::Separator();

                // ── Sequencer widget ─────────────────────────────────────────
                // BeginNeoSequencer returns false if the widget is collapsed/clipped
                if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, {0,0}, 
                    ImGuiNeoSequencerFlags_EnableSelection |
                    ImGuiNeoSequencerFlags_Selection_EnableDragging
                    ))
                {
                    if (ImGui::BeginNeoGroup("Transform", &transformOpen)) 
                    {
                        if (ImGui::BeginNeoTimelineEx("Position"))
                        {
                            for (auto&& v : animationTracks[0].keyframes)
                            {
                                ImGui::NeoKeyframe(&v.frame);
                                /*
                                if (ImGui::IsNeoKeyframeSelected())
                                {
                                    std::cout << "Selected keyframe: " << v.frame << " with position: " << v.position.x << std::endl;
                                }
                                */
                            }
                            ImGui::EndNeoTimeLine();
                        }
                        ImGui::EndNeoGroup();
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


                        KeyframeData previousFrameData;
                        KeyframeData nextFrameData;

                        animationTracks[0].GetClampedKeyframes(currentFrame, previousFrameData, nextFrameData);

                        float t = 0.0f;
                        int range = nextFrameData.frame - previousFrameData.frame;
                        if (range > 0)
                            t = float(currentFrame - previousFrameData.frame) / float(range);

                        t = glm::clamp(t, 0.0f, 1.0f);

                        glm::vec3 pos = glm::mix(previousFrameData.position, nextFrameData.position, t);
                        glm::quat rot = glm::slerp(previousFrameData.rotation, nextFrameData.rotation, t);
                        glm::vec3 scl = glm::mix(previousFrameData.scale, nextFrameData.scale, t);

                        animationTracks[0].linkedObject->SetPosition(pos);
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
