#include "leak_check.h"
#include "Renderer.h"
#include "SpriteManager.h"
#include "physfs.h"
#include "Animator.h"
#include "Sprite.h"
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

Texture* SpriteManager::GetImage(std::string const& imagePath) const
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
			uint8_t* m_data = neww uint8_t[m_size];

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

		Texture* newTexture = neww Texture(imagePath.c_str());

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
void SpriteManager::ReadAnimData(const std::string& dataFilePath, std::vector<AnimState*>& animStates)
{
	std::unordered_map<std::string, std::string> args;
	ReadAnimData(dataFilePath, animStates, args);
}

//TODO: Only read this data once at the beginning and then store it for lookup later
void SpriteManager::ReadAnimData(const std::string& dataFilePath, std::vector<AnimState*>& animStates, 
	std::unordered_map<std::string, std::string>& args)
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

			animStates.push_back(new AnimState(stateName, stateSpeed,
				new Sprite(spriteStartFrame, spriteEndFrame, spriteFrameWidth, spriteFrameHeight,
					*this, spriteFilePath,
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