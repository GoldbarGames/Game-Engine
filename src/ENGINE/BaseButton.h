#ifndef BASEBUTTON_H
#define BASEBUTTON_H
#pragma once
#include "leak_check.h"
#include <SDL2/SDL.h>
#include <string>
#include "globals.h"

#include "Text.h"

class Renderer;
class Game;

class KINJO_API BaseButton : public Entity
{
public:
	Text* text = nullptr;
	bool isSelected = false;
	bool pressedAnyKey = false;

	std::vector<Sprite*> images;

	int loopImages = 0;

	int btnID = -1;

	BaseButton* buttonPressedUp = nullptr;
	BaseButton* buttonPressedDown = nullptr;
	BaseButton* buttonPressedLeft = nullptr;
	BaseButton* buttonPressedRight = nullptr;
	BaseButton();

	virtual ~BaseButton();
	virtual void Render(const Renderer& renderer);
	virtual BaseButton* Update(Game& game, const Uint8* currentKeyStates);

	void SetButtonsUpDownLeftRight(BaseButton* up = nullptr, BaseButton* down = nullptr, BaseButton* left = nullptr, BaseButton* right = nullptr);
	virtual void SetOptionColors(Color c);
	void SetColor(Color c);
	Color GetImageColor(const unsigned int index);

	virtual void Highlight(Game& game);
	virtual void Unhighlight(Game& game);

	glm::vec3 AlignTextCenterY();
	glm::vec3 AlignTextTopY();
	glm::vec3 AlignTextBottomY();
};


#endif