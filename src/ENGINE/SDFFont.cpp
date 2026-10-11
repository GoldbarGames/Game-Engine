#include "SDFFont.h"
#include "render/RenderDevice.h"
#include "Game.h"
#include "Renderer.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include <SDL2/SDL_ttf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

// SDF bake parameters: glyphs rasterized at basePt, padded by SPREAD px on
// every side, distance saturating at SPREAD (the smoothstep band lives well
// inside that). Each padded box downsamples into one fixed atlas cell.
static const int SPREAD = 8;
// 128 texels for the 88 px padded box: 64 lost the shapes of small letters
// (a stroke was three texels across), and HUD text drawn small looked
// pixelated. The layout is in font pixels, so this changes no text's size.
static const int CELL = 128;
static const int COLS = 16;
static const int ROWS = 6;   // 16*6 = 96 cells >= 95 glyphs

static const char* SDF_VERT =
"#version 300 es\n"
"precision mediump float;\n"
"layout (location = 0) in vec3 pos;\n"
"layout (location = 1) in vec2 tex;\n"
"struct DrawData { mat4 mvp; vec4 sdfColor; };\n"   // engine DrawData shape (shaders/draw.glsl)
"uniform DrawData draw;\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"    gl_Position = draw.mvp * vec4(pos, 1.0);\n"
"    TexCoord = tex;\n"
"}";

static const char* SDF_FRAG =
"#version 300 es\n"
"precision mediump float;\n"
"in vec2 TexCoord;\n"
"out vec4 color;\n"
"uniform sampler2D theTexture;\n"
"struct DrawData { mat4 mvp; vec4 sdfColor; };\n"   // same default precision as the vertex stage
"uniform DrawData draw;\n"
// Four samples across the pixel (a rotated grid), each with its own edge: text
// drawn small, where one sample a pixel stair-stepped the strokes, comes out
// smooth, and large text looks the same as before.
"float Coverage(vec2 uv, float w)\n"
"{\n"
"    float d = texture(theTexture, uv).a;\n"
"    return smoothstep(0.5 - w, 0.5 + w, d);\n"
"}\n"
"void main()\n"
"{\n"
"    float d = texture(theTexture, TexCoord).a;\n"
"    float w = fwidth(d) * 0.55 + 0.002;\n"
"    vec2 dx = dFdx(TexCoord), dy = dFdy(TexCoord);\n"
"    float alpha = 0.25 * (Coverage(TexCoord + dx * 0.125 + dy * 0.375, w)\n"
"        + Coverage(TexCoord - dx * 0.125 - dy * 0.375, w)\n"
"        + Coverage(TexCoord + dx * 0.375 - dy * 0.125, w)\n"
"        + Coverage(TexCoord - dx * 0.375 + dy * 0.125, w));\n"
"    color = vec4(draw.sdfColor.rgb, draw.sdfColor.a * alpha);\n"
"}";

// The distance from every pixel to the nearest pixel of the other kind, exactly
// (Felzenszwalb & Huttenlocher's distance transform, one axis at a time).
//
// The bake used to find it by brute force - every atlas texel scanned a
// 17 x 17 window of the glyph - which is 112 million tests for a font, and a
// second of every start-up in a Debug build. This gives the same distances
// (the nearest such pixel is within the window exactly when it is within
// SPREAD) in a few milliseconds (docs/STARTUP.md).
static void DistanceTransform1D(const double* f, double* d, int n, int* v, double* z)
{
	int k = 0;
	v[0] = 0;
	z[0] = -1e30;
	z[1] = 1e30;
	for (int q = 1; q < n; q++)
	{
		double s = ((f[q] + (double)q * q) - (f[v[k]] + (double)v[k] * v[k])) / (2.0 * q - 2.0 * v[k]);
		while (s <= z[k])
		{
			k--;
			s = ((f[q] + (double)q * q) - (f[v[k]] + (double)v[k] * v[k])) / (2.0 * q - 2.0 * v[k]);
		}
		k++;
		v[k] = q;
		z[k] = s;
		z[k + 1] = 1e30;
	}
	k = 0;
	for (int q = 0; q < n; q++)
	{
		while (z[k + 1] < q)
			k++;
		d[q] = (double)(q - v[k]) * (q - v[k]) + f[v[k]];
	}
}

