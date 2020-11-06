#ifndef DIALOG_H
#define DIALOG_H
#pragma once

#include "Text.h"
#include "leak_check.h"
class DECLSPEC Dialog
{
public:
	bool visible = false;
	Vector2 position = Vector2(0, 0);
	Vector2 scale = Vector2(1, 1);
	Sprite* sprite = nullptr;
	Text* text = nullptr;
	Text* input = nullptr;

	Dialog(const Vector2& pos, SpriteManager* manager);
	~Dialog();

	void Update();
	void Render(const Renderer& renderer);
};

#endif