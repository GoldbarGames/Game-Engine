#ifndef SOUNDTEST_H
#define SOUNDTEST_H
#pragma once

#include "Game.h"
#include "Renderer.h"
#include "EditorButton.h"
#include "Text.h"
#include "Dialog.h"

class SoundManager;


struct SoundLoop
{
	std::string name = "";

	uint32_t startTime = 0;
	uint32_t endTime = 0;
	Color color = { 255, 255, 255, 255 };

	Text* text = nullptr;

	EditorButton* modifyButton = nullptr;
	EditorButton* removeButton = nullptr;
	EditorButton* selectButton = nullptr;
	EditorButton* jumpButton = nullptr;

	SoundLoop(std::string n)
	{
		name = n;
	}

	~SoundLoop()
	{
		if (text != nullptr)
			delete_it(text);

		if (modifyButton != nullptr)
			delete_it(modifyButton);

		if (removeButton != nullptr)
			delete_it(removeButton);

		if (selectButton != nullptr)
			delete_it(selectButton);

		if (jumpButton != nullptr)
			delete_it(jumpButton);
	}
};

class KINJO_API SoundTest
{
public:
	SoundManager* manager = nullptr;

	bool isPaused = true;
	EditorButton* folderDirButton = nullptr;
	EditorButton* loadBGMButton = nullptr;
	EditorButton* playButton = nullptr;
	EditorButton* stepForwardButton = nullptr;
	EditorButton* stepBackButton = nullptr;
	EditorButton* setTimeButton = nullptr;
	EditorButton* addLoopButton = nullptr;

	EditorButton* bgmUpButton = nullptr;
	EditorButton* bgmDownButton = nullptr;

	Sprite timelineRectangle;
	Sprite timelineLocation;

	std::unordered_map<std::string, std::vector<SoundLoop*>> soundLoops;

	Text songText;

	Text timerText;
	Dialog dialog;

	std::string currentDir = "";
	std::string currentBGM = "";
	float songTimer = 0.0f;
	int currentlyModifyingLoop = -1;
	int selectedLoop = -1;

	std::vector<EditorButton*> buttons;

	SoundTest(SoundManager& m);
	~SoundTest();

	void Update(Game& game);
	void UpdateSoundMode(Game& game);
	void Render(const Renderer& renderer);

	void SetSelectedLoopFromName(const std::string& bgmName, const std::string& loopName);

	void CreateDialog(const std::string& txt);
	void AfterDirDialog(const std::string& dir);
	void AfterFileDialog(const std::string& bgm);
	void AfterJumpDialog(const std::string& time);

	void AfterLoopDialog1(const std::string& time);
	void AfterLoopDialog2(const std::string& time);
	void AfterLoopDialog3(const std::string& color);

	void ScrollCurrentBGM(bool up);

	void SaveData();
	void CreateLoop(const std::string& name, uint32_t st, uint32_t et, Color c);

	float CalcTimelinePosition(float time, float a, float b, float w);

	void MusicFinished();
	void UpdateTimerText();
	std::string ConvertTimeToStringFromNumber(uint32_t time);
};

#endif