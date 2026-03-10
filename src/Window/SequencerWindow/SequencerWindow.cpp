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

                ImSequencer::Sequencer(
                    &animationSequencer,
                    &currentFrame,   // playhead position (read/write)
                    &expanded,       // expand/collapse all tracks
                    &selectedEntry,  // which track row is selected
                    &firstFrame,     // horizontal scroll offset (first visible frame)
                    sequencerFlags
                );

                if (playing)
                {
                    double now = ImGui::GetTime(); //Second
                    timeAccumulated += now - lastTime;
                    lastTime = now;

                    if (timeAccumulated >= 0.01667) // 60 fps
                    {
                        timeAccumulated = 0;
                        currentFrame += 1.0;
                        if (currentFrame > animationSequencer.mFrameMax)
                            currentFrame = animationSequencer.mFrameMin;
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
