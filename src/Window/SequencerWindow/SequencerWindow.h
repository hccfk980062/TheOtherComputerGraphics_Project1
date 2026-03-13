#pragma once
#pragma once
#pragma once

#include <ImSequencer.h>

#include "Scene/MainScene.h"

namespace CG
{
    struct AnimationSequencer : public ImSequencer::SequenceInterface
    {
        // --- Required state ---
        int mFrameMin = 0;
        int mFrameMax = 120;
        struct KeyframeData
        {
            int startFrame;
            int endFrame;
            glm::vec3 startPosition;
            glm::quat startRotation;
            glm::vec3 startScale;
            glm::vec3 endPosition;
            glm::quat endRotation;
            glm::vec3 endScale;
        };
        struct KeyframeTrack
        {
            std::string label;
            unsigned int color;                          // RGBA packed

            SceneObject* LinkedObject;
            std::vector<KeyframeData> keyframes; // {frameStart, frameEnd}
        };

        std::vector<KeyframeTrack> tracks;

        // --- Interface overrides ---

        int GetFrameMin() const override { return mFrameMin; }
        int GetFrameMax() const override { return mFrameMax; }
        int GetItemCount() const override { return (int)tracks.size(); }

        int GetItemTypeCount() const override { return 1; }
        const char* GetItemTypeName(int i) const override { return "Keyframe"; }
        const char* GetItemLabel(int i) const override { return tracks[i].label.c_str(); }

        void Get(int index, int** start, int** end, int* type, unsigned int* color) override 
        {
            auto& t = tracks[index];
            // ImSequencer calls Get() for each keyframe segment — you need
            // to track which segment is being queried via an internal iterator
            // or index offset. A common pattern uses a flat list:
            if (color) *color = t.color;
            if (type)  *type = 0;
            // start/end point into your data
            if (start) *start = &t.keyframes[0].startFrame;
            if (end)   *end = &t.keyframes[0].endFrame;
        }

        void Add(int type) override
        {
            KeyframeTrack tra = { "Track " + std::to_string(tracks.size()), 0xFF3080FF };
            tra.keyframes.push_back(KeyframeData{0, 60});
            tracks.push_back(tra);
        }

        void Del(int index) override
        {
            tracks.erase(tracks.begin() + index);
        }

        void Duplicate(int index) override
        {
            tracks.push_back(tracks[index]);
        }

        size_t GetCustomHeight(int index) override { return 0; }

        void DoubleClick(int index) override
        {
            // Optional: open a property panel for this track
        }

        void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect,
            const ImRect& clippingRect, const ImRect& legendClippingRect) override
        {
        }

        void CustomDrawCompact(int index, ImDrawList* draw_list,
            const ImRect& rc, const ImRect& clippingRect) override
        {
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
            animationSequencer.tracks[0].LinkedObject = scene->rootObject.children[0].get();


            animationSequencer.tracks[0].keyframes[0].startPosition = glm::vec3(0.0f, 0.0f, 0.0f);
            animationSequencer.tracks[0].keyframes[0].endPosition = glm::vec3(60.0f, 0.0f, 0.0f);

            animationSequencer.tracks[0].keyframes[0].startRotation = glm::quat(glm::vec3(0.0f));
            animationSequencer.tracks[0].keyframes[0].endRotation = glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
		}

	private:
		MainScene* targetScene = nullptr;

        AnimationSequencer animationSequencer;
        int currentFrame = 0;
        bool expanded = true;
        int selectedEntry = -1;
        int firstFrame = 0;

        bool playing = false;
        double lastTime = 0.0;
        double timeAccumulated = 0;
	};
}