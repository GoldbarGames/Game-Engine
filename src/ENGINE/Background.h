#ifndef BACKGROUND_H
#define BACKGROUND_H
#pragma once
#include "leak_check.h"
#include <vector>
#include "Vector2.h"
#include <unordered_map>
#include "globals.h"

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
	float scaleX = 1;
	float scaleY = 1;
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
	Vector2 position;
	std::string name = "";
	std::vector<Entity*> layers;
	Background(const std::string& n, const Vector2& pos);
	void CreateBackground(const std::string& n, glm::vec3 pos, const SpriteManager& spriteManager, const Renderer& renderer);
	void ResetBackground();
	~Background();
	void Render(const Renderer& renderer);
	Entity* AddLayer(const glm::vec3& pos, const BackgroundLayerData& data,
		const SpriteManager& spriteManager, const Renderer& renderer);
	void Save(std::unordered_map<std::string, std::string>& map);
	void SpawnBackground(const std::string& n, Game& game);

	static std::unordered_map<std::string, BackgroundData*> bgData;
	static void ReadBackgroundData(const std::string& dataFilePath);
};

#endif