#include "leak_check.h"
#include "Renderer.h"
#include "SpriteManager.h"
#include "Animator.h"
#include "Sprite.h"
#include <sstream>
#include <iterator>

#if PHYSFS_ENABLED
#include "physfs.h"
#endif

SpriteManager::SpriteManager()
{
	//Note: To link PHYSFS, use the static library
#if PHYSFS_ENABLED
	PHYSFS_init(NULL);
	PHYSFS_addToSearchPath("assets.wdk", 1);
#endif
}

SpriteManager::~SpriteManager()
{	
	for (auto& [key, val] : images)
	{
		if (val != nullptr)
			delete_it(val);
	}

	for (auto& [key, val] : glyphTextures)
	{
		if (val != nullptr)
			delete_it(val);
	}

	for (auto& [key, val] : textImages)
	{
		if (val != nullptr)
			delete_it(val);
	}

	for (auto& [key, val] : animationStates)
	{
		for (size_t i = 0; i < val.size(); i++)
		{
			if (val[i] != nullptr)
				delete_it(val[i]);
		}		
	}

#if PHYSFS_ENABLED
	PHYSFS_deinit();
#endif
}

void SpriteManager::Init(Renderer* r)
{
	renderer = r;
	Animator::spriteManager = this;
}

void SpriteManager::ClearCache(std::string const& imagePath)
{
	auto it = images.find(imagePath);
	if (it != images.end()) 
	{
		images.erase(it);
	}
}

Texture* SpriteManager::GetImage(std::string const& imagePath, Texture::Filter filter) const
{
	if (images.count(imagePath) == 0)
	{
		SDL_Surface* surface;

#if PHYSFS_ENABLED

			PHYSFS_file* myfile = PHYSFS_openRead(imagePath.c_str());

			if (myfile == nullptr)
			{
				myfile = PHYSFS_openRead("assets/gui/white.png");
				std::cout << "FAILED TO LOAD SPRITE: " << imagePath << std::endl;
			}
				
			PHYSFS_sint64  m_size = PHYSFS_fileLength(myfile);
			uint8_t* m_data = new uint8_t[m_size];

			int length_read = PHYSFS_read(myfile, m_data, 1, m_size);

			if (length_read != (int)m_size)
			{
				delete[] m_data;
				m_data = 0;
				return nullptr;
			}

			PHYSFS_close(myfile);

			SDL_RWops *rw = SDL_RWFromMem(m_data, m_size);

			surface = IMG_Load_RW(rw, 0);

#else

		surface = IMG_Load(imagePath.c_str());
		if (surface == nullptr)
		{
			surface = IMG_Load("assets/gui/white.png");
			std::cout << "FAILED TO LOAD SPRITE: " << imagePath << std::endl;
		}

#endif

		Texture* newTexture = new Texture(imagePath.c_str());

		newTexture->LoadTexture(surface, false, filter);
		images[imagePath] = newTexture;

		SDL_FreeSurface(surface);
	}
		
	return images[imagePath];
}

