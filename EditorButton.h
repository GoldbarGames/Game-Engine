#pragma once

#include <SDL.h>
#include <string>

#include "Vector2.h"

class Game;
class Text;

class EditorButton
{
public:
	SDL_Texture * buttonTexture = nullptr;
	SDL_Rect buttonTextureRect;
	SDL_Rect buttonWindowRect;

	Text* text;

	Vector2 position = Vector2(0,0);
	std::string name = "";

	EditorButton(std::string txt, std::string filename, Vector2 pos, Game& game, Vector2 size = Vector2(0,0));
	~EditorButton();

	void Render(SDL_Renderer* renderer);

	bool IsClicked(const int& x, const int& y);
};

