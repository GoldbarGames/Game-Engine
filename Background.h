#pragma once

#include <vector>
#include "Entity.h"

class Background
{
public:
	Vector2 position = Vector2(0, 0);
	std::vector<Entity*> layers;
	Background(Vector2 pos);
	~Background();
	void Render(Renderer * renderer, GLuint uniformModel);
	void AddLayer(SpriteManager* spriteManager, Renderer* renderer, std::string filepath, int drawOrder, float parallax);
	void DeleteLayers(Game& game);
};

