#ifndef BACKGROUND_H
#define BACKGROUND_H
#pragma once
#include "leak_check.h"
#include <vector>
#include <glm/vec3.hpp>
#include <unordered_map>
#include "globals.h"

class Entity;
class SpriteManager;
class Renderer;
class Game;
class BackgroundLayer;

struct BackgroundLayerData 
{
	int offsetX = 0;
	int offsetY = 0;
	int drawOrder = 0;
	float parallax = 0.0f;
	float scaleX = 1;
	float scaleY = 1;
	int shader = 2;
	Color color = { 255, 255, 255, 255 };
	std::string filepath = "";
};

struct BackgroundData
{
	std::string name = "";
	int width = 1;
	int height = 1;
	int numBGs = 1;
	int xOffset = 0;
	int yOffset = 0;
	std::vector<BackgroundLayerData*> layers;
};

class KINJO_API Background
{
public:
	glm::vec3 position;
	std::string name = "";
	std::vector<BackgroundLayer*> layers;
	Background(const std::string& n, const glm::vec3& pos);
	void ResetBackground();
	~Background();
	void Render(const Renderer& renderer);
	void Save(std::unordered_map<std::string, std::string>& map);
	void SpawnBackground(const std::string& n, int x, int y, Game& game);

	static std::unordered_map<std::string, BackgroundData*> bgData;
	static void ReadBackgroundData(const std::string& dataFilePath);
};

#endif