Texture* SpriteManager::GetTexture(TTF_Font* f, char c, int size)
{
	GlyphSurfaceData data;
	data.fontName = TTF_FontFaceStyleName(f);
	data.glyph = c; //TODO: What happens if this is /0?
	data.color = { 255, 255, 255, 255 };
	data.size = size;

	if (glyphTextures.count(data) == 0)
	{
		SDL_Surface* textSurface = TTF_RenderGlyph_Blended(f, data.glyph, data.color);

		// Rich text lays glyphs out by their texture size, so each glyph
		// texture must be a proper "cell": as wide as the glyph's ADVANCE (pen
		// movement, including side bearings) and as tall as the font's line
		// height, with the ink placed at its left bearing / baseline. Rendering
		// the raw ink bitmap (its old behavior) only worked for monospace/CJK
		// fonts where ink width ~= advance; proportional fonts (e.g. Aileron)
		// came out cramped and vertically misaligned. Building the cell here
		// makes the layout correct for ANY font without touching Text.cpp.
		int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
		bool haveMetrics = (TTF_GlyphMetrics(f, data.glyph,
			&minx, &maxx, &miny, &maxy, &advance) == 0);
		int lineHeight = TTF_FontHeight(f);

		SDL_Surface* cell = nullptr;
		// TTF_RenderGlyph_Blended already returns a surface that is FULL line
		// height and baseline-aligned vertically (only the WIDTH is ink-tight,
		// starting at the glyph's left ink edge). So we only need to widen it to
		// the glyph's ADVANCE, placing the ink at its left side-bearing (minx).
		// Do NOT shift it vertically - that clips the bottom of every glyph.
		if (haveMetrics && advance > 0 && textSurface != nullptr
			&& advance >= textSurface->w)
		{
			cell = SDL_CreateRGBSurfaceWithFormat(0, advance, textSurface->h, 32,
				SDL_PIXELFORMAT_RGBA32);
			if (cell != nullptr)
			{
				SDL_FillRect(cell, nullptr, SDL_MapRGBA(cell->format, 0, 0, 0, 0));
				// Copy the ink (with its alpha) in at the left bearing; clamp so
				// it can't spill past the cell (negative bearing / overhang).
				// Don't blend against the empty transparent background.
				int dx = minx;
				if (dx < 0) dx = 0;
				if (dx + textSurface->w > advance) dx = advance - textSurface->w;
				if (dx < 0) dx = 0;
				SDL_SetSurfaceBlendMode(textSurface, SDL_BLENDMODE_NONE);
				SDL_Rect dst = { dx, 0, 0, 0 };
				SDL_BlitSurface(textSurface, nullptr, cell, &dst);
			}
		}

		Texture* textTexture = new Texture(std::string(1, data.glyph));
		textTexture->LoadTexture(cell != nullptr ? cell : textSurface,
			false, Texture::Filter::Smooth);

		glyphTextures[data] = textTexture;

		if (cell != nullptr)
			SDL_FreeSurface(cell);
		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}

	return glyphTextures[data];
}

