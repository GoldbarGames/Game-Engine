#ifndef DIALOG_H
#define DIALOG_H
#pragma once

#include "Text.h"

class Dialog
{
public:
	bool visible = false;
	Vector2 position = Vector2(0, 0);
	Sprite* sprite = nullptr;
	Text* text = nullptr;
	Text* input = nullptr;

	Dialog(const Vector2& pos, SpriteManager* manager);
	~Dialog();

	void Update();
	void Render(Renderer* renderer);
};

#endif