#pragma once
#include "Animator.h"
#include "Sprite.h"
#include "Text.h"

class Textbox
{
private:
	Vector2 position = Vector2(0, 0);
public:
	Animator* animator = nullptr;
	Sprite* boxSprite = nullptr;

	TTF_Font* textFont = nullptr;
	TTF_Font* speakerFont = nullptr;
	Text* text = nullptr;
	Text* speaker = nullptr;


	const Uint32 boxWidth = 575;

	void Render(Renderer * renderer);

	Textbox(SpriteManager * manager, Renderer * renderer);
	~Textbox();
};

