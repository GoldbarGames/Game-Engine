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

enum class AlignmentX { LEFT, CENTER, RIGHT };
enum class AlignmentY { TOP, CENTER, BOTTOM };

struct Glyph
{
	Sprite* sprite = nullptr;
	Vector2 position = Vector2(0,0);

	~Glyph() 
	{
		delete_it(sprite);
	}
};



struct GlyphSurfaceData
{
	std::string fontName = "";
	char glyph = 'x';
	SDL_Color color = { 255, 255, 255, 255 };

	bool operator==(const GlyphSurfaceData& other) const
	{
		if (fontName != other.fontName)
			return false;

		if (glyph != other.glyph)
			return false;

		if (color.r != other.color.r)
			return false;

		if (color.g != other.color.g)
			return false;

		if (color.b != other.color.b)
			return false;

		if (color.a != other.color.a)
			return false;

		return true;
	}

};

class GlyphHashFunction
{
public:
	std::size_t operator()(const GlyphSurfaceData& k) const
	{
		return ((std::hash<std::string>()(k.fontName)
			^ (std::hash<char>()(k.glyph) << 1)) >> 1)
			^ (std::hash<int>()(k.color.r) << 1)
			^ (std::hash<int>()(k.color.g) << 1)
			^ (std::hash<int>()(k.color.b) << 1)
			^ (std::hash<int>()(k.color.a) << 1);
	}
};



class Text : public Entity
{
private:
	Renderer* renderer;
	TTF_Font* font;
	// The key = font + letter + color (turned into numbers)
	//std::vector<GlyphSurfaceData*>,

	//TODO: Maybe move this to a location where multiple text objects can all share the same memory
	std::unordered_map<GlyphSurfaceData, std::unique_ptr<Texture, ImageDeleter>, GlyphHashFunction> glyphTextures;
public:
	std::string id = ""; // this will always be english
	std::string txt = "ERROR"; // this might get translated
	Color textColor = { 255, 255, 255, 255 };

	std::vector<Glyph*> glyphs;

	AlignmentX alignX = AlignmentX::LEFT;
	AlignmentY alignY = AlignmentY::TOP;
	Uint32 wrapWidth = 0;

	//Sprite* textSprite = nullptr;

	int GetTextWidth();
	int GetTextHeight();
	//Sprite* GetSprite() { return textSprite; };

	std::string GetTextString();

	Texture* GetTexture(TTF_Font* f, char c, SDL_Color col);

	void SetScale(Vector2 newScale);

	//Vector2 position = Vector2(0,0);
	Text(Renderer* newRenderer, TTF_Font* newFont);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt,
		bool relPos = false, bool relScale = false);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, Color color);
	~Text();
	void SetText(string text, Color color = { 255, 255, 255, 255 }, Uint32 wrapWidth=0);
	void AddText(char c, Color color = { 255, 255, 255, 255 });
	void Render(Renderer* renderer);
	void Render(Renderer* renderer, Vector2 offset);
	void SetPosition(const float x, const float y);
	void SetPosition(const int x, const int y);
	void SetFont(TTF_Font* newFont);
};

