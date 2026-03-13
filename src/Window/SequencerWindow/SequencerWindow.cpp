#include "SequencerWindow.h"
#include<imgui.h>
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
        animationSequencer.Add(0);
        return true;
    }

    void SequencerWindow::Display()
    {

        if (ImGui::Begin("Animation Sequencer", nullptr))
        {
            if (targetScene)
            {
                int sequencerFlags =
                    ImSequencer::SEQUENCER_EDIT_STARTEND |  // drag segment edges
                    ImSequencer::SEQUENCER_ADD |  // "+" button
                    ImSequencer::SEQUENCER_DEL |  // delete button
                    ImSequencer::SEQUENCER_COPYPASTE |  // Ctrl+C / Ctrl+V
                    ImSequencer::SEQUENCER_CHANGE_FRAME;      // click to seek

                if (ImGui::Button(playing ? "Pause" : "Play"))
                    playing = !playing, lastTime = ImGui::GetTime();

                ImSequencer::Sequencer(&animationSequencer, &currentFrame, &expanded,&selectedEntry, &firstFrame, sequencerFlags);

                if (playing)
                {
                    double now = ImGui::GetTime(); //Second
                    timeAccumulated += now - lastTime;
                    lastTime = now;

                    if (timeAccumulated >= 0.01667) // 60 fps
                    {
                        timeAccumulated = 0;
                        currentFrame += 1.0;
                        if (currentFrame > animationSequencer.mFrameMax) currentFrame = animationSequencer.mFrameMin;

                        float timeCalcuated = (float)(currentFrame - animationSequencer.tracks[0].keyframes[0].startFrame) / (animationSequencer.tracks[0].keyframes[0].endFrame - animationSequencer.tracks[0].keyframes[0].startFrame);

                        float t = glm::clamp(timeCalcuated, 0.0f, 1.0f);
                        glm::vec3 currentPosition = (1 - t) * animationSequencer.tracks[0].keyframes[0].startPosition + t * animationSequencer.tracks[0].keyframes[0].endPosition;
                        glm::quat currentRotation = glm::slerp(animationSequencer.tracks[0].keyframes[0].startRotation, animationSequencer.tracks[0].keyframes[0].endRotation, t);
                        glm::vec3 currentScale = (1 - t) * animationSequencer.tracks[0].keyframes[0].startScale + t * animationSequencer.tracks[0].keyframes[0].endScale;
                    
                        animationSequencer.tracks[0].LinkedObject->SetRotation(currentRotation);
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
