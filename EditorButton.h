#include <SDL.h>
#include <string>

#include "Vector2.h"

class Game;

#pragma once
class EditorButton
{
public:
	SDL_Texture * buttonTexture = nullptr;
	SDL_Rect buttonTextureRect;
	SDL_Rect buttonWindowRect;

	Vector2 position = Vector2(0,0);
	std::string name = "";

	EditorButton(std::string filepath, std::string function, Vector2 pos, Game& game);
	~EditorButton();

	void Render(SDL_Renderer* renderer);

	bool IsClicked(const int& x, const int& y);
};

