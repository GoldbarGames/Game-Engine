#pragma once
#include "SDL.h"
#include <SDL_image.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"
#include "Vector2.h"

#include "Texture.h"

class Renderer;

class SpriteManager
{
private:
	std::unordered_map<std::string, std::unique_ptr<Texture, ImageDeleter>> images;
	std::unordered_map<std::string, Vector2> pivotPoints;
public:
	Texture* GetImage(std::string const& imagePath);
	Vector2 GetPivotPoint(std::string const& filename);
	SpriteManager();
	~SpriteManager();
};

