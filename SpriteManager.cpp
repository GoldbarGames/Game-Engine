#include "Renderer.h"
#include "SpriteManager.h"
#include "physfs.h"

SpriteManager::SpriteManager()
{
	//Note: To link PHYSFS, use the static library
	PHYSFS_init(NULL);
	PHYSFS_addToSearchPath("assets.wdk", 1);
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
		loadFromFile = true;
#endif

		SDL_Surface * surface;

		if (loadFromFile)
		{
			PHYSFS_file* myfile = PHYSFS_openRead(imagePath.c_str());

			if (myfile == nullptr)
				myfile = PHYSFS_openRead("assets/gui/white.png");			

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
				surface = IMG_Load("assets/gui/white.png");
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