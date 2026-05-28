#pragma once
#include <string>
#include <fstream>
#include <memory>

#include <json.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Scene/MainScene.h"
#include "ParticleEffects/ParticleSerializer.h"

namespace CG
{
    // ── Transform helpers ─────────────────────────────────────────────────────
    inline nlohmann::json ss_xform_to_json(const Transform& t)
    {
        return {
            {"position", {t.position.x, t.position.y, t.position.z}},
            {"rotation", {t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w}},
            {"scale",    {t.scale.x,    t.scale.y,    t.scale.z}}
        };
    }

    inline Transform ss_xform_from_json(const nlohmann::json& j)
    {
        Transform t;
        if (j.contains("position"))
        {
            auto& p = j["position"];
            t.position = { p[0].get<float>(), p[1].get<float>(), p[2].get<float>() };
        }
        if (j.contains("rotation"))
        {
            auto& r = j["rotation"];
            // JSON stores [x, y, z, w]; glm::quat constructor is (w, x, y, z)
            t.rotation = glm::quat(r[3].get<float>(), r[0].get<float>(), r[1].get<float>(), r[2].get<float>());
        }
        if (j.contains("scale"))
        {
            auto& s = j["scale"];
            t.scale = { s[0].get<float>(), s[1].get<float>(), s[2].get<float>() };
        }
        return t;
    }

    // ── Save ─────────────────────────────────────────────────────────────────
    inline bool SaveScene(const std::string& path, MainScene* scene)
    {
        nlohmann::json j;
        j["version"] = 1;

        // Camera
        {
            auto& cam = scene->freeViewCamera;
            j["camera"] = {
                {"position", {cam.Position.x, cam.Position.y, cam.Position.z}},
                {"yaw",      cam.Yaw},
                {"pitch",    cam.Pitch}
            };
        }

        // Light
        {
            auto& l = scene->light;
            j["light"] = {
                {"type",       static_cast<int>(l.type)},
                {"position",   {l.position.x,  l.position.y,  l.position.z}},
                {"direction",  {l.direction.x, l.direction.y, l.direction.z}},
                {"color",      {l.color.x,     l.color.y,     l.color.z}},
                {"intensity",  l.intensity},
                {"shadowBias", l.shadowBias}
            };
        }

        // Scene objects (non-emitter)
        j["sceneObjects"] = nlohmann::json::array();
        for (SceneObject* obj : scene->ObjectList)
        {
            if (obj->objectType == 2) continue;
            j["sceneObjects"].push_back({
                {"name",      obj->objectName},
                {"parent",    obj->parent ? obj->parent->objectName : ""},
                {"transform", ss_xform_to_json(obj->transform)}
            });
        }

        // Particle emitters (serialized inline)
        j["particleEmitters"] = nlohmann::json::array();
        for (SceneObject* obj : scene->ObjectList)
        {
            if (obj->objectType != 2 || !obj->emitter) continue;
            j["particleEmitters"].push_back({
                {"name",        obj->objectName},
                {"parent",      obj->parent ? obj->parent->objectName : ""},
                {"transform",   ss_xform_to_json(obj->transform)},
                {"emitterData", EmitterToJson(obj->emitter)}
            });
        }

        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(2);
        return f.good();
    }

    // ── Load ─────────────────────────────────────────────────────────────────
    inline bool LoadScene(const std::string& path, MainScene* scene)
    {
        std::ifstream f(path);
        if (!f.is_open()) return false;

        nlohmann::json j;
        try { f >> j; }
        catch (...) { return false; }

        // Full reset then rebuild base scene (models + default hierarchy)
        scene->Reset();
        scene->Initialize();

        // Camera
        if (j.contains("camera"))
        {
            auto& jc = j["camera"];
            if (jc.contains("position"))
            {
                auto& p = jc["position"];
                scene->freeViewCamera.Position = { p[0].get<float>(), p[1].get<float>(), p[2].get<float>() };
            }
            scene->freeViewCamera.Yaw   = jc.value("yaw",   scene->freeViewCamera.Yaw);
            scene->freeViewCamera.Pitch = jc.value("pitch", scene->freeViewCamera.Pitch);
            scene->freeViewCamera.updateCameraVectors();
        }

        // Light
        if (j.contains("light"))
        {
            auto& jl = j["light"];
            scene->light.type = static_cast<LightData::Type>(jl.value("type", 1));
            if (jl.contains("position"))
            {
                auto& p = jl["position"];
                scene->light.position = { p[0].get<float>(), p[1].get<float>(), p[2].get<float>() };
            }
            if (jl.contains("direction"))
            {
                auto& d = jl["direction"];
                scene->light.direction = { d[0].get<float>(), d[1].get<float>(), d[2].get<float>() };
            }
            if (jl.contains("color"))
            {
                auto& c = jl["color"];
                scene->light.color = { c[0].get<float>(), c[1].get<float>(), c[2].get<float>() };
            }
            scene->light.intensity  = jl.value("intensity",  1.0f);
            scene->light.shadowBias = jl.value("shadowBias", 0.005f);
        }

        // Scene objects: fix hierarchy first, then apply transforms
        if (j.contains("sceneObjects"))
        {
            // Pass 1: re-establish parent relationships that differ from the default
            for (auto& jo : j["sceneObjects"])
            {
                std::string name       = jo.value("name",   "");
                std::string parentName = jo.value("parent", "");
                SceneObject* obj = scene->FindObjectByName(name);
                if (!obj) continue;

                std::string currentParentName = obj->parent ? obj->parent->objectName : "";
                if (currentParentName == parentName) continue;

                SceneObject* newParent = parentName.empty()
                    ? &scene->rootObject
                    : scene->FindObjectByName(parentName);
                scene->ReparentObjectDirect(obj, newParent ? newParent : &scene->rootObject);
            }

            // Pass 2: apply local transforms
            for (auto& jo : j["sceneObjects"])
            {
                SceneObject* obj = scene->FindObjectByName(jo.value("name", ""));
                if (!obj || !jo.contains("transform")) continue;
                obj->transform = ss_xform_from_json(jo["transform"]);
                obj->MarkDirty();
            }
        }

        // Recreate particle emitters from saved inline data
        if (j.contains("particleEmitters"))
        {
            for (auto& jo : j["particleEmitters"])
            {
                if (!jo.contains("emitterData")) continue;
                auto emitter = EmitterFromJson(jo["emitterData"]);
                if (!emitter) continue;

                auto objPtr = std::make_unique<SceneObject>();
                objPtr->id                      = scene->objectCount++;
                objPtr->objectName              = jo.value("name", emitter->m_name);
                objPtr->animationGroupName      = objPtr->objectName;
                objPtr->animationSerializedName = objPtr->objectName;
                objPtr->objectType              = 2;
                objPtr->emitter                 = emitter.get();
                emitter->ownerNode              = objPtr.get();

                if (jo.contains("transform"))
                {
                    objPtr->transform = ss_xform_from_json(jo["transform"]);
                    objPtr->MarkDirty();
                }

                std::string parentName = jo.value("parent", "");
                SceneObject* parent = parentName.empty()
                    ? &scene->rootObject
                    : scene->FindObjectByName(parentName);
                if (!parent) parent = &scene->rootObject;

                objPtr->parent = parent;
                SceneObject* rawPtr = objPtr.get();
                parent->children.push_back(std::move(objPtr));
                scene->m_emitterObjects.push_back(rawPtr);
                scene->ObjectList.push_back(rawPtr);
                scene->m_particleEmitters.push_back(std::move(emitter));
            }
        }

        return true;
    }
}
