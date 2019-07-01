#pragma once
#include "SDL.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"

class SpriteManager
{
private:
	std::unordered_map<std::string, std::unique_ptr<SDL_Surface, ImageDeleter>> images;
public:
	SDL_Surface* GetImage(std::string const& image);
	void LoadImage(std::string const & image);
	SpriteManager();
	~SpriteManager();
};

