#ifndef TEXT_H
#define TEXT_H
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
#include "leak_check.h"

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

class KINJO_API Text : public Entity
{
private:
	TTF_Font* font = nullptr;
public:
	std::string id = ""; // this will always be english
	std::string txt = "ERROR"; // this might get translated

	FontInfo* currentFontInfo = nullptr;

	bool isRichText = false;

	std::vector<Glyph*> glyphs;
	std::unordered_map<int, int> lineNumToIndex;

	Glyph* GetLastGlyph();

	AlignmentX alignX = AlignmentX::LEFT;
	AlignmentY alignY = AlignmentY::TOP;
	Uint32 wrapWidth = 0;

	int GetTextWidth();
	int GetTextHeight();
	glm::vec3 GetLastGlyphPosition();

	std::string GetTextString();

	Texture* GetTexture(TTF_Font* f, char c, int size);

	void SetScale(Vector2 newScale);
	Vector2 currentScale = Vector2(1, 1);

	Text();
	Text(FontInfo* newFontInfo);
	Text(FontInfo* newFontInfo, const std::string& txt, Color color);
	Text(FontInfo* newFontInfo, const std::string& txt,
		bool relPos = false, bool relScale = false);

	~Text();

	void SetText(const std::string& text, Color color = { 255, 255, 255, 255 }, Uint32 wrapWidth=0);
	void SetTextAsOneSprite(const std::string& text, Color color = { 255, 255, 255, 255 }, Uint32 wrapWidth = 0);

	void AddText(char c, Color color = { 255, 255, 255, 255 });
	void AddImage(Sprite* newSprite);

	void Render(const Renderer& renderer);
	void Render(const Renderer& renderer, glm::vec3 offset);
	void SetPosition(const float x, const float y);
	void SetPosition(const int x, const int y);
	void SetFont(TTF_Font* newFont);
	void SetFontAndInfo(FontInfo* fInfo);
	void SetColor(Color newColor);
};

#endif