// Squared distance from each cell of a w x h grid to the nearest cell where
// `target` is set (1e20 where there is none).
static void DistanceTransform2D(const uint8_t* target, int w, int h, double* out)
{
	const int n = std::max(w, h);
	std::vector<double> f(n), d(n), z(n + 1);
	std::vector<int> v(n);
	double* fp = f.data();
	double* dp = d.data();
	for (int i = 0; i < w * h; i++)
		out[i] = target[i] ? 0.0 : 1e20;
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
			fp[y] = out[y * w + x];
		DistanceTransform1D(fp, dp, h, v.data(), z.data());
		for (int y = 0; y < h; y++)
			out[y * w + x] = dp[y];
	}
	for (int y = 0; y < h; y++)
	{
		DistanceTransform1D(out + y * w, dp, w, v.data(), z.data());
		for (int x = 0; x < w; x++)
			out[y * w + x] = dp[x];
	}
}

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
		// The surface SDL_ttf renders a glyph into starts AT THE PEN (a glyph's
		// left bearing is already inside it), unless the glyph reaches left of
		// the pen, when it starts there instead. Adding minx again shifted every
		// letter right by its own bearing, so the spacing went uneven - "rig ht",
		// "w hen" - in every game's SDF text until 2026-10-06.
		g.xoff = (float)std::min(0, minx) - SPREAD;
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

		// The glyph as a mask, in glyph-surface coords with a margin of
		// 2 x SPREAD all round: every sample point lies within SPREAD of the
		// box, which sits SPREAD px up-left of the surface, and its search
		// reaches SPREAD further. Outside the surface is outside the glyph.
		const int margin = 2 * SPREAD;
		const int gw = (gs != nullptr) ? gs->w : 0;
		const int gh = (gs != nullptr) ? gs->h : 0;
		const int span = (int)std::ceil(boxSize);
		const int W = std::max(gw, span) + 2 * margin;
		const int H = std::max(gh, span) + 2 * margin;
		std::vector<uint8_t> inMask((size_t)W * H, 0), outMask((size_t)W * H, 1);
		for (int y = 0; y < gh; y++)
		{
			for (int x = 0; x < gw; x++)
			{
				if (apix[y * gs->pitch + x * 4 + 3] > 127)
				{
					inMask[(size_t)(y + margin) * W + (x + margin)] = 1;
					outMask[(size_t)(y + margin) * W + (x + margin)] = 0;
				}
			}
		}
		std::vector<double> toInside((size_t)W * H), toOutside((size_t)W * H);
		DistanceTransform2D(inMask.data(), W, H, toInside.data());
		DistanceTransform2D(outMask.data(), W, H, toOutside.data());

		uint8_t* opix = (uint8_t*)atlasSurface->pixels;
		for (int oy = 0; oy < CELL; oy++)
		{
			for (int ox = 0; ox < CELL; ox++)
			{
				int sx = (int)((ox + 0.5f) * step) - SPREAD;
				int sy = (int)((oy + 0.5f) * step) - SPREAD;
				const size_t at = (size_t)(std::min(std::max(sy + margin, 0), H - 1)) * W
					+ std::min(std::max(sx + margin, 0), W - 1);
				bool in = inMask[at] != 0;

				// Nearest opposite-state pixel, saturating at SPREAD
				float best = sqrtf((float)(in ? toOutside[at] : toInside[at]));
				if (!(best < (float)SPREAD))
					best = (float)SPREAD;

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
	std::vector<float> verts;
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

		float quad[] = {
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
	unsigned int id = font->shader->GetID();

	// GUI space: same convention as GUI text sprites (guiProjection,
	// position offset by the GUI camera, z = -2)
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(
		position.x + renderer.guiCamera.position.x,
		position.y + renderer.guiCamera.position.y, -2.0f));
	model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

	// projection * model folded on the CPU: keeps DrawData within the 128-byte
	// push-constant budget (two mat4s + a colour would be 144).
	const glm::mat4 mvp = renderer.camera.guiProjection * model;
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "draw.mvp")), mvp);
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "draw.sdfColor")), glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f));
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	font->atlas->UseTexture();
	mesh->RenderMesh(0);
}
