#ifndef SETTINGSBUTTON_H
#define SETTINGSBUTTON_H
#pragma once

#include <vector>
#include "Text.h"
#include <glm/vec2.hpp>
#include "BaseButton.h"
#include "leak_check.h"

class Game;

enum class SettingsType { NORMAL, KEYMAP, BUTTONMAP };

class KINJO_API SettingsButton : public BaseButton
{
public:
	int selectedOption = 0;
	std::vector<Text*> options;
	Text* label = nullptr;

	SettingsType settingsType;

	SettingsButton(const std::string& n, const glm::vec2& pos, Game& game, SettingsType st = SettingsType::NORMAL);
	~SettingsButton();
	void Render(const Renderer& renderer);
	BaseButton* Update(Game& game, const Uint8* currentKeyStates);
	void ExecuteSelectedOption(Game& game);
	void SetOptionColors(Color c);
};

#endif