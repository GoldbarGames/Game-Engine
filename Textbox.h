#pragma once
#include "Animator.h"
#include "Sprite.h"
#include "Text.h"
#include <unordered_map>

class Textbox
{
private:
	Vector2 position = Vector2(0, 0);
	Vector2 offset = Vector2(0, 0);
public:
	Animator* animator = nullptr;
	Sprite* boxSprite = nullptr;

	TTF_Font* textFont = nullptr;
	TTF_Font* speakerFont = nullptr;
	Text* text = nullptr;
	Text* speaker = nullptr;

	bool shouldRender = true;
	bool isReading = false;
	const Uint32 boxWidth = 1140;

	void UpdateText(const std::string& newText, const Color& color);
	void Render(Renderer * renderer, const int& screenWidth, const int& screenHeight);

	Textbox(SpriteManager * manager, Renderer * renderer);
	~Textbox();
};

