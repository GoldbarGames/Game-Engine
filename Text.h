#pragma once
#include "SDL.h"
#include <SDL_ttf.h>
#include <string>

#include "GL/glew.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Entity.h"
#include "globals.h"

#include "Texture.h"
#include "Vector2.h"


using std::string;

class Renderer;
class Sprite;

class Text : public Entity
{
private:
	Renderer* renderer;
	TTF_Font* font;
public:
	std::string id = ""; // this will always be english
	std::string txt = "ERROR"; // this might get translated
	Color textColor = { 255, 255, 255, 255 };

	//Sprite* textSprite = nullptr;

	int GetTextWidth();
	int GetTextHeight();
	//Sprite* GetSprite() { return textSprite; };

	//Vector2 position = Vector2(0,0);
	Text(Renderer* newRenderer, TTF_Font* newFont);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt,
		bool relPos = false, bool relScale = false);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, Color color);
	~Text();
	void SetText(string text, Color color = { 255, 255, 255, 255 }, Uint32 wrapWidth=0);
	void Render(Renderer* renderer);
	void Render(Renderer* renderer, Vector2 offset);
	void SetPosition(const float x, const float y);
	void SetPosition(const int x, const int y);
	void SetFont(TTF_Font* newFont);
};

