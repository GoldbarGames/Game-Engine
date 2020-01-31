#pragma once
#include "Animator.h"
#include "Sprite.h"
#include "Text.h"


class Textbox
{
private:
	Vector2 position = Vector2(0, 0);
	Vector2 offset = Vector2(0, 0);
public:
	Animator* animator = nullptr;
	Sprite* boxSprite = nullptr;

	Sprite* leftSprite = nullptr;
	Sprite* centerSprite = nullptr;
	Sprite* rightSprite = nullptr;

	TTF_Font* textFont = nullptr;
	TTF_Font* speakerFont = nullptr;
	Text* text = nullptr;
	Text* speaker = nullptr;

	bool shouldRender = true;

	Color textColor = { 255, 255, 255, 255 };
	const Uint32 boxWidth = 1160;

	void UpdateText(std::string newText);
	void Render(Renderer * renderer);

	Textbox(SpriteManager * manager, Renderer * renderer);
	~Textbox();
};

