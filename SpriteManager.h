#ifndef SPRITEMANAGER_H
#define SPRITEMANAGER_H
#pragma once

#include "SDL.h"
#include <SDL_image.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"
#include "Vector2.h"

#include "Texture.h"

struct AnimState;

class Renderer;

class SpriteManager
{
private:
	mutable std::unordered_map<std::string, Texture*> images;
	std::unordered_map<std::string, std::vector<AnimState*>> animationStates;
public:
	Renderer* renderer = nullptr;
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath);
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath, std::unordered_map<std::string, std::string>& args);
	Texture* GetImage(const std::string& imagePath) const;
	void Init(Renderer* r);
	SpriteManager();
	~SpriteManager();
};

#endif