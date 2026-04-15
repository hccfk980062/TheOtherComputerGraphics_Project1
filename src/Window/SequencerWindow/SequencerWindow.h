#pragma once

#include<ImGui.h>
#include<imgui_internal.h>

#include<ImNeoSequencer/imgui_neo_sequencer.h>
#include<ImNeoSequencer/imgui_neo_internal.h>

#include<ImGuiFileDialog/ImGuiFileDialog.h>

#include<glm/gtc/type_ptr.hpp>
#include<json.hpp>
using json = nlohmann::json;

#include "Scene/MainScene.h"

namespace CG
{
	struct AnimationTrack;


	struct KeyframeData
	{
		int frame;
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		AnimationTrack* sourceTrack;
	};
	struct AnimationTrack
	{
		std::string trackName;
		std::vector<KeyframeData> keyframes;
		SceneObject* linkedObject = nullptr;

		bool open = true;

		AnimationTrack() = default;

		void SortKeyframeDatas()
		{
			std::sort(keyframes.begin(), keyframes.end(), [](const KeyframeData& a, const KeyframeData& b)
				{
					return a.frame < b.frame;
				});
		}

		//:                                                   previousFrameData     nextFrameData
		//> frame 剛好在某關鍵幀上:      前一關鍵幀(或自身)       該關鍵幀
		//> frame 在所有關鍵幀之前:      第一個關鍵幀(夾緊)       第一個關鍵幀
		//> frame 在所有關鍵幀之後:      最後一個關鍵幀              最後一個關鍵幀(夾緊)
		//> 只有一個關鍵幀:                      該關鍵幀                          該關鍵幀
		void GetClampedKeyframes(int frame, KeyframeData &previousFrameData, KeyframeData &nextFrameData)
		{
			if (keyframes.empty()) return;

			// 找第一個 frame >= 目標幀 的迭代器
			auto nextIt = std::lower_bound(keyframes.begin(), keyframes.end(), frame, [](const KeyframeData& kf, int f) 
			{
				return kf.frame < f;
			});

			// --- 決定 nextFrameData ---
			if (nextIt == keyframes.end())
				nextIt = std::prev(keyframes.end()); // 超過最後一幀，夾緊到末端

			// --- 決定 previousFrameData ---
			auto prevIt = (nextIt == keyframes.begin())
				? keyframes.begin()              // 在第一幀之前，夾緊到開頭
				: std::prev(nextIt);             // 正常往前一格

			previousFrameData = *prevIt;
			nextFrameData = *nextIt;
		}
		
		AnimationTrack(SceneObject *obj)
		{
			trackName = obj->animationSerializedName;
			linkedObject = obj;
		}

	};
	struct AnimationGroup
	{
		std::string groupName;
		std::vector<AnimationTrack>tracks;

		AnimationGroup() = default;

		AnimationGroup(SceneObject* rootObject)
		{
			groupName = rootObject->animationGroupName;
			for (auto& subObj : rootObject->GetChilerenObjects())
			{
				tracks.push_back(AnimationTrack(subObj));
			}
		}
	};

	// ── KeyframeData ──────────────────────────────────────────
	inline void to_json(json& j, const KeyframeData& kf)
	{
		j = json{
			{"frame",    kf.frame},
			{"position", {kf.position.x, kf.position.y, kf.position.z}},
			{"rotation", {kf.rotation.w, kf.rotation.x, kf.rotation.y, kf.rotation.z}},
			{"scale",    {kf.scale.x,    kf.scale.y,    kf.scale.z}}
		};
	}
	inline void from_json(const json& j, KeyframeData& kf)
	{
		j.at("frame").get_to(kf.frame);

		auto pos = j.at("position");
		kf.position = { pos[0], pos[1], pos[2] };

		auto rot = j.at("rotation");
		kf.rotation = { rot[0], rot[1], rot[2], rot[3] }; // w, x, y, z

		auto scl = j.at("scale");
		kf.scale = { scl[0], scl[1], scl[2] };
	}

	// ── AnimationTrack ────────────────────────────────────────
	inline void to_json(json& j, const AnimationTrack& track)
	{
		j = json{
			{"trackName", track.trackName},
			{"keyframes", track.keyframes}   // 自動遞迴呼叫上面的 to_json
		};
	}
	inline void from_json(const json& j, AnimationTrack& track)
	{
		j.at("trackName").get_to(track.trackName);
		j.at("keyframes").get_to(track.keyframes); // 自動遞迴呼叫上面的 from_json
	}

	// ── AnimationGroup ────────────────────────────────────────
	inline void to_json(json& j, const AnimationGroup& track)
	{
		j = json{
			{"groupName", track.groupName},
			{"tracks", track.tracks}   // 自動遞迴呼叫上面的 to_json
		};
	}
	inline void from_json(const json& j, AnimationGroup& track)
	{
		j.at("groupName").get_to(track.groupName);
		j.at("tracks").get_to(track.tracks); // 自動遞迴呼叫上面的 from_json
	}

	class SequencerWindow
	{
	public:
		SequencerWindow();
		~SequencerWindow();

		auto Initialize() -> bool;
		void Display();

		void SetTargetScene(MainScene* scene)
		{
			targetScene = scene;

			for (auto& object : targetScene->rootObject.children)
			{
				CreateAnimationGroup(object.get());
			}
		}
		void CreateAnimationGroup(SceneObject* obj)
		{
			animationGroups.push_back(AnimationGroup(obj));
		}


	private:
		MainScene* targetScene = nullptr;

		std::vector<AnimationGroup>animationGroups;

		int currentFrame = 0;
		int startFrame = 0;
		int endFrame = 120;
		bool isPlaying = false;

		double lastTime = 0;
		double timeAccumulated = 0;
		int selectedGroupIndex = -1;

		bool transformTabOpen[8] = {true};

		KeyframeData* selectedKeyframe = nullptr;
		AnimationTrack* selectedAnimationTrack = nullptr;

		void TransformToFrame()
		{
			for(auto& group : animationGroups)
			{
				for (auto& track : group.tracks)
				{
					if (track.keyframes.size() == 0) continue;

					KeyframeData previousFrameData;
					KeyframeData nextFrameData;

					track.GetClampedKeyframes(currentFrame, previousFrameData, nextFrameData);

					float t = 0.0f;
					int range = nextFrameData.frame - previousFrameData.frame;
					if (range > 0)
						t = float(currentFrame - previousFrameData.frame) / float(range);

					t = glm::clamp(t, 0.0f, 1.0f);

					glm::vec3 pos = glm::mix(previousFrameData.position, nextFrameData.position, t);
					glm::quat rot = glm::slerp(previousFrameData.rotation, nextFrameData.rotation, t);
					glm::vec3 scl = glm::mix(previousFrameData.scale, nextFrameData.scale, t);

					track.linkedObject->SetPosition(pos);
					track.linkedObject->SetRotation(rot);
					track.linkedObject->SetScale(scl);
				}
			}
		}

		void ExportAllToJson(const std::string& filepath);
		void ImportAllFromJson(const std::string& filepath);

		// ── 單一 Group 的匯出/匯入 ────────────────────────────
		void ExportSpecificGroupToJson(int groupIndex, const std::string& filepath);
		void ImportSpecificGroupFromJson(int groupIndex, const std::string& filepath);
	};
}