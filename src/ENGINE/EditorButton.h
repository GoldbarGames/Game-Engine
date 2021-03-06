#ifndef EDITORBUTTON_H
#define EDITORBUTTON_H
#pragma once

#include <SDL2/SDL.h>
#include <string>

#include "Vector2.h"
#include "globals.h"
#include "Texture.h"
#include "leak_check.h"
#include "Text.h"

class Game;
class Text;
class Renderer;

class KINJO_API EditorButton
{
public:
	SDL_Rect buttonTextureRect;
	SDL_Rect buttonWindowRect;

	Sprite* image = nullptr;
	Text* text= nullptr;

	glm::vec3 position = glm::vec3(0,0,0);
	Vector2 imageScale = Vector2(1, 1);
	std::string name = "";

	Color color = { 255, 255, 255, 255 };

	bool isClicked = false;
	bool isHovered = false;

	EditorButton(std::string txt, std::string filename, glm::vec3 pos,
		Game& game, Vector2 size = Vector2(0, 0), Color color = { 255, 255, 255, 255 });

	~EditorButton();

	void Render(const Renderer& renderer);

	bool IsPointInsideButton(const int& x, const int& y);
};

#endif