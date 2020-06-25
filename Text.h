#ifndef TEXT_H
#define TEXT_H
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

#include "Glyph.h"

using std::string;

class Renderer;
class Sprite;
class Entity;

#ifndef STRUCT_FONT_INFO
#define STRUCT_FONT_INFO
struct FontInfo;
#endif

enum class AlignmentX { LEFT, CENTER, RIGHT };
enum class AlignmentY { TOP, CENTER, BOTTOM };

class Text : public Entity
{
private:
	Renderer* renderer = nullptr;
	TTF_Font* font = nullptr;

	//TODO: Maybe move this to a location where multiple text objects can all share the same memory
	std::unordered_map<GlyphSurfaceData, std::unique_ptr<Texture, ImageDeleter>, GlyphHashFunction> glyphTextures;
	void SetTextAsOneSprite(string text, Color color = { 255, 255, 255, 255 }, Uint32 wrapWidth = 0);

public:
	std::string id = ""; // this will always be english
	std::string txt = "ERROR"; // this might get translated
	Color textColor = { 255, 255, 255, 255 };
	FontInfo* currentFontInfo = nullptr;

	bool isRichText = false;

	std::vector<Glyph*> glyphs;
	std::unordered_map<int, int> lineNumToIndex;

	AlignmentX alignX = AlignmentX::LEFT;
	AlignmentY alignY = AlignmentY::TOP;
	Uint32 wrapWidth = 0;

	int GetTextWidth();
	int GetTextHeight();
	Vector2 GetLastGlyphPosition();

	std::string GetTextString();

	Texture* GetTexture(TTF_Font* f, char c, SDL_Color col);

	void SetScale(Vector2 newScale);
	Vector2 currentScale = Vector2(1, 1);

	Text(Renderer* newRenderer, FontInfo* newFontInfo);
	Text(Renderer* newRenderer, FontInfo* newFontInfo, std::string txt, Color color);
	Text(Renderer* newRenderer, FontInfo* newFontInfo, std::string txt,
		bool relPos = false, bool relScale = false);

	~Text();

	void SetText(std::string text, Color color = { 255, 255, 255, 255 }, Uint32 wrapWidth=0);
	void AddText(char c, Color color = { 255, 255, 255, 255 });
	void AddImage(Sprite* newSprite);

	void Render(Renderer* renderer);
	void Render(Renderer* renderer, Vector2 offset);
	void SetPosition(const float x, const float y);
	void SetPosition(const int x, const int y);
	void SetFont(TTF_Font* newFont);
	void SetColor(Color newColor);
};

#endif