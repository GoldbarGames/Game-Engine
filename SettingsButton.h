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

	SettingsButton(std::string n, Vector2 pos, Game& game);
	~SettingsButton();
	void Render(Renderer* renderer);
	SettingsButton* buttonPressedUp = nullptr;
	SettingsButton* buttonPressedDown = nullptr;
	std::vector<Text*> options;

	BaseButton* Update(const Uint8* currentKeyStates);
};

