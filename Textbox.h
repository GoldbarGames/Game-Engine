#pragma once
#include "Animator.h"
#include "Sprite.h"
#include "Text.h"
#include <unordered_map>
#include "FontInfo.h"

class Textbox
{
private:
	//Vector2 position = Vector2(0, 0);
	//Vector2 offset = Vector2(0, 0);
public:
	Animator* animator = nullptr;
	Entity* boxObject = nullptr;
	Entity* nameObject = nullptr;
	Entity* clickToContinue = nullptr;

	//TODO: Minimize string allocations, use a map of ints to strings
	std::unordered_map<std::string, FontInfo*> fonts;

	FontInfo* currentFontInfo = nullptr;
	TTF_Font* textFont = nullptr;
	TTF_Font* speakerFont = nullptr;
	Text* text = nullptr;
	Text* speaker = nullptr;

	bool shouldRender = true;
	bool isReading = false;

	const Uint32 boxWidth = 1140;

	SpriteManager* spriteManager = nullptr;
	Renderer* renderer = nullptr;

	void ChangeNameFont(const std::string& fontName);
	void ChangeNameSprite(const std::string& filepath);

	void ChangeBoxFont(const std::string& fontName);
	void ChangeBoxSprite(const std::string& filepath);

	void UpdateText(const char c, const Color& color);
	void UpdateText(const std::string& newText, const Color& color);
	void Render(Renderer* renderer, const int& screenWidth, const int& screenHeight);

	void SetCursorPosition(bool endOfPage);
	void SetFontSize(int newSize);

	Textbox(SpriteManager* m, Renderer* r);
	~Textbox();
};

