#pragma once
#include <imgui.h>
#include <ImNeoSequencer/imgui_neo_sequencer.h>
#include "Scene/ParticleEditorScene.h"

namespace CG
{
    // Single-timeline sequencer for the particle editor.
    // Each emitter has two keyframe markers (startFrame / endFrame)
    // that define when it is allowed to emit new particles.
    class ParticleSequencerWindow
    {
    public:
        ParticleSequencerWindow();
        ~ParticleSequencerWindow();

        bool Initialize();
        void Display();

        void SetScene(ParticleEditorScene* scene) { m_scene = scene; }

    private:
        ParticleEditorScene* m_scene = nullptr;

        void DrawEmitterTimeline(EmitterBase* emitter);
        void DoExport();
        void DoImport();
    };
}
