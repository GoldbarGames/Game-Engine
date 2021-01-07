#ifndef DIALOG_H
#define DIALOG_H
#pragma once

#include "Text.h"
#include "leak_check.h"
class KINJO_API Dialog
{
public:
	bool visible = false;
	glm::vec3 position = glm::vec3(0, 0, 0);
	Vector2 scale = Vector2(1, 1);
	Sprite* sprite = nullptr;
	Text* text = nullptr;
	Text* input = nullptr;

	Dialog(SpriteManager* manager);
	Dialog(const glm::vec3& pos, SpriteManager* manager);
	~Dialog();

	void Update(const std::string& newText);
	void Render(const Renderer& renderer);
};

#endif