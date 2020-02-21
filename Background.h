#pragma once

#include <vector>
#include "Entity.h"

class Background
{
public:
	Vector2 position = Vector2(0, 0);
	std::string name = "";
	std::vector<Entity*> layers;
	Background(std::string n, Vector2 pos);
	~Background();
	void Render(Renderer * renderer);
	void AddLayer(SpriteManager* spriteManager, Renderer* renderer, 
		std::string filepath, int drawOrder, float parallax);
	void DeleteLayers(Game& game);
	void Save(std::ostringstream& level);
};

