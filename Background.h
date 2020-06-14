#ifndef BACKGROUND_H
#define BACKGROUND_H
#pragma once

#include <vector>
#include "Vector2.h"

class Entity;
class SpriteManager;
class Renderer;
class Game;

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
	void Render(Renderer * renderer);
	Entity* AddLayer(Vector2 offset, SpriteManager* spriteManager, Renderer* renderer,
		std::string filepath, int drawOrder, float parallax);
	void DeleteLayers(Game& game);
	void Save(std::ostringstream& level);
};

#endif