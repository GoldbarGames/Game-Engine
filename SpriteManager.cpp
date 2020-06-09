#include "Renderer.h"
#include "SpriteManager.h"
#include "physfs.h"
#include "Animator.h"
#include <sstream>
#include <iterator>

SpriteManager::SpriteManager(Renderer* r)
{
	//Note: To link PHYSFS, use the static library
	PHYSFS_init(NULL);
	PHYSFS_addToSearchPath("assets.wdk", 1);
	renderer = r;
}

SpriteManager::~SpriteManager()
{	
	PHYSFS_deinit();
}

Texture* SpriteManager::GetImage(std::string const& imagePath)
{
	if (images[imagePath].get() == nullptr)
	{
		bool loadFromFile = true;

#if _DEBUG
		loadFromFile = false;
#endif

		SDL_Surface * surface;

		if (loadFromFile)
		{
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
				return false;
			}

			PHYSFS_close(myfile);

			SDL_RWops *rw = SDL_RWFromMem(m_data, m_size);

			surface = IMG_Load_RW(rw, 0);
		}
		else
		{
			surface = IMG_Load(imagePath.c_str());
			if (surface == nullptr)
			{
				surface = IMG_Load("assets/gui/white.png");
				std::cout << "FAILED TO LOAD SPRITE: " << imagePath << std::endl;
			}
				
		}	

		Texture* newTexture = new Texture(imagePath.c_str());

		newTexture->LoadTexture(surface);
		images[imagePath].reset(newTexture);

		SDL_FreeSurface(surface);
	}
		
	return images[imagePath].get();
}

Vector2 SpriteManager::GetPivotPoint(std::string const& filename)
{
	return pivotPoints[filename];
}

//TODO: Only read this data once at the beginning and then store it for lookup later
void SpriteManager::ReadAnimData(std::string dataFilePath, std::vector<AnimState*>& animStates)
{
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

	try
	{
		while (ss.good() && !ss.eof())
		{
			std::istringstream buf(lineChar);
			std::istream_iterator<std::string> beg(buf), end;
			std::vector<std::string> tokens(beg, end);

			int index = 0;

			std::string stateName = tokens[index++];
			int stateSpeed = std::stoi(tokens[index++]);
			int spriteStartFrame = std::stoi(tokens[index++]);
			int spriteEndFrame = std::stoi(tokens[index++]);
			int spriteFrameWidth = std::stoi(tokens[index++]);
			int spriteFrameHeight = std::stoi(tokens[index++]);

			std::string spriteFilePath = tokens[index++];
			int spritePivotX = std::stoi(tokens[index++]);
			int spritePivotY = std::stoi(tokens[index++]);

			animStates.push_back(new AnimState(stateName, stateSpeed,
				new Sprite(spriteStartFrame, spriteEndFrame, spriteFrameWidth, spriteFrameHeight,
					this, spriteFilePath,
					renderer->shaders[ShaderName::Default],
					Vector2(spritePivotX, spritePivotY))));

			ss.getline(lineChar, 256);
		}
	}
	catch (const std::exception & ex)
	{
		const char* message = ex.what();
		std::cout << message << std::endl;
	}
}