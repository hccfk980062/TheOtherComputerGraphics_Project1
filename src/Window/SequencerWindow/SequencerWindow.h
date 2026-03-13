#pragma once

#include<ImGui.h>
#include<imgui_internal.h>

#include<ImNeoSequencer/imgui_neo_sequencer.h>
#include<ImNeoSequencer/imgui_neo_internal.h>

#include "Scene/MainScene.h"

namespace CG
{
	struct KeyframeData
	{
		int frame;
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
	};
	struct AnimationTrack
	{
		std::string trackName;
		std::vector<KeyframeData> keyframes; //可以不按frame順序排列
		SceneObject* linkedObject = nullptr;

		bool open = true;


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
		
	};


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
			animationTracks[0].linkedObject = scene->rootObject.children[0].get();
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


		bool transformOpen = true;
	};
}