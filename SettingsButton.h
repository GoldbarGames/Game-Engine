#ifndef SETTINGSBUTTON_H
#define SETTINGSBUTTON_H
#pragma once

#include <vector>
#include "Text.h"
#include "Vector2.h"
#include "BaseButton.h"

class Game;

class SettingsButton : public BaseButton
{
public:
	Vector2 position = Vector2(0,0);
	int selectedOption = 0;
	std::vector<Text*> options;
	Text* label = nullptr;

	SettingsButton(const std::string& n, const Vector2& pos, Game& game);
	~SettingsButton();
	void Render(const Renderer& renderer);
	BaseButton* Update(Game& game, const Uint8* currentKeyStates);
	void ExecuteSelectedOption(Game& game);
	void SetOptionColors(Color color);
};

#endif