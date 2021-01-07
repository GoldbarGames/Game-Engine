#ifndef SOUNDTEST_H
#define SOUNDTEST_H
#pragma once

#include "Game.h"
#include "Renderer.h"
#include "EditorButton.h"
#include "Text.h"
#include "Dialog.h"

class SoundManager;

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

	Sprite timelineRectangle;
	Sprite timelineLocation;

	Text songText;

	Text timerText;
	Dialog dialog;

	std::string currentDir = "";
	std::string currentBGM = "";
	float songTimer = 0.0f;

	std::vector<EditorButton*> buttons;

	SoundTest(SoundManager& m);
	~SoundTest();

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void CreateDialog(const std::string& txt);
	void AfterDirDialog(const std::string& dir);
	void AfterFileDialog(const std::string& bgm);
	void AfterJumpDialog(const std::string& time);

	void MusicFinished();
	void UpdateTimerText();
};

#endif