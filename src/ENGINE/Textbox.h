#ifndef TEXTBOX_H
#define TEXTBOX_H
#pragma once

#include "Animator.h"
#include "Sprite.h"
#include "Text.h"
#include <unordered_map>
#include "FontInfo.h"
#include "leak_check.h"

class KINJO_API Textbox
{
private:
	//Vector2 position = Vector2(0, 0);
	//Vector2 offset = Vector2(0, 0);
public:
	Animator* animator = nullptr;
	Entity* boxObject = nullptr;
	Entity* nameObject = nullptr;
	Entity* clickToContinue = nullptr;

	FontInfo* fontInfoText = nullptr;
	FontInfo* fontInfoSpeaker = nullptr;
	Text* text = nullptr;
	Text* speaker = nullptr;

	bool shouldRender = true;
	bool isReading = false;

	uint32_t boxWidth = 1100;

	std::string fullTextString = "";
	SpriteManager* spriteManager = nullptr;
	Renderer* renderer = nullptr;

	void ChangeNameFont(const std::string& fontName, const int size);
	void ChangeNameSprite(const std::string& filepath);

	void ChangeBoxFont(const std::string& fontName, const int size);
	void ChangeBoxSprite(const std::string& filepath);

	void UpdateText(const char c, const Color& color);
	void UpdateText(const std::string& newText, const Color& color);
	void Render(const Renderer& renderer, const int& screenWidth, const int& screenHeight);

	void SetCursorPosition(bool endOfPage);
	void SetCursorPosition(bool endOfPage, Vector2 newCursorPos);

	void SetFontSize(int newSize);

	Textbox(SpriteManager& m, Renderer& r);
	~Textbox();
};

#endif