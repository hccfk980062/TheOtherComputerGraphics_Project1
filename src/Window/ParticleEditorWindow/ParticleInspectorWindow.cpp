#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "ParticleEffects/ParticleTextureLoader.h"
#include "Window/ParticleEditorWindow/ParticleInspectorWindow.h"

namespace CG
{
    ParticleInspectorWindow::ParticleInspectorWindow()  {}
    ParticleInspectorWindow::~ParticleInspectorWindow() {}

    bool ParticleInspectorWindow::Initialize() { return true; }

    void ParticleInspectorWindow::Display()
    {
        // ── Main inspector window ──────────────────────────────────────────
        bool visible = ImGui::Begin("Particle Inspector");
        if (visible)
        {
            if (!m_scene || !m_scene->m_selectedEmitter)
                ImGui::TextDisabled("Select an emitter in the Hierarchy.");
            else
                DisplayEmitterPanel(m_scene->m_selectedEmitter);
        }
        ImGui::End();

        // ── Texture file dialog (independent top-level window) ─────────────
        HandleTextureFileDialog();
    }

    void ParticleInspectorWindow::HandleTextureFileDialog()
    {
        if (!ImGuiFileDialog::Instance()->Display("ChooseParticleTex",
            ImGuiWindowFlags_NoCollapse, ImVec2(640, 420)))
            return;

        if (ImGuiFileDialog::Instance()->IsOk() && m_texDialogTarget)
        {
            std::string filePath =
                ImGuiFileDialog::Instance()->GetFilePathName(IGFD_ResultMode_KeepInputFile);

            GLuint tid = LoadParticleTexture(filePath);
            if (tid != 0)
            {
                m_texDialogTarget->textureID   = tid;
                m_texDialogTarget->texturePath = filePath;
            }
        }

        m_texDialogTarget = nullptr;
        ImGuiFileDialog::Instance()->Close();
    }

    // ── Helper: labeled vec3 drag ─────────────────────────────────────────────

    static void DragVec3Pair(const char* label, const char* randLabel,
                             glm::vec3& base, glm::vec3& rand,
                             float speed = 0.01f, float lo = -1e6f, float hi = 1e6f)
    {
        ImGui::DragFloat3(label,     glm::value_ptr(base), speed, lo, hi);
        ImGui::DragFloat3(randLabel, glm::value_ptr(rand), speed, 0.0f, hi);
    }

    static void DragFloatPair(const char* label, const char* randLabel,
                              float& base, float& rand,
                              float speed = 0.01f, float lo = -1e6f, float hi = 1e6f)
    {
        ImGui::DragFloat(label,     &base, speed, lo, hi);
        ImGui::DragFloat(randLabel, &rand, speed, 0.0f, hi);
    }

    // ── Emitter Panel ─────────────────────────────────────────────────────────

