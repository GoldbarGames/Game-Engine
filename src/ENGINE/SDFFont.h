#ifndef SDFFONT_H
#define SDFFONT_H
#pragma once

#include <string>
#include <glm/glm.hpp>
#include "globals.h"
#include "leak_check.h"

class Game;
class Texture;
class ShaderProgram;
class Mesh;
class Renderer;

// Signed-distance-field font: bakes ASCII 32-126 into one distance-field
// atlas at load (SDL_ttf rasterization + brute-force SDF), rendered by
// SDFText with a smoothstep shader - crisp at ANY scale from one texture,
// with no per-string texture bakes (the proper fix for scaled/dynamic text).
class KINJO_API SDFFont
{
public:
	struct Glyph
	{
		float u0 = 0, v0 = 0, u1 = 0, v1 = 0;  // atlas cell uv rect
		float xoff = 0;      // box left relative to the pen (font px)
		float yoff = 0;      // box top above the baseline (font px)
		float advance = 0;   // pen advance (font px)
	};

	static const int FIRST_CHAR = 32;
	static const int CHAR_COUNT = 95;

	Glyph glyphs[CHAR_COUNT];
	Texture* atlas = nullptr;
	ShaderProgram* shader = nullptr;   // registered as renderer shader 100
	bool loaded = false;

	// All geometry below is in "font pixels" of the bake size
	float basePt = 48.0f;    // rasterization size; quads scale from this
	float boxSize = 0.0f;    // uniform padded glyph box (basePt*1.5 + 2*spread)

	SDFFont(Game& game, const std::string& ttfPath);
	~SDFFont();

	// Pen width of a string in font pixels (multiply by SDFText::scale);
	// matches SDFText::SetText layout exactly (sum of advances, no kerning)
	float MeasureWidth(const std::string& s) const;
};

// One dynamic string of SDF glyph quads (a single Mesh, rebuilt on SetText),
// drawn in GUI coordinates (2x window pixels) like the engine's Text.
class KINJO_API SDFText
{
public:
	SDFFont* font = nullptr;
	Mesh* mesh = nullptr;
	std::string text;
	glm::vec2 position = glm::vec2(0, 0);  // GUI coords of the first pen (baseline)
	float scale = 1.0f;                    // 1 = glyphs at the bake size
	Color color = { 255, 255, 255, 255 };

	SDFText(SDFFont* f);
	~SDFText();

	void SetText(const std::string& s);
	void SetPosition(float x, float y) { position = glm::vec2(x, y); }

	void Render(const Renderer& renderer);
};

#endif
