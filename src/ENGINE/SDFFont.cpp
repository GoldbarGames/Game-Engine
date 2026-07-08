#include "SDFFont.h"
#include "Game.h"
#include "Renderer.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include <SDL2/SDL_ttf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <iostream>

// SDF bake parameters: glyphs rasterized at basePt, padded by SPREAD px on
// every side, distance saturating at SPREAD (the smoothstep band lives well
// inside that). Each padded box downsamples into one fixed atlas cell.
static const int SPREAD = 8;
static const int CELL = 64;
static const int COLS = 16;
static const int ROWS = 6;   // 16*6 = 96 cells >= 95 glyphs

static const char* SDF_VERT =
"#version 300 es\n"
"precision mediump float;\n"
"layout (location = 0) in vec3 pos;\n"
"layout (location = 1) in vec2 tex;\n"
"uniform mat4 model;\n"
"uniform mat4 projection;\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * model * vec4(pos, 1.0);\n"
"    TexCoord = tex;\n"
"}";

static const char* SDF_FRAG =
"#version 300 es\n"
"precision mediump float;\n"
"in vec2 TexCoord;\n"
"out vec4 color;\n"
"uniform sampler2D theTexture;\n"
"uniform vec4 sdfColor;\n"
"void main()\n"
"{\n"
"    float d = texture(theTexture, TexCoord).a;\n"
"    float w = fwidth(d) * 0.8 + 0.004;\n"
"    float alpha = smoothstep(0.5 - w, 0.5 + w, d);\n"
"    color = vec4(sdfColor.rgb, sdfColor.a * alpha);\n"
"}";

SDFFont::SDFFont(Game& game, const std::string& ttfPath)
{
	TTF_Font* font = TTF_OpenFont(ttfPath.c_str(), (int)basePt);
	if (font == nullptr)
	{
		std::cout << "ERROR: SDFFont could not open " << ttfPath << std::endl;
		return;
	}

	// Uniform padded box: tall enough for ascenders+descenders at basePt
	boxSize = basePt * 1.5f + 2.0f * SPREAD;

	SDL_Surface* atlasSurface = SDL_CreateRGBSurfaceWithFormat(
		0, COLS * CELL, ROWS * CELL, 32, SDL_PIXELFORMAT_RGBA32);
	SDL_FillRect(atlasSurface, nullptr, 0);

	const SDL_Color white = { 255, 255, 255, 255 };
	const int ascent = TTF_FontAscent(font);
	const float step = boxSize / CELL;         // hi-res px per atlas texel
	const float norm = 2.0f * SPREAD;          // distance normalization

	for (int i = 0; i < CHAR_COUNT; i++)
	{
		char ch = (char)(FIRST_CHAR + i);
		int minx = 0, maxx = 0, miny = 0, maxy = 0, adv = 0;
		TTF_GlyphMetrics(font, ch, &minx, &maxx, &miny, &maxy, &adv);

		Glyph& g = glyphs[i];
		g.advance = (float)adv;
		g.xoff = (float)minx - SPREAD;
		// Glyph surfaces are full-cell height: row 0 is the ascent line,
		// NOT the glyph bbox top (maxy) - anchoring at maxy dropped short
		// glyphs like 's' below the baseline
		g.yoff = (float)ascent + SPREAD;

		SDL_Surface* gs = TTF_RenderGlyph_Blended(font, ch, white);
		int col = i % COLS, row = i / COLS;
		g.u0 = (float)(col * CELL) / atlasSurface->w;
		g.v0 = (float)(row * CELL) / atlasSurface->h;
		g.u1 = (float)(col * CELL + CELL) / atlasSurface->w;
		g.v1 = (float)(row * CELL + CELL) / atlasSurface->h;

		if (gs != nullptr)
			SDL_LockSurface(gs);
		uint8_t* apix = (gs != nullptr) ? (uint8_t*)gs->pixels : nullptr;

		// inside() in glyph-surface coords; the box top-left sits SPREAD px
		// up-left of the surface
		auto inside = [&](int sx, int sy) -> bool
		{
			if (apix == nullptr || sx < 0 || sy < 0 || sx >= gs->w || sy >= gs->h)
				return false;
			return apix[sy * gs->pitch + sx * 4 + 3] > 127;
		};

		uint8_t* opix = (uint8_t*)atlasSurface->pixels;
		for (int oy = 0; oy < CELL; oy++)
		{
			for (int ox = 0; ox < CELL; ox++)
			{
				int sx = (int)((ox + 0.5f) * step) - SPREAD;
				int sy = (int)((oy + 0.5f) * step) - SPREAD;
				bool in = inside(sx, sy);

				// Nearest opposite-state pixel within the spread window
				float best = (float)SPREAD;
				for (int dy = -SPREAD; dy <= SPREAD; dy++)
				{
					for (int dx = -SPREAD; dx <= SPREAD; dx++)
					{
						if (inside(sx + dx, sy + dy) != in)
						{
							float d = sqrtf((float)(dx * dx + dy * dy));
							if (d < best) best = d;
						}
					}
				}

				float signedDist = in ? best : -best;
				float a = 0.5f + signedDist / norm;
				if (a < 0.0f) a = 0.0f;
				if (a > 1.0f) a = 1.0f;

				int px = col * CELL + ox, py = row * CELL + oy;
				uint8_t* p = opix + py * atlasSurface->pitch + px * 4;
				p[0] = 255; p[1] = 255; p[2] = 255;
				p[3] = (uint8_t)(a * 255.0f);
			}
		}

		if (gs != nullptr)
		{
			SDL_UnlockSurface(gs);
			SDL_FreeSurface(gs);
		}
	}

	TTF_CloseFont(font);

	atlas = new Texture("");
	atlas->LoadTexture(atlasSurface, false, Texture::Filter::Smooth);
	SDL_FreeSurface(atlasSurface);

	// Engine-internal SDF shader parked at index 100, clear of the game's
	// shaders.dat range
	if (game.renderer.shaders.count(100) == 0)
		game.renderer.CreateShader(100, SDF_VERT, SDF_FRAG, true);
	shader = game.renderer.shaders[100];

	loaded = true;
	std::cout << "SDFFont baked " << ttfPath << " (" << COLS * CELL << "x"
		<< ROWS * CELL << " atlas)" << std::endl;
}

