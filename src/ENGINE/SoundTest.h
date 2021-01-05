#ifndef SOUNDTEST_H
#define SOUNDTEST_H
#pragma once

#include "Game.h"
#include "Renderer.h"
#include "EditorButton.h"
#include "Timer.h"

class SoundManager;

class KINJO_API SoundTest
{
public:
	SoundManager* manager = nullptr;

	bool isPaused = false;
	EditorButton* loadBGMButton = nullptr;
	EditorButton* playButton = nullptr;
	EditorButton* stepForwardButton = nullptr;
	EditorButton* stepBackButton = nullptr;
	EditorButton* setTimeButton = nullptr;

	Timer timer;

	std::vector<EditorButton*> buttons;

	SoundTest(SoundManager& m);
	~SoundTest();

	void Update(Game& game);
	void Render(const Renderer& renderer);
};

#endif