#pragma once
#include "SDL.h"
#include <SDL_ttf.h>
#include <string>
#include "globals.h"
#include "GL/glew.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Texture.h"
#include "Vector2.h"

using std::string;

class Renderer;
class Sprite;

class Text
{
private:
	Renderer* renderer;
	TTF_Font* font;
public:
	std::string id = ""; // this will always be english
	std::string txt = ""; // this might get translated
	Color textColor;
	//Texture* textTexture = nullptr;
	//SDL_Surface* textSurface = nullptr;
	Sprite* textSprite = nullptr;

	int GetTextWidth();
	int GetTextHeight();

	Vector2 position = Vector2(0,0);
	Text(Renderer* newRenderer, TTF_Font* newFont);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, Color color);
	~Text();
	void SetText(string text, Color color = { 255, 255, 255, 255 });
	void SetTextWrapped(string text, Uint32 width);
	void Render(Renderer* renderer);
	void Render(Renderer* renderer, Vector2 offset);
	void SetPosition(float x, float y);
	void SetPosition(int x, int y);
	void SetFont(TTF_Font* newFont);
};