SDFFont::~SDFFont()
{
	if (atlas != nullptr)
	{
		delete atlas;
		atlas = nullptr;
	}
	// The shader is owned by the renderer's shader map
}

float SDFFont::MeasureWidth(const std::string& s) const
{
	float pen = 0.0f;
	for (char ch : s)
	{
		if (ch < FIRST_CHAR || ch >= FIRST_CHAR + CHAR_COUNT)
			continue;
		pen += glyphs[ch - FIRST_CHAR].advance;
	}
	return pen;
}

SDFText::SDFText(SDFFont* f)
{
	font = f;
}

SDFText::~SDFText()
{
	if (mesh != nullptr)
	{
		delete mesh;
		mesh = nullptr;
	}
}

void SDFText::SetText(const std::string& s)
{
	if (font == nullptr || !font->loaded)
		return;
	if (s == text && mesh != nullptr)
		return;
	text = s;

	delete mesh;
	mesh = nullptr;

	// One quad per glyph in font pixels: pen at the origin baseline,
	// +y down (GUI convention). Layout: pos3, uv2, normal3.
	std::vector<GLfloat> verts;
	std::vector<unsigned int> inds;
	float pen = 0.0f;
	float box = font->boxSize;

	for (char ch : s)
	{
		if (ch < SDFFont::FIRST_CHAR || ch >= SDFFont::FIRST_CHAR + SDFFont::CHAR_COUNT)
			continue;
		const SDFFont::Glyph& g = font->glyphs[ch - SDFFont::FIRST_CHAR];

		float x0 = pen + g.xoff, y0 = -g.yoff;
		float x1 = x0 + box, y1 = y0 + box;
		unsigned int base = (unsigned int)(verts.size() / 8);

		GLfloat quad[] = {
			x0, y0, 0,  g.u0, g.v0,  0, 0, 1,
			x1, y0, 0,  g.u1, g.v0,  0, 0, 1,
			x0, y1, 0,  g.u0, g.v1,  0, 0, 1,
			x1, y1, 0,  g.u1, g.v1,  0, 0, 1,
		};
		verts.insert(verts.end(), quad, quad + 32);
		unsigned int quadInds[] = { base, base + 1, base + 2, base + 2, base + 1, base + 3 };
		inds.insert(inds.end(), quadInds, quadInds + 6);

		pen += g.advance;
	}

	if (verts.empty())
		return;

	mesh = new Mesh();
	mesh->CreateMesh(verts.data(), inds.data(),
		(unsigned int)verts.size(), (unsigned int)inds.size(), 8, 3, 5);
}

void SDFText::Render(const Renderer& renderer)
{
	if (mesh == nullptr || font == nullptr || !font->loaded
		|| font->shader == nullptr || font->atlas == nullptr)
		return;

	font->shader->UseShader();
	GLuint id = font->shader->GetID();

	// GUI space: same convention as GUI text sprites (guiProjection,
	// position offset by the GUI camera, z = -2)
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(
		position.x + renderer.guiCamera.position.x,
		position.y + renderer.guiCamera.position.y, -2.0f));
	model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

	glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(id, "projection"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.guiProjection));
	glUniform4f(glGetUniformLocation(id, "sdfColor"),
		color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
	glUniform1i(glGetUniformLocation(id, "theTexture"), 0);

	font->atlas->UseTexture();
	mesh->RenderMesh(0);
}