    void ParticleInspectorWindow::DisplayEmitterPanel(EmitterBase* emitter)
    {
        // Name
        char buf[128];
        strncpy_s(buf, sizeof(buf), emitter->m_name.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name", buf, sizeof(buf)))
            emitter->m_name = buf;

        // Emitter type (read-only label)
        static const char* typeNames[] = { "Point", "Sphere", "Box", "Ring" };
        ImGui::LabelText("Emitter Type", "%s", typeNames[(int)emitter->m_type]);

        ImGui::Separator();

        // Spawn control
        if (ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragInt("Spawn Count",    &emitter->m_spawnCount,    1, 1, 1000);
            ImGui::DragInt("Max Particles",  &emitter->m_maxParticles,  1, 1, 10000);
            ImGui::DragFloat("Spawn Interval", &emitter->m_spawnInterval, 0.001f, 0.001f, 10.0f);
            ImGui::DragInt("Start Frame", &emitter->m_startFrame, 1, 0, emitter->m_endFrame);
            ImGui::DragInt("End Frame",   &emitter->m_endFrame,   1, emitter->m_startFrame, 9999);
        }

        // Shape-specific params
        if (ImGui::CollapsingHeader("Shape", ImGuiTreeNodeFlags_DefaultOpen))
            DisplayShapePanel(emitter);

        // Particle spawn properties
        if (ImGui::CollapsingHeader("Particle Properties", ImGuiTreeNodeFlags_DefaultOpen))
            DisplaySpawnPropsPanel(emitter->m_spawnProps);
    }

    void ParticleInspectorWindow::DisplayShapePanel(EmitterBase* emitter)
    {
        switch (emitter->m_type)
        {
        case EmitterType::Sphere:
            if (auto* se = dynamic_cast<SphereEmitter*>(emitter))
                ImGui::DragFloat("Radius", &se->m_radius, 0.01f, 0.0f, 100.0f);
            break;

        case EmitterType::Box:
            if (auto* be = dynamic_cast<BoxEmitter*>(emitter))
                ImGui::DragFloat3("Half Extents", glm::value_ptr(be->m_halfExtents), 0.01f, 0.0f, 100.0f);
            break;

        case EmitterType::Ring:
            if (auto* re = dynamic_cast<RingEmitter*>(emitter))
            {
                ImGui::DragFloat("Ring Radius",    &re->m_radius,    0.01f, 0.0f, 100.0f);
                ImGui::DragFloat("Band Width",     &re->m_bandWidth, 0.001f, 0.0f, 10.0f);
            }
            break;

        default:
            ImGui::TextDisabled("(no shape parameters)");
            break;
        }
    }

    void ParticleInspectorWindow::DisplaySpawnPropsPanel(EmitterSpawnProps& p)
    {
        // Particle type selector
        static const char* ptNames[] = { "Sprite", "Trail", "Ring" };
        int ptIdx = (int)p.particleType;
        if (ImGui::Combo("Particle Type", &ptIdx, ptNames, 3))
            p.particleType = (ParticleType)ptIdx;

        // Lifetime
        ImGui::Separator();
        ImGui::TextDisabled("Lifetime");
        DragFloatPair("##lt", "##ltR", p.lifetime, p.lifetimeRand, 0.01f, 0.001f, 100.0f);
        ImGui::SameLine(0, 4); ImGui::TextDisabled("+/-");

        // Position offset
        ImGui::Separator();
        ImGui::TextDisabled("Init Position  /  Offset Amplitude");
        DragVec3Pair("##pos", "##posR", p.initPos, p.initPosRand);

        // Velocity
        ImGui::TextDisabled("Init Velocity  /  Offset Amplitude");
        DragVec3Pair("##vel", "##velR", p.initVel, p.initVelRand);

        // Acceleration
        ImGui::TextDisabled("Init Accel  /  Offset Amplitude");
        DragVec3Pair("##acc", "##accR", p.initAccel, p.initAccelRand);

        // Rotation
        ImGui::Separator();
        ImGui::TextDisabled("Rot / RotVel / RotAccel  (rad)");
        DragFloatPair("Rot##v",      "Rot##r",      p.initRot,      p.initRotRand,      0.01f);
        DragFloatPair("RotVel##v",   "RotVel##r",   p.initRotVel,   p.initRotVelRand,   0.01f);
        DragFloatPair("RotAccel##v", "RotAccel##r", p.initRotAccel, p.initRotAccelRand, 0.01f);

        // Scale
        ImGui::Separator();
        ImGui::TextDisabled("Scale / ScaleVel / ScaleAccel");
        DragFloatPair("Scale##v",      "Scale##r",      p.initScale,      p.initScaleRand,      0.001f, 0.0f, 100.0f);
        DragFloatPair("ScaleVel##v",   "ScaleVel##r",   p.initScaleVel,   p.initScaleVelRand,   0.001f);
        DragFloatPair("ScaleAccel##v", "ScaleAccel##r", p.initScaleAccel, p.initScaleAccelRand, 0.001f);

        // Appearance
        ImGui::Separator();
        ImGui::TextDisabled("Appearance");
        ImGui::ColorEdit4("Color", glm::value_ptr(p.color));

        // ── Texture ───────────────────────────────────────────────────────
        ImGui::Separator();
        ImGui::TextDisabled("Particle Texture");

        if (p.textureID != 0)
        {
            // Thumbnail preview
            ImGui::Image(
                (ImTextureID)(intptr_t)p.textureID,
                ImVec2(64.0f, 64.0f),
                ImVec2(0, 0), ImVec2(1, 1));
            ImGui::SameLine();

            ImGui::BeginGroup();
            {
                // Truncate long paths so they fit in the panel
                std::string display = p.texturePath;
                if (display.size() > 42)
                    display = "..." + display.substr(display.size() - 39);
                ImGui::TextWrapped("%s", display.c_str());

                if (ImGui::Button("Clear##tex"))
                {
                    p.textureID   = 0;
                    p.texturePath = "";
                }
            }
            ImGui::EndGroup();
        }
        else
        {
            ImGui::TextDisabled("(no texture  —  solid color mode)");
        }

        if (ImGui::Button("Load Image...##tex"))
        {
            m_texDialogTarget = &p;
            IGFD::FileDialogConfig cfg;
            cfg.path            = ".";
            cfg.countSelectionMax = 1;
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseParticleTex",
                "Choose Particle Texture",
                ".png,.jpg,.jpeg,.bmp,.tga,.PNG,.JPG,.JPEG,.BMP,.TGA",
                cfg);
        }

        // Trail-specific
        if (p.particleType == ParticleType::Trail)
        {
            ImGui::Separator();
            ImGui::TextDisabled("Trail");
            ImGui::DragInt("Max Trail Length", &p.maxTrailLength, 1, 2, 200);
            ImGui::DragFloat("Trail Width",    &p.trailWidth,     0.001f, 0.001f, 10.0f);
        }

        // Ring-specific
        if (p.particleType == ParticleType::Ring)
        {
            ImGui::Separator();
            ImGui::TextDisabled("Ring Particle");
            ImGui::DragFloat("Inner Radius", &p.ringInnerRadius, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("Outer Radius", &p.ringOuterRadius, 0.01f, 0.0f, 100.0f);
            ImGui::DragInt("Segments",       &p.ringSegments,    1, 3, 128);
        }
    }
}
