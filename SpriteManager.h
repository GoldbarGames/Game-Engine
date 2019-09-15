#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"
#include "Vector2.h"

class Renderer;

class SpriteManager
{
private:
	std::unordered_map<std::string, std::unique_ptr<SDL_Texture, ImageDeleter>> images;
	std::unordered_map<std::string, Vector2> pivotPoints;
public:
	SDL_Texture* GetImage(Renderer* renderer, std::string const& imagePath);
	Vector2 GetPivotPoint(std::string const& filename);
	SpriteManager();
	~SpriteManager();
};