FontAtlas* SpriteManager::GetFontAtlas(TTF_Font* f, int size)
{
	if (f == nullptr)
		return nullptr;

	// Key by family + style + size so different fonts (Aileron vs SazanamiGothic)
	// never collide - the style name alone ("Regular") is shared across families.
	const char* fam = TTF_FontFaceFamilyName(f);
	const char* sty = TTF_FontFaceStyleName(f);
	std::string key = std::string(fam ? fam : "?") + "|" + std::string(sty ? sty : "?")
		+ "|" + std::to_string(size);
	auto found = fontAtlases.find(key);
	if (found != fontAtlases.end())
		return &found->second;

	const int first = 32, last = 126;      // printable ASCII
	const int gutter = 2;                  // transparent margin so filtering can't bleed
	const int lineHeight = TTF_FontHeight(f);
	SDL_Color white = { 255, 255, 255, 255 };

	// Pass 1: build each glyph's advance-width cell surface (same construction as
	// the per-glyph GetTexture path) and track the widest advance for the grid.
	struct Cell { char c; SDL_Surface* surf; int advance; };
	std::vector<Cell> cells;
	int maxAdvance = 1;
	for (int ch = first; ch <= last; ch++)
	{
		SDL_Surface* ink = TTF_RenderGlyph_Blended(f, (Uint16)ch, white);
		int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
		bool haveMetrics = (TTF_GlyphMetrics(f, (Uint16)ch, &minx, &maxx, &miny, &maxy, &advance) == 0);

		SDL_Surface* cell = nullptr;
		if (haveMetrics && advance > 0 && ink != nullptr && advance >= ink->w)
		{
			cell = SDL_CreateRGBSurfaceWithFormat(0, advance, ink->h, 32, SDL_PIXELFORMAT_RGBA32);
			if (cell != nullptr)
			{
				SDL_FillRect(cell, nullptr, SDL_MapRGBA(cell->format, 0, 0, 0, 0));
				int dx = minx;
				if (dx < 0) dx = 0;
				if (dx + ink->w > advance) dx = advance - ink->w;
				if (dx < 0) dx = 0;
				SDL_SetSurfaceBlendMode(ink, SDL_BLENDMODE_NONE);
				SDL_Rect dst = { dx, 0, 0, 0 };
				SDL_BlitSurface(ink, nullptr, cell, &dst);
			}
		}

		SDL_Surface* use = (cell != nullptr) ? cell : ink;   // raw ink for rare overhang glyphs
		if (use == nullptr)
			continue;
		int adv = (advance > 0) ? advance : use->w;
		cells.push_back({ (char)ch, use, adv });
		if (adv > maxAdvance) maxAdvance = adv;
		if (cell != nullptr && ink != nullptr)
			SDL_FreeSurface(ink);            // ink was copied into the cell
	}
	if (cells.empty())
		return nullptr;

	// Pass 2: pack the cells into a uniform grid (16 columns) with gutters.
	const int cols = 16;
	const int slotW = maxAdvance + gutter;
	const int slotH = lineHeight + gutter;
	const int rows = ((int)cells.size() + cols - 1) / cols;
	const int atlasW = cols * slotW;
	const int atlasH = rows * slotH;

	SDL_Surface* atlas = SDL_CreateRGBSurfaceWithFormat(0, atlasW, atlasH, 32, SDL_PIXELFORMAT_RGBA32);
	if (atlas == nullptr)
	{
		for (Cell& c : cells) SDL_FreeSurface(c.surf);
		return nullptr;
	}
	SDL_FillRect(atlas, nullptr, SDL_MapRGBA(atlas->format, 0, 0, 0, 0));

	FontAtlas fa;
	fa.lineHeight = lineHeight;
	for (size_t i = 0; i < cells.size(); i++)
	{
		int x = (int)(i % cols) * slotW;
		int y = (int)(i / cols) * slotH;
		SDL_SetSurfaceBlendMode(cells[i].surf, SDL_BLENDMODE_NONE);
		SDL_Rect dst = { x, y, 0, 0 };
		SDL_BlitSurface(cells[i].surf, nullptr, atlas, &dst);

		GlyphUV uv;
		uv.u0 = (float)x / atlasW;
		uv.v0 = (float)y / atlasH;
		uv.u1 = (float)(x + cells[i].advance) / atlasW;
		uv.v1 = (float)(y + lineHeight) / atlasH;
		uv.cellW = cells[i].advance;
		uv.cellH = lineHeight;
		fa.glyphs[cells[i].c] = uv;

		SDL_FreeSurface(cells[i].surf);
	}

	Texture* tex = new Texture("__fontatlas__");
	tex->LoadTexture(atlas, false, Texture::Filter::Smooth);
	fa.texture = tex;
	SDL_FreeSurface(atlas);

	fontAtlases[key] = fa;
	return &fontAtlases[key];
}

Texture* SpriteManager::GetTexture(TTF_Font* f, const std::string& txt, int wrapWidth)
{
	Texture* textTexture = nullptr;
	SDL_Surface* textSurface = nullptr;
	SDL_Color textColor = { 255, 255, 255, 255 };

	if (textImages.count(txt) == 0)
	{
		if (wrapWidth > 0)
		{
			textSurface = TTF_RenderText_Blended_Wrapped(f, txt.c_str(), textColor, wrapWidth);
		}
		else
		{
			textSurface = TTF_RenderText_Blended(f, txt.c_str(), textColor);
		}

		if (textSurface != nullptr)
		{
			textTexture = new Texture(txt.c_str());
			textTexture->LoadTexture(textSurface, false, Texture::Filter::Smooth);

			// TODO: Include the font name in the key.
			// In order to do this, we must pass in currentFontInfo
			// from the Text all the way to this function.
			// And the currentFontInfo must also store the name of the font
			// (or we must also have some other way of getting the name)
			// (or it can store just some kind of unique ID)
			// (Also, this needs to be different for whether the font is bold, italic, etc.)
			// (Unless "not rich text" means it only handles the regular style)

			textImages[txt] = textTexture;
			SDL_FreeSurface(textSurface);
		}
		else
		{
			std::cout << "ERROR loading SDL Surface for text \"" << txt << "\"" << std::endl;
		}
	}
	else
	{
		textTexture = textImages[txt];
	}

	return textTexture;
}


