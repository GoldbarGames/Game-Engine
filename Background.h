#ifndef BACKGROUND_H
#define BACKGROUND_H
#pragma once

#include <vector>
#include "Vector2.h"
#include <unordered_map>

class Entity;
class SpriteManager;
class Renderer;
class Game;

struct BackgroundLayerData 
{
	int offsetX = 0;
	int offsetY = 0;
	int drawOrder = 0;
	float parallax = 0.0f;
	std::string filepath = "";
};

struct BackgroundData
{
	std::string name = "";
	std::vector<BackgroundLayerData*> layers;
};

class Background
{
public:
	Vector2 position;
	std::string name = "";
	std::vector<Entity*> layers;
	Background(std::string n, Vector2 pos);
	void CreateBackground(std::string n, Vector2 pos, SpriteManager* spriteManager, Renderer* renderer);
	void ResetBackground();
	~Background();
	void Render(Renderer* renderer);
	Entity* AddLayer(Vector2 offset, SpriteManager* spriteManager, Renderer* renderer,
		std::string filepath, int drawOrder, float parallax);
	void DeleteLayers(Game& game);
	void Save(std::ostringstream& level);

	static std::unordered_map<std::string, BackgroundData*> bgData;
	static void ReadBackgroundData(const std::string& dataFilePath);
};

#endif