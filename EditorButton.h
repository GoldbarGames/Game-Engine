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
	Texture * buttonTexture = nullptr;
	SDL_Rect buttonTextureRect;
	SDL_Rect buttonWindowRect;

	Text* text;
	Color buttonColor;

	// TODO: Make an unordered map of colors for this too?
	Color colorOn;
	Color colorOff;

	Vector2 position = Vector2(0,0);
	std::string name = "";

	EditorButton(std::string txt, std::string filename, Vector2 pos, Game& game, Vector2 size = Vector2(0, 0), Color color = { 255, 255, 255, 255 });
	~EditorButton();

	void SetColors(Color c1, Color c2);

	void Render(Renderer* renderer, GLuint uniformModel);

	bool IsClicked(const int& x, const int& y);
};

