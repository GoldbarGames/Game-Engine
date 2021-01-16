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
		for (int i = 0; i < val.size(); i++)
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

Texture* SpriteManager::GetImage(std::string const& imagePath) const
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

		newTexture->LoadTexture(surface);
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

		Texture* textTexture = nullptr;
		std::string path = "";
		path += data.glyph;
		textTexture = new Texture(path);
		textTexture->LoadTexture(textSurface);

		glyphTextures[data] = textTexture;

		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}

	return glyphTextures[data];
}

Texture* SpriteManager::GetTexture(TTF_Font* f, const std::string& txt, int wrapWidth)
{
	Texture* textTexture = nullptr;
	SDL_Surface* textSurface = nullptr;
	SDL_Color textColor = { 255, 255, 255, 255 };

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
		if (textImages.count(txt) == 0)
		{
			textTexture = new Texture(txt.c_str());
			textTexture->LoadTexture(textSurface);

			// TODO: Include the font name in the key.
			// In order to do this, we must pass in currentFontInfo
			// from the Text all the way to this function.
			// And the currentFontInfo must also store the name of the font
			// (or we must also have some other way of getting the name)
			// (or it can store just some kind of unique ID)
			// (Also, this needs to be different for whether the font is bold, italic, etc.)
			// (Unless "not rich text" means it only handles the regular style)

			textImages[txt] = textTexture;
		}
		else
		{
			textTexture = textImages[txt];
		}		

		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}
	else
	{
		std::cout << "ERROR loading SDL Surface" << std::endl;
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

			//TODO: Parse this filepath and check for {0}, {1}, etc., and replace them
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