#pragma once
#include <string>
#include <vector>
#include <memory>
#include <fstream>

#include <json.hpp>

#include "ParticleEffects/EmitterBase.h"
#include "ParticleEffects/ConcreteEmitters.h"

namespace CG
{
    // ── GLM conversion helpers ────────────────────────────────────────────────
    inline nlohmann::json ps_v3(const glm::vec3& v) { return {v.x, v.y, v.z}; }
    inline glm::vec3 ps_fv3(const nlohmann::json& j)
    { return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()}; }

    inline nlohmann::json ps_v4(const glm::vec4& v) { return {v.x, v.y, v.z, v.w}; }
    inline glm::vec4 ps_fv4(const nlohmann::json& j)
    { return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()}; }

    // ── EmitterSpawnProps serialization ───────────────────────────────────────
    inline nlohmann::json SpawnPropsToJson(const EmitterSpawnProps& p)
    {
        nlohmann::json j;
        j["initPos"]            = ps_v3(p.initPos);
        j["initPosRand"]        = ps_v3(p.initPosRand);
        j["initVel"]            = ps_v3(p.initVel);
        j["initVelRand"]        = ps_v3(p.initVelRand);
        j["initAccel"]          = ps_v3(p.initAccel);
        j["initAccelRand"]      = ps_v3(p.initAccelRand);
        j["initRot"]            = p.initRot;
        j["initRotRand"]        = p.initRotRand;
        j["initRotVel"]         = p.initRotVel;
        j["initRotVelRand"]     = p.initRotVelRand;
        j["initRotAccel"]       = p.initRotAccel;
        j["initRotAccelRand"]   = p.initRotAccelRand;
        j["initScale"]          = p.initScale;
        j["initScaleRand"]      = p.initScaleRand;
        j["initScaleVel"]       = p.initScaleVel;
        j["initScaleVelRand"]   = p.initScaleVelRand;
        j["initScaleAccel"]     = p.initScaleAccel;
        j["initScaleAccelRand"] = p.initScaleAccelRand;
        j["color"]              = ps_v4(p.color);
        j["lifetime"]           = p.lifetime;
        j["lifetimeRand"]       = p.lifetimeRand;
        j["maxTrailLength"]     = p.maxTrailLength;
        j["trailWidth"]         = p.trailWidth;
        j["ringInnerRadius"]    = p.ringInnerRadius;
        j["ringOuterRadius"]    = p.ringOuterRadius;
        j["ringSegments"]       = p.ringSegments;
        j["particleType"]       = static_cast<int>(p.particleType);
        return j;
    }

    inline EmitterSpawnProps SpawnPropsFromJson(const nlohmann::json& j)
    {
        EmitterSpawnProps p;
        auto gv3 = [&](const char* k, glm::vec3 def) -> glm::vec3
            { return j.contains(k) ? ps_fv3(j[k]) : def; };

        p.initPos           = gv3("initPos",        glm::vec3(0));
        p.initPosRand       = gv3("initPosRand",    glm::vec3(0));
        p.initVel           = gv3("initVel",        glm::vec3(0, 1, 0));
        p.initVelRand       = gv3("initVelRand",    glm::vec3(0.1f));
        p.initAccel         = gv3("initAccel",      glm::vec3(0));
        p.initAccelRand     = gv3("initAccelRand",  glm::vec3(0));
        p.initRot           = j.value("initRot",            0.0f);
        p.initRotRand       = j.value("initRotRand",        0.0f);
        p.initRotVel        = j.value("initRotVel",         0.0f);
        p.initRotVelRand    = j.value("initRotVelRand",     0.0f);
        p.initRotAccel      = j.value("initRotAccel",       0.0f);
        p.initRotAccelRand  = j.value("initRotAccelRand",   0.0f);
        p.initScale         = j.value("initScale",          0.2f);
        p.initScaleRand     = j.value("initScaleRand",      0.0f);
        p.initScaleVel      = j.value("initScaleVel",       0.0f);
        p.initScaleVelRand  = j.value("initScaleVelRand",   0.0f);
        p.initScaleAccel    = j.value("initScaleAccel",     0.0f);
        p.initScaleAccelRand= j.value("initScaleAccelRand", 0.0f);
        p.color             = j.contains("color") ? ps_fv4(j["color"]) : glm::vec4(1,0.5f,0.1f,1);
        p.lifetime          = j.value("lifetime",       2.0f);
        p.lifetimeRand      = j.value("lifetimeRand",   0.5f);
        p.maxTrailLength    = j.value("maxTrailLength", 20);
        p.trailWidth        = j.value("trailWidth",     0.1f);
        p.ringInnerRadius   = j.value("ringInnerRadius",0.2f);
        p.ringOuterRadius   = j.value("ringOuterRadius",0.5f);
        p.ringSegments      = j.value("ringSegments",   32);
        p.particleType      = static_cast<ParticleType>(j.value("particleType", 0));
        return p;
    }

    // ── Emitter tree serialization (self-recursive) ───────────────────────────
    inline nlohmann::json EmitterToJson(const EmitterBase* emitter)
    {
        nlohmann::json j;
        j["name"]          = emitter->m_name;
        j["type"]          = static_cast<int>(emitter->m_type);
        j["spawnCount"]    = emitter->m_spawnCount;
        j["maxParticles"]  = emitter->m_maxParticles;
        j["spawnInterval"] = emitter->m_spawnInterval;
        j["startFrame"]    = emitter->m_startFrame;
        j["endFrame"]      = emitter->m_endFrame;
        j["spawnProps"]    = SpawnPropsToJson(emitter->m_spawnProps);

        // Shape-specific fields
        if (emitter->m_type == EmitterType::Sphere)
        {
            const auto* s = static_cast<const SphereEmitter*>(emitter);
            j["radius"] = s->m_radius;
        }
        else if (emitter->m_type == EmitterType::Box)
        {
            const auto* b = static_cast<const BoxEmitter*>(emitter);
            j["halfExtents"] = ps_v3(b->m_halfExtents);
        }
        else if (emitter->m_type == EmitterType::Ring)
        {
            const auto* r = static_cast<const RingEmitter*>(emitter);
            j["radius"]    = r->m_radius;
            j["bandWidth"] = r->m_bandWidth;
        }

        // Recursively serialize child templates
        nlohmann::json children = nlohmann::json::array();
        for (const auto& child : emitter->m_children)
            children.push_back(EmitterToJson(child.get()));
        j["children"] = children;

        return j;
    }

    inline std::unique_ptr<EmitterBase> EmitterFromJson(const nlohmann::json& j)
    {
        EmitterType type = static_cast<EmitterType>(j.value("type", 0));
        std::unique_ptr<EmitterBase> emitter;

        switch (type)
        {
            case EmitterType::Sphere:
            {
                auto s = std::make_unique<SphereEmitter>();
                s->m_radius = j.value("radius", 1.0f);
                emitter = std::move(s);
                break;
            }
            case EmitterType::Box:
            {
                auto b = std::make_unique<BoxEmitter>();
                if (j.contains("halfExtents"))
                    b->m_halfExtents = ps_fv3(j["halfExtents"]);
                emitter = std::move(b);
                break;
            }
            case EmitterType::Ring:
            {
                auto r = std::make_unique<RingEmitter>();
                r->m_radius    = j.value("radius",    1.0f);
                r->m_bandWidth = j.value("bandWidth", 0.1f);
                emitter = std::move(r);
                break;
            }
            default:
                emitter = std::make_unique<PointEmitter>();
                break;
        }

        emitter->m_name          = j.value("name", std::string("Emitter"));
        emitter->m_spawnCount    = j.value("spawnCount",    5);
        emitter->m_maxParticles  = j.value("maxParticles",  100);
        emitter->m_spawnInterval = j.value("spawnInterval", 0.1f);
        emitter->m_startFrame    = j.value("startFrame",    0);
        emitter->m_endFrame      = j.value("endFrame",      120);
        if (j.contains("spawnProps"))
            emitter->m_spawnProps = SpawnPropsFromJson(j["spawnProps"]);

        // Recursively deserialize child templates
        if (j.contains("children"))
            for (const auto& cj : j["children"])
                emitter->m_children.push_back(EmitterFromJson(cj));

        return emitter;
    }

    // ── File I/O ──────────────────────────────────────────────────────────────
    inline bool SaveParticleEffect(const std::string& path,
                                   const std::vector<std::unique_ptr<EmitterBase>>& roots)
    {
        nlohmann::json j;
        j["emitters"] = nlohmann::json::array();
        for (const auto& e : roots)
            j["emitters"].push_back(EmitterToJson(e.get()));

        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(2);
        return f.good();
    }

    inline std::vector<std::unique_ptr<EmitterBase>> LoadParticleEffectFromFile(const std::string& path)
    {
        std::vector<std::unique_ptr<EmitterBase>> result;
        std::ifstream f(path);
        if (!f.is_open()) return result;

        nlohmann::json j;
        try { f >> j; }
        catch (...) { return result; }

        if (j.contains("emitters"))
            for (const auto& ej : j["emitters"])
                result.push_back(EmitterFromJson(ej));

        return result;
    }
}
