#pragma once
#include <imgui.h>
#include <ImNeoSequencer/imgui_neo_sequencer.h>
#include <ImNeoSequencer/imgui_neo_internal.h>
#include "Scene/ParticleEditorScene.h"

namespace CG
{
    // 時間軸視窗：單一 NeoSequencer，每個 Emitter 顯示為一條軌道
    // 軌道上有兩個關鍵幀標記 Emitter 的 startTime / endTime
    class ParticleSequencerWindow
    {
    public:
        void Display(ParticleEditorScene* scene);

    private:
        static constexpr int FPS = 60;  // 時間軸每秒的幀數
    };

} // namespace CG
