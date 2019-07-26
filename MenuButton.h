#pragma once

#include <SDL.h>
#include <string>
#include "Sprite.h"

class Game;

class MenuButton
{
private:
	int thickness = 4;
	SDL_Rect outlineHorizontal;
	SDL_Rect outlineVertical;
	SDL_Rect outlineCorners;
	Sprite* image = nullptr;
	std::string text = "";
	Vector2 position = Vector2(0, 0);
public:
	MenuButton(std::string t, std::string filepath, Vector2 pos, Game& game);
	~MenuButton();
	void Render(SDL_Renderer* renderer);
};