std::vector<AnimState*> SpriteManager::ReadAnimData(const std::string& dataFilePath) const
{
	std::unordered_map<std::string, std::string> args;
	return ReadAnimData(dataFilePath, args);
}

// We want to read in the file only once, creating a base set of states that are stored here.
// Then whenever an object is created, we give the object its own copy of the states.
// That way, those states can be manipulated (and eventually deleted) by the local object.
std::vector<AnimState*> SpriteManager::ReadAnimData(const std::string& dataFilePath, 
	std::unordered_map<std::string, std::string>& args) const
{
	std::vector<AnimState*> animStates;

	//std::cout << dataFilePath << std::endl;
	std::string animStateKey = dataFilePath;
	for (auto& [key, val] : args)
	{
		animStateKey += val;
	}

	// If we have already read this file, grab it from the table
	if (animationStates.count(animStateKey) != 0)
	{
		return animationStates[animStateKey];
	}

	//std::cout << "Base anim: " << std::endl;

	// Get anim data from the file
	std::ifstream fin;
	fin.open(dataFilePath);

	std::string animData = "";
	for (std::string line; std::getline(fin, line); )
	{
		// Remove trailing \r if present (Windows line endings)
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		animData += line + "\n";
	}

	fin.close();

	// Go through the data and add all states
	std::stringstream ss{ animData };

	char lineChar[256];
	ss.getline(lineChar, 256);

	std::string stateName = "";
	int stateSpeed = 0;
	int spriteStartFrame = 0;
	int spriteEndFrame = 0;
	int spriteFrameWidth = 0;
	int spriteFrameHeight = 0;

	int spritePivotX = 0;
	int spritePivotY = 0;
	int filePathIndex = 0;

	int index = 0;

	std::string filePathInput = "";
	std::string argumentNumber = "";
	std::string spriteFilePath = "";

	try
	{
		while (ss.good() && !ss.eof())
		{
			std::istringstream buf(lineChar);
			std::istream_iterator<std::string> beg(buf), end;
			std::vector<std::string> tokens(beg, end);

			index = 0;

			stateName = tokens[index++];
			stateSpeed = std::stoi(tokens[index++]);
			spriteStartFrame = std::stoi(tokens[index++]);
			spriteEndFrame = std::stoi(tokens[index++]);
			spriteFrameWidth = std::stoi(tokens[index++]);
			spriteFrameHeight = std::stoi(tokens[index++]);

			filePathInput = tokens[index++];
			spriteFilePath = "";

			//Parse this filepath and check for {0}, {1}, etc., and replace them
			filePathIndex = 0;

			while (filePathIndex < filePathInput.size())
			{
				if (filePathInput[filePathIndex] == '{')
				{
					filePathIndex++;
					argumentNumber = "";
					while (filePathIndex < filePathInput.size() && filePathInput[filePathIndex] != '}')
					{
						argumentNumber += filePathInput[filePathIndex];
						filePathIndex++;
					}

					filePathIndex++;
					if (args.count(argumentNumber) == 1)
					{
						spriteFilePath += args[argumentNumber];
					}					
				}
				else
				{
					spriteFilePath += filePathInput[filePathIndex];
					filePathIndex++;
				}
			}

			spritePivotX = std::stoi(tokens[index++]);
			spritePivotY = std::stoi(tokens[index++]);

			animStates.push_back(new AnimState(stateName, spriteFilePath, stateSpeed, spriteStartFrame, spriteEndFrame, 
				spriteFrameWidth, spriteFrameHeight, spritePivotX, spritePivotY));

			ss.getline(lineChar, 256);
		}
	}
	catch (const std::exception & ex)
	{
		const char* message = ex.what();
		std::cout << message << std::endl;
	}

	animationStates[animStateKey] = animStates;

	// Call recursively so that the object's animator points to a copy.
	return animationStates[animStateKey];
}