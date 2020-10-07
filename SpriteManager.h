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
	mutable std::unordered_map<std::string, std::unique_ptr<Texture, ImageDeleter>> images;
	std::unordered_map<std::string, Vector2> pivotPoints;
public:
	Renderer* renderer = nullptr;
	void ReadAnimData(const std::string& dataFilePath, std::vector<AnimState*>& animStates);
	void ReadAnimData(const std::string& dataFilePath, std::vector<AnimState*>& animStates, std::unordered_map<std::string, std::string>& args);
	Texture* GetImage(const std::string& imagePath) const;
	Vector2 GetPivotPoint(const std::string& filename);
	void Init(Renderer* r);
	SpriteManager();
	~SpriteManager();
};

#endif