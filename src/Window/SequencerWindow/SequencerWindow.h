#pragma once

#include<ImGui.h>
#include<imgui_internal.h>

#include<ImNeoSequencer/imgui_neo_sequencer.h>
#include<ImNeoSequencer/imgui_neo_internal.h>

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
			trackName = obj->name;
			linkedObject = obj;
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

			for (int i = 0; i < targetScene->ObjectList.size(); i++)
			{
				CreateAnimationTrack(targetScene->ObjectList[i]);
			}

			//animationTracks[0].keyframes.push_back(KeyframeData{ 0, glm::vec3(0.0f), glm::quat(glm::radians(glm::vec3(0.0f,0.0f,0.0f))), glm::vec3(1.0f) });
			//animationTracks[0].keyframes.push_back(KeyframeData{ 30, glm::vec3(10.0f), glm::quat(glm::radians(glm::vec3(90.0f,0.0f,0.0f))), glm::vec3(1.0f) });
			//animationTracks[0].keyframes.push_back(KeyframeData{ 60, glm::vec3(00.0f), glm::quat(glm::radians(glm::vec3(180.0f,0.0f,0.0f))), glm::vec3(1.0f) });
			//animationTracks[0].keyframes.push_back(KeyframeData{ 90, glm::vec3(-10.0f), glm::quat(glm::radians(glm::vec3(270.0f,0.0f,0.0f))), glm::vec3(1.0f) });
			//animationTracks[0].keyframes.push_back(KeyframeData{ 120, glm::vec3(0.0f), glm::quat(glm::radians(glm::vec3(360.0f,0.0f,0.0f))), glm::vec3(1.0f) });
		}
		void CreateAnimationTrack(SceneObject* obj)
		{
			animationTracks.push_back(AnimationTrack(obj));
		}


	private:
		MainScene* targetScene = nullptr;
		std::vector<AnimationTrack> animationTracks; //目前應該只有一個 (控制某個物件的變換用的Track)

		int currentFrame = 0;
		int startFrame = 0;
		int endFrame = 120;
		bool isPlaying = false;

		double lastTime = 0;
		double timeAccumulated = 0;


		bool transformTabOpen = true;

		KeyframeData* selectedKeyframe = nullptr;
		AnimationTrack* selectedAnimationTrack = nullptr;

		void TransformToFrame()
		{
			for (auto& track : animationTracks)
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

		void ExportToJson(const std::string& filepath)
		{
			json j = animationTracks;   // 自動呼叫 to_json

			std::ofstream file(filepath);
			if (!file.is_open())
			{
				std::cerr << "[Sequencer] 無法開啟檔案: " << filepath << "\n";
				return;
			}

			file << j.dump(4);          // 縮排 4 格，方便人工閱讀
			std::cout << "[Sequencer] 已匯出至: " << filepath << "\n";
		}
		void ImportFromJson(const std::string& filepath)
		{
			std::ifstream file(filepath);
			if (!file.is_open())
			{
				std::cerr << "[Sequencer] 找不到檔案: " << filepath << "\n";
				return;
			}

			try
			{
				json j = json::parse(file);
				animationTracks = j.get<std::vector<AnimationTrack>>(); // 自動呼叫 from_json
				std::cout << "[Sequencer] 已載入 " << animationTracks.size() << " 條軌道\n";

				for (auto& track : animationTracks)
				{
					for (auto& keyframe : track.keyframes)
					{
						keyframe.sourceTrack = &track;
					}

					for (auto* obj : targetScene->ObjectList)
					{
						if (obj->name == track.trackName)
						{
							track.linkedObject = obj;
							break;
						}
					}
				}
			}
			catch (const json::exception& e)
			{
				std::cerr << "[Sequencer] JSON 解析錯誤: " << e.what() << "\n";
			}
		}
	};
}