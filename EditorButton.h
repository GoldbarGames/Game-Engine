#pragma once

#include <SDL.h>
#include <string>

#include "Vector2.h"
#include "globals.h"
#include "Texture.h"

#include "Text.h"

class Game;
class Text;
class Renderer;

class EditorButton
{
public:
	SDL_Rect buttonTextureRect;
	SDL_Rect buttonWindowRect;

	Sprite* image = nullptr;
	Text* text= nullptr;
	Color buttonColor;

	Vector2 position = Vector2(0,0);
	std::string name = "";

	bool isClicked = false;
	bool isHovered = false;

	EditorButton(std::string txt, std::string filename, Vector2 pos, 
		Game& game, Vector2 size = Vector2(0, 0), Color color = { 255, 255, 255, 255 });

	~EditorButton();

	void Render(Renderer* renderer);

	bool IsPointInsideButton(const int& x, const int& y);
};

