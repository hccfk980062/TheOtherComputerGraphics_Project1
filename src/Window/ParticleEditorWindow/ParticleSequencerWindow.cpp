#include "Window/ParticleEditorWindow/ParticleSequencerWindow.h"

namespace CG
{
    void ParticleSequencerWindow::Display(ParticleEditorScene* scene)
    {
        // ── 播放控制列 ────────────────────────────────────────────────────────
        if (ImGui::Button(scene->isPlaying ? "Pause##pe" : "Play ##pe"))
            scene->isPlaying = !scene->isPlaying;

        ImGui::SameLine();
        if (ImGui::Button("Reset##pe"))
        {
            scene->currentTime = 0.0f;
            scene->isPlaying   = false;
        }

        ImGui::SameLine();
        ImGui::Text("Time: %.2fs", scene->currentTime);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::DragFloat("End##pe", &scene->timelineEnd, 0.1f, 1.0f, 600.0f, "%.1fs");

        ImGui::Separator();

        // ── NeoSequencer ─────────────────────────────────────────────────────
        int currentFrame = (int)(scene->currentTime * FPS);
        int startFrame   = 0;
        int endFrame     = (int)(scene->timelineEnd * FPS);

        if (ImGui::BeginNeoSequencer("##pe_seq",
            &currentFrame, &startFrame, &endFrame,
            { 0, 0 },
            ImGuiNeoSequencerFlags_EnableSelection))
        {
            // NeoSequencer 可能因使用者點擊時間軸而修改 currentFrame
            scene->currentTime = (float)currentFrame / (float)FPS;

            std::vector<EmitterBase*> allEmitters;
            scene->CollectAll(allEmitters);

            for (auto* emitter : allEmitters)
            {
                ImGui::PushID(emitter);

                if (ImGui::BeginNeoTimelineEx(emitter->name.c_str()))
                {
                    // 兩個關鍵幀標記：startTime 與 endTime
                    int sf = (int)(emitter->startTime * FPS);
                    int ef = (int)(emitter->endTime   * FPS);

                    ImGui::NeoKeyframe(&sf);
                    ImGui::NeoKeyframe(&ef);

                    // 若使用者拖動關鍵幀，NeoKeyframe 會修改 sf / ef
                    float newStart = (float)sf / (float)FPS;
                    float newEnd   = (float)ef / (float)FPS;
                    if (newStart < newEnd)  // 保證 start < end
                    {
                        emitter->startTime = newStart;
                        emitter->endTime   = newEnd;
                    }

                    // 點擊軌道選取對應 Emitter
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                        ImGui::IsNeoTimelineSelected())
                    {
                        scene->selectedEmitter = emitter;
                    }

                    ImGui::EndNeoTimeLine();
                }

                ImGui::PopID();
            }

            ImGui::EndNeoSequencer();
        }
    }

} // namespace CG
