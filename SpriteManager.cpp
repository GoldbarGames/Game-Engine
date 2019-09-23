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

SDL_Texture* SpriteManager::GetImage(Renderer* renderer, std::string const& imagePath)
{
	if (images[imagePath].get() == nullptr)
	{
		PHYSFS_file* myfile = PHYSFS_openRead(imagePath.c_str());
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

		SDL_Surface * surface = IMG_Load_RW(rw, 0);
		SDL_Texture * texture = renderer->CreateTextureFromSurface(surface);
		images[imagePath].reset(texture);
		SDL_FreeSurface(surface);
	}
		
	return images[imagePath].get();
}

Vector2 SpriteManager::GetPivotPoint(std::string const& filename)
{
	return pivotPoints[filename];
}