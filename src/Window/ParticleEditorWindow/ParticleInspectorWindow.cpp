#include "Window/ParticleEditorWindow/ParticleInspectorWindow.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace CG
{
    void ParticleInspectorWindow::Display(ParticleEditorScene* scene)
    {
        ImGui::Text("Inspector");
        ImGui::Separator();

        EmitterBase* e = scene->selectedEmitter;
        if (!e)
        {
            ImGui::TextDisabled("(No emitter selected)");
            return;
        }

        // 名稱編輯
        char nameBuf[128];
        strncpy_s(nameBuf, e->name.c_str(), sizeof(nameBuf) - 1);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            e->name = nameBuf;

        ImGui::Spacing();
        DrawTypeSpecificProps(e);
        DrawEmissionProps(e);
        DrawTimelineProps(e);
        DrawEmitterMotionProps(e);
        DrawParticleProps(e);
    }

    void ParticleInspectorWindow::DrawTypeSpecificProps(EmitterBase* e)
    {
        if (!ImGui::CollapsingHeader("Type Properties", ImGuiTreeNodeFlags_DefaultOpen)) return;

        switch (e->type)
        {
        case EmitterType::Point:
            ImGui::TextDisabled("Point: no extra settings");
            break;

        case EmitterType::Sphere:
            if (auto* s = dynamic_cast<SphereEmitter*>(e))
            {
                ImGui::DragFloat("Radius##sph", &s->radius, 0.01f, 0.01f, 50.0f);
                ImGui::Checkbox ("Converge to Center", &s->convergeToCenter);
                if (s->convergeToCenter)
                {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(80.0f);
                    ImGui::DragFloat("Speed##conv", &s->convergeSpeed, 0.01f, 0.01f, 50.0f);
                }
            }
            break;

        case EmitterType::Box:
            if (auto* b = dynamic_cast<BoxEmitter*>(e))
                ImGui::DragFloat3("Extents##box", glm::value_ptr(b->extents), 0.01f, 0.01f, 50.0f);
            break;

        case EmitterType::Ring:
            if (auto* r = dynamic_cast<RingEmitter*>(e))
            {
                ImGui::DragFloat("Radius##ring", &r->radius, 0.01f, 0.01f, 50.0f);
                ImGui::DragFloat("Width##ring",  &r->width,  0.01f, 0.001f, 10.0f);
            }
            break;

        case EmitterType::Ribbon:
            if (auto* r = dynamic_cast<RibbonEmitter*>(e))
            {
                ImGui::DragFloat("Ribbon Width", &r->ribbonWidth, 0.01f, 0.01f, 10.0f);
                ImGui::ColorEdit3("Ribbon Color", glm::value_ptr(r->ribbonColor));
            }
            break;
        }
    }

    void ParticleInspectorWindow::DrawEmissionProps(EmitterBase* e)
    {
        if (e->type == EmitterType::Ribbon) return;  // Ribbon 不使用粒子發射
        if (!ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen)) return;

        ImGui::DragInt  ("Emit Count",    &e->emitCount,    1, 1, 1000);
        ImGui::DragInt  ("Max Particles", &e->maxParticles, 1, 1, 10000);
        ImGui::DragFloat("Interval (s)",  &e->emitInterval, 0.001f, 0.001f, 10.0f);
    }

    void ParticleInspectorWindow::DrawTimelineProps(EmitterBase* e)
    {
        if (!ImGui::CollapsingHeader("Timeline", ImGuiTreeNodeFlags_DefaultOpen)) return;

        ImGui::DragFloat("Start Time (s)", &e->startTime, 0.01f, 0.0f, e->endTime - 0.01f);
        ImGui::DragFloat("End Time (s)",   &e->endTime,   0.01f, e->startTime + 0.01f, 3600.0f);
    }

    void ParticleInspectorWindow::DrawEmitterMotionProps(EmitterBase* e)
    {
        if (!ImGui::CollapsingHeader("Emitter Motion")) return;

        ImGui::DragFloat3("Position##em",          glm::value_ptr(e->position),        0.01f);
        ImGui::DragFloat3("Velocity##em",          glm::value_ptr(e->velocity),        0.01f);
        ImGui::DragFloat3("Acceleration##em",      glm::value_ptr(e->acceleration),    0.001f);
        ImGui::Spacing();

        // 旋轉以尤拉角顯示
        glm::vec3 euler = glm::degrees(glm::eulerAngles(e->rotation));
        if (ImGui::DragFloat3("Rotation (Euler)", glm::value_ptr(euler), 0.5f))
            e->rotation = glm::quat(glm::radians(euler));

        ImGui::DragFloat3("Angular Velocity",  glm::value_ptr(e->angularVelocity), 0.01f);
    }

    void ParticleInspectorWindow::DrawParticleProps(EmitterBase* e)
    {
        if (e->type == EmitterType::Ribbon) return;
        if (!ImGui::CollapsingHeader("Particle Settings")) return;

        ImGui::DragFloat ("Life (s)",          &e->particleLife,                 0.01f, 0.01f, 60.0f);
        ImGui::DragFloat ("Size",              &e->particleSize,                 0.001f, 0.001f, 10.0f);
        ImGui::ColorEdit4("Color",             glm::value_ptr(e->particleColor));
        ImGui::Spacing();
        ImGui::DragFloat3("Init Velocity",     glm::value_ptr(e->particleInitVelocity),    0.01f);
        ImGui::DragFloat3("Acceleration",      glm::value_ptr(e->particleAcceleration),    0.001f);
        ImGui::DragFloat3("Angular Velocity",  glm::value_ptr(e->particleAngularVelocity), 0.01f);
        ImGui::Checkbox  ("Rot Follows Emitter", &e->particleRotFollowEmitter);
    }

} // namespace